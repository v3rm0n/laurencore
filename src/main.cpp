#include "music.h"
#include "performance_controls.h"
#include "space_effect.h"
#include <cstdint>
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"

namespace {
constexpr uint8_t kAudioPin = 20;
constexpr uint16_t kPwmTop = 1023;
constexpr bool kSwapFunctionKnobs = false;
constexpr uint32_t kButtonDebounceMs = 30;
constexpr uint32_t kMelodyHoldMs = 450;
constexpr uint32_t kSequencerChordMs = 1000;
constexpr uint32_t kSynthAttackMs = 8;
constexpr int32_t kSynthFilterDivisor = 4;
// C major pentatonic from C3 to E7. Every pitched edit stays in this scale.
constexpr double kScaleHz[23] = {
    130.81, 146.83, 164.81, 196.00, 220.00,
    261.63, 293.66, 329.63, 392.00, 440.00,
    523.25, 587.33, 659.25, 783.99, 880.00,
    1046.50, 1174.66, 1318.51, 1567.98, 1760.00,
    2093.00, 2349.32, 2637.02};

struct Voice {
  uint32_t phase = 0;
  uint32_t phase2 = 0;
  uint32_t increment = 0;
  uint32_t increment2 = 0;
  uint16_t envelope = 0;
  uint16_t decrement = 1;
  uint16_t gain = 0;
  int32_t filtered = 0;
  bool melody_mode = false;
};
Voice voices[music::kTracks];
volatile uint8_t enabled = 0;
volatile uint8_t step = 0;
volatile uint8_t last_hits = 0;
volatile uint32_t step_serial = 0;
volatile uint8_t track_pattern[music::kLoopTracks] = {};
volatile uint8_t hand_edited_patterns = 0;
volatile uint8_t track_tone[music::kTracks] = {4, 4, 4, 4, 4, 4, 4, 4};
volatile uint8_t track_length[music::kTracks] = {4, 4, 4, 4, 4, 4, 4, 4};
volatile uint8_t space_amount = 0;
volatile uint8_t bounce_amount = 0;
volatile bool manual_mode = false;
bool sequencer_mode = false;
uint8_t sequencer_track = 0;
bool chord_active = false;
bool chord_consumed = false;
uint32_t chord_started_ms = 0;
uint8_t suppress_release_mask = 0;
volatile uint8_t fill_steps_left = 0;
volatile uint8_t fill_position = 0;
volatile uint8_t fill_variant = 0;
volatile uint32_t samples_per_step = 3000;
uint32_t samples_until_step = 0;
uint32_t phrase_count = 0;
uint8_t next_fill_variant = 0;
volatile bool melody_held = false;
volatile uint8_t melody_steps_until_note = 2;
uint8_t melody_position = 2;
uint32_t melody_rng = 0xAC56823Du;
uint32_t button8_press_ms = 0;
uint32_t noise_state = 0x3A9E475Du;
uint32_t sample_rate = 24414;
uint16_t synth_attack_step = 336;
uint32_t scale_increments[23];
uint32_t kick_increments[9];
uint32_t snare_increments[9];
uint16_t decay_steps[music::kTracks][9];
SpaceEffect space_effect;

uint32_t phase_increment(double frequency) {
  return static_cast<uint32_t>(frequency * 4294967296.0 / sample_rate);
}

void trigger(uint8_t track) {
  Voice& v = voices[track];
  if (track == 7) v.melody_mode = false;
  // Keep ringing notes and the chime phase-continuous across retriggers.
  if (track == 5 || track == 6 || v.envelope == 0) {
    v.phase = 0;
    v.phase2 = 0;
    v.filtered = 0;
  }
  v.envelope = 65535;
  const uint8_t tone = track_tone[track];
  v.decrement = decay_steps[track][track_length[track]];
  if (track < 5) {
    v.increment = scale_increments[5 + track + tone - 4];
  } else if (track == 7) {
    v.increment = scale_increments[15 + tone - 4];
    v.increment2 = scale_increments[18 + tone - 4];
  } else if (track == 5) {
    v.increment = kick_increments[tone];
  } else {
    v.increment = snare_increments[tone];
  }
}

void play_editor_chime(bool entering) {
  // A high pair announces entry; a lower pair announces exit.
  Voice& v = voices[7];
  v.phase = 0;
  v.phase2 = 0;
  v.filtered = 0;
  v.gain = 0;
  v.melody_mode = false;
  v.envelope = 65535;
  v.decrement = decay_steps[7][2];
  v.increment = scale_increments[entering ? 15 : 10];  // C6 or C5
  v.increment2 = scale_increments[entering ? 18 : 13]; // G6 or G5
}

void trigger_melody_note() {
  melody_rng ^= melody_rng << 13;
  melody_rng ^= melody_rng >> 17;
  melody_rng ^= melody_rng << 5;
  melody_position = music::next_melody_note(melody_position, melody_rng);
  Voice& v = voices[7];
  v.melody_mode = true;
  if (v.envelope == 0) {
    v.phase = 0;
    v.filtered = 0;
  }
  v.envelope = 65535;
  v.increment = scale_increments[music::melody_pitch_index(
      melody_position, track_tone[melody_position])];
  v.decrement = decay_steps[melody_position][track_length[melody_position]];
}

int32_t triangle(uint32_t phase) {
  uint16_t x = phase >> 22;  // 0..1023
  return x < 512 ? static_cast<int32_t>(x) * 2 - 512
                 : 1534 - static_cast<int32_t>(x) * 2;
}

int32_t render_voice(uint8_t track) {
  Voice& v = voices[track];
  if (v.envelope == 0) return 0;
  noise_state ^= noise_state << 13;
  noise_state ^= noise_state >> 17;
  noise_state ^= noise_state << 5;
  int32_t noise = static_cast<int32_t>((noise_state >> 22) & 1023) - 512;
  int32_t wave = 0;
  if (track < 5) {
    const uint32_t phase = v.phase;
    int32_t fundamental = triangle(phase);
    // Each key keeps its own character now that ADC2 controls Space.
    switch (track) {
      case 0: wave = fundamental; break;                // soft
      case 1: wave = (fundamental * 3 + triangle(phase * 2)) / 4; break; // wooden
      case 2: wave = (fundamental * 2 + triangle(phase * 3)) / 3; break; // bell
      case 3: wave = (fundamental + triangle(phase * 2)) / 2; break; // bright
      default: wave = (fundamental * 4 + triangle(phase * 2)) / 5; break;
    }
  } else if (track == 5) {
    // A downward sweep and short envelope make a gentle kick.
    wave = triangle(v.phase);
    v.increment -= v.increment > kick_increments[track_tone[5]] / 3
                       ? v.increment / 6000 : 0;
  } else if (track == 6) {
    // The same low-to-high gesture darkens or brightens the snare noise.
    int32_t divisor = 12 - track_tone[6] * 10 / 8;
    v.filtered += (noise - v.filtered) / divisor;
    wave = (v.filtered * 3 + triangle(v.phase)) / 4;
  } else {
    if (v.melody_mode) {
      // A gentle single-note lead in the same register as keys 1-5.
      wave = (triangle(v.phase) * 4 + triangle(v.phase * 2)) / 5;
    } else {
      // C6 and G6 make a bright, consonant ping unlike the noise-based snare.
      wave = (triangle(v.phase) * 3 + triangle(v.phase2) * 2) / 5;
      v.phase2 += v.increment2;
    }
  }
  const bool tonal = track < 5 || track == 7;
  if (tonal) {
    // Low-pass and ease in each pitched sound to avoid clicks at note starts.
    const int32_t divisor = track == 7 ? 3 : kSynthFilterDivisor;
    v.filtered += (wave - v.filtered) / divisor;
    wave = v.filtered;
    uint32_t attack = track == 7 ? synth_attack_step * 2 : synth_attack_step;
    uint32_t next_gain = static_cast<uint32_t>(v.gain) + attack;
    v.gain = next_gain < v.envelope ? next_gain : v.envelope;
  }
  v.phase += v.increment;
  int32_t output = wave * (tonal ? v.gain : v.envelope) / 65535;
  v.envelope = v.envelope > v.decrement ? v.envelope - v.decrement : 0;
  if (tonal && v.gain > v.envelope) v.gain = v.envelope;
  return output;
}

void audio_interrupt() {
  pwm_clear_irq(pwm_gpio_to_slice_num(kAudioPin));
  if (samples_until_step == 0) {
    uint8_t current = step;
    uint8_t active = enabled;
    uint8_t hits = 0;
    if (!manual_mode) {
      for (uint8_t i = 0; i < music::kLoopTracks; ++i)
        if ((active & (1u << i)) &&
            (track_pattern[i] & (1u << current)))
          hits |= 1u << i;
      hits |= music::groove_extra_hits(current, phrase_count,
                                      bounce_amount,
                                      active & ~hand_edited_patterns);
      if (fill_steps_left) {
        hits |= music::fill_mask(fill_variant, fill_position);
        ++fill_position;
        --fill_steps_left;
      }
      for (uint8_t i = 0; i < music::kTracks; ++i)
        if (hits & (1u << i)) trigger(i);
    }
    if (melody_held && --melody_steps_until_note == 0) {
      trigger_melody_note();
      melody_steps_until_note = 2;
    }
    last_hits = hits;
    ++step_serial;
    step = (current + 1) % music::kSteps;
    if (current == music::kSteps - 1) ++phrase_count;
    samples_until_step = music::step_samples(samples_per_step, current,
                                            bounce_amount);
  }
  --samples_until_step;
  int32_t mix = 0;
  for (uint8_t i = 0; i < music::kTracks; ++i) mix += render_voice(i);
  mix /= 8;
  if (mix > 500) mix = 500;
  if (mix < -500) mix = -500;
  int16_t output = space_effect.process(
      static_cast<int16_t>(mix), samples_per_step * 2, space_amount);
  pwm_set_gpio_level(kAudioPin, 512 + output);
}

uint16_t read_knob(uint8_t input) {
  adc_select_input(input);
  // Pikocore firmware uses an inverted ADC reading for these pots.
  return 4095 - adc_read();
}

struct Button {
  bool stable = false;
  bool last_raw = false;
  uint32_t changed_at = 0;
};
Button buttons[music::kTracks];
uint8_t sequencer_press_track[music::kTracks] = {};
uint16_t raw_knobs[3];
PerformanceControls controls;

uint8_t rhythm_knob_index() { return kSwapFunctionKnobs ? 2 : 1; }
uint8_t space_knob_index() { return kSwapFunctionKnobs ? 1 : 2; }

void logical_knobs(uint16_t adc[3]) {
  adc[0] = raw_knobs[0];
  adc[1] = raw_knobs[rhythm_knob_index()];
  adc[2] = raw_knobs[space_knob_index()];
}

void publish_controls(uint8_t audition = 0) {
  uint32_t flags = save_and_disable_interrupts();
  enabled = controls.enabled();
  hand_edited_patterns = controls.hand_edited();
  for (uint8_t i = 0; i < music::kTracks; ++i) {
    track_tone[i] = controls.tone(i);
    track_length[i] = controls.length(i);
    if (i < music::kLoopTracks) track_pattern[i] = controls.pattern(i);
  }
  samples_per_step = sample_rate * 60 / (controls.tempo_bpm() * 4);
  bounce_amount = controls.bounce();
  space_amount = controls.space();
  manual_mode = controls.manual_mode();
  if (manual_mode) fill_steps_left = 0;
  for (uint8_t i = 0; i < music::kTracks; ++i)
    if (audition & (1u << i)) {
      trigger(i);
      if (manual_mode && i < 5 && buttons[i].stable) voices[i].decrement = 0;
    }
  restore_interrupts(flags);
}

void poll_buttons(uint32_t now_ms) {
  for (uint8_t i = 0; i < music::kTracks; ++i) {
    bool raw = !gpio_get(4 + i);
    Button& b = buttons[i];
    if (raw != b.last_raw) {
      b.last_raw = raw;
      b.changed_at = now_ms;
    }
    if (raw != b.stable && now_ms - b.changed_at >= kButtonDebounceMs) {
      b.stable = raw;
      const uint8_t bit = 1u << i;
      if (sequencer_mode) {
        if (raw) {
          sequencer_press_track[i] = sequencer_track;
          uint32_t flags = save_and_disable_interrupts();
          trigger(sequencer_track);
          restore_interrupts(flags);
        } else if (!(suppress_release_mask & bit)) {
          controls.toggle_step(sequencer_press_track[i], i);
          publish_controls();
        }
        continue;
      }
      if (i == 7) {
        if (raw) {
          uint32_t flags = save_and_disable_interrupts();
          button8_press_ms = now_ms;
          fill_steps_left = 0;
          melody_held = true;
          melody_steps_until_note = 2;
          trigger_melody_note();
          restore_interrupts(flags);
        } else {
          uint32_t flags = save_and_disable_interrupts();
          melody_held = false;
          if (!(suppress_release_mask & bit) && !manual_mode &&
              b.changed_at - button8_press_ms < kMelodyHoldMs) {
            fill_variant = next_fill_variant;
            next_fill_variant = (next_fill_variant + 1) % 4;
            fill_position = 0;
            fill_steps_left = music::kSteps;
          }
          restore_interrupts(flags);
        }
        continue;
      }
      uint16_t adc[3];
      logical_knobs(adc);
      if (raw) {
        controls.press(i, adc);
        uint32_t flags = save_and_disable_interrupts();
        trigger(i);
        if (manual_mode && i < 5) voices[i].decrement = 0;
        restore_interrupts(flags);
      } else {
        if (!(suppress_release_mask & bit)) controls.release(i, adc);
        publish_controls();
        if (manual_mode && i < 5) {
          uint32_t flags = save_and_disable_interrupts();
          voices[i].decrement = decay_steps[i][track_length[i]];
          restore_interrupts(flags);
        }
      }
    }
  }
  const bool both = buttons[5].stable && buttons[6].stable;
  if (both && !chord_active) {
    chord_active = true;
    chord_started_ms = now_ms;
  }
  if (both && !chord_consumed &&
      now_ms - chord_started_ms >= kSequencerChordMs) {
    chord_consumed = true;
    uint16_t adc[3];
    logical_knobs(adc);
    for (uint8_t i = 0; i < music::kTracks; ++i)
      if (buttons[i].stable) {
        suppress_release_mask |= 1u << i;
        if (!sequencer_mode) controls.cancel(i, adc);
      }
    if (!sequencer_mode) publish_controls();
    sequencer_mode = !sequencer_mode;
    uint32_t flags = save_and_disable_interrupts();
    melody_held = false;
    fill_steps_left = 0;
    play_editor_chime(sequencer_mode);
    restore_interrupts(flags);
  }
  if (!both) chord_active = false;
  if (!buttons[5].stable && !buttons[6].stable) chord_consumed = false;
  uint8_t still_held = 0;
  for (uint8_t i = 0; i < music::kTracks; ++i)
    if (buttons[i].stable) still_held |= 1u << i;
  suppress_release_mask &= still_held;
}

void poll_knobs() {
  for (uint8_t i = 0; i < 3; ++i) raw_knobs[i] = read_knob(i);
  uint16_t adc[3];
  logical_knobs(adc);
  if (sequencer_mode)
    sequencer_track = static_cast<uint32_t>(adc[1]) * music::kLoopTracks / 4096;
  publish_controls(controls.turn(adc, sequencer_mode));
}

void update_leds(uint32_t now_ms) {
  static uint32_t seen_serial = 0;
  static uint32_t flash_until_ms = 0;
  static uint8_t flashing = 0;
  uint32_t serial = step_serial;
  if (serial != seen_serial) {
    seen_serial = serial;
    flashing = last_hits;
    flash_until_ms = now_ms + 45;
  }
  uint8_t active = enabled;
  if (sequencer_mode) {
    const uint8_t pattern = track_pattern[sequencer_track];
    const uint8_t playing_step = (step + music::kSteps - 1) % music::kSteps;
    for (uint8_t i = 0; i < music::kSteps; ++i) {
      bool lit = (pattern & (1u << i)) != 0;
      if (i == playing_step && now_ms < flash_until_ms) lit = !lit;
      gpio_put(12 + i, lit);
    }
    return;
  }
  if (manual_mode) {
    for (uint8_t i = 0; i < music::kTracks; ++i)
      gpio_put(12 + i, buttons[i].stable);
    return;
  }
  for (uint8_t i = 0; i < music::kLoopTracks; ++i) {
    bool lit = (active & (1u << i)) != 0;
    bool blink = now_ms < flash_until_ms && (flashing & (1u << i));
    bool selected = (controls.held() & (1u << i)) != 0;
    gpio_put(12 + i, selected ? now_ms % 200 < 100 : (lit && !blink));
  }
  gpio_put(19, melody_held ? now_ms % 260 < 180
                          : fill_steps_left && now_ms % 140 < 90);
}
}

int main() {
  for (uint8_t i = 0; i < music::kTracks; ++i) {
    gpio_init(4 + i);
    gpio_set_dir(4 + i, GPIO_IN);
    gpio_pull_up(4 + i);
    gpio_init(12 + i);
    gpio_set_dir(12 + i, GPIO_OUT);
  }
  adc_init();
  for (uint8_t i = 0; i < 3; ++i) {
    adc_gpio_init(26 + i);
    raw_knobs[i] = read_knob(i);
  }
  uint16_t adc[3];
  logical_knobs(adc);
  controls.init(adc);
  gpio_set_function(kAudioPin, GPIO_FUNC_PWM);
  uint slice = pwm_gpio_to_slice_num(kAudioPin);
  pwm_config config = pwm_get_default_config();
  pwm_config_set_clkdiv(&config, 5.0f);
  pwm_config_set_wrap(&config, kPwmTop);
  sample_rate = clock_get_hz(clk_sys) / (5 * (kPwmTop + 1));
  synth_attack_step = 65535u * 1000u / (sample_rate * kSynthAttackMs);
  for (uint8_t i = 0; i < 23; ++i)
    scale_increments[i] = phase_increment(kScaleHz[i]);
  for (uint8_t setting = 0; setting < 9; ++setting) {
    kick_increments[setting] = phase_increment(90 + setting * 15);
    snare_increments[setting] = phase_increment(100 + setting * 20);
    for (uint8_t track = 0; track < music::kTracks; ++track) {
      const uint32_t square = setting * setting;
      const uint32_t duration_ms = track < 5 ? 80 + square * 20
                                   : track == 5 ? 50 + square * 5
                                   : track == 6 ? 35 + square * 3
                                                : 120 + setting * 100;
      const uint32_t duration_samples = sample_rate * duration_ms / 1000;
      uint32_t decrement = (65535 + duration_samples / 2) / duration_samples;
      if (decrement == 0) decrement = 1;
      decay_steps[track][setting] = decrement;
    }
  }
  publish_controls();
  pwm_init(slice, &config, false);
  pwm_set_gpio_level(kAudioPin, 512);
  pwm_clear_irq(slice);
  irq_set_exclusive_handler(PWM_IRQ_WRAP, audio_interrupt);
  pwm_set_irq_enabled(slice, true);
  irq_set_enabled(PWM_IRQ_WRAP, true);
  pwm_set_enabled(slice, true);
  while (true) {
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    poll_buttons(now_ms);
    poll_knobs();
    update_leds(now_ms);
    sleep_ms(10);
  }
}
