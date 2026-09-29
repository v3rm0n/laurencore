#include "performance_controls.h"
#include <cassert>

int main() {
  PerformanceControls controls;
  const uint16_t initial[3] = {1000, 1000, 0};
  controls.init(initial);
  const uint16_t start_tempo = controls.tempo_bpm();
  const uint8_t start_bounce = controls.bounce();
  assert(controls.pulses(0) == 3 && controls.pulses(5) == 4);
  assert(controls.pattern(0) == music::euclidean_pattern(3, 0));
  assert(controls.pulses(6) == 2 && controls.pulses(7) == 0);
  assert(controls.tone(0) == 4 && controls.length(0) == 4);
  assert(controls.space() == 0 && !controls.manual_mode());

  controls.press(0, initial);
  controls.release(0, initial);
  assert(controls.enabled() == 1);
  controls.press(7, initial);  // Magic Fill is not a looping part.
  controls.release(7, initial);
  assert(controls.enabled() == 1);

  const uint16_t small_move[3] = {1030, 1000, 0};
  controls.press(0, initial);
  assert(controls.turn(small_move) == 0);
  controls.release(0, small_move);
  assert(controls.enabled() == 0);  // a tap despite knob noise

  const uint16_t high_note[3] = {3000, 1000, 0};
  controls.press(0, initial);
  assert(controls.turn(high_note) == 1);
  controls.release(0, high_note);
  assert(controls.enabled() == 0);
  assert(controls.tone(0) == 6 && controls.tone(1) == 4);
  assert(controls.tempo_bpm() == start_tempo);
  controls.turn(high_note);
  assert(controls.tempo_bpm() == start_tempo);  // no global jump on release

  const uint16_t new_rhythm[3] = {3000, 3500, 0};
  controls.press(1, high_note);
  controls.turn(new_rhythm);
  controls.release(1, new_rhythm);
  assert(controls.pulses(1) == music::density(3500));
  assert(controls.pattern(1) == music::euclidean_pattern(controls.pulses(1), 2));
  assert(controls.pulses(0) == 3);
  assert(controls.bounce() == start_bounce);
  controls.turn(new_rhythm);
  assert(controls.bounce() == start_bounce);

  const uint16_t long_sound[3] = {3000, 3500, 3600};
  controls.press(1, new_rhythm);
  controls.press(6, new_rhythm);
  assert(controls.turn(long_sound) == ((1u << 1) | (1u << 6)));
  controls.release(1, long_sound);
  controls.release(6, long_sound);
  assert(controls.length(1) == 7 && controls.length(6) == 7);
  assert(controls.length(0) == 4 && controls.space() == 0);

  const uint16_t free_turn[3] = {3200, 3700, 3800};
  controls.turn(free_turn);
  assert(controls.tempo_bpm() == music::tempo_bpm(3200));
  assert(controls.bounce() == (3700 >> 4));
  assert(controls.space() == (3800 >> 4));
  assert(controls.pulses(1) == music::density(3500));

  // The tempo knob's bottom stop is manual play; taps preserve saved loops.
  controls.press(2, free_turn);
  controls.release(2, free_turn);
  assert(controls.enabled() == (1u << 2));
  const uint16_t manual[3] = {0, 3700, 3800};
  controls.turn(manual);
  assert(controls.manual_mode());
  controls.press(0, manual);
  controls.release(0, manual);
  controls.press(2, manual);
  controls.release(2, manual);
  assert(controls.enabled() == (1u << 2));
  controls.turn(free_turn);
  assert(!controls.manual_mode());
  controls.press(0, free_turn);
  controls.release(0, free_turn);
  assert(controls.enabled() == ((1u << 2) | 1));

  // Hand-edited steps play as stored; regenerating one rhythm leaves others.
  const uint8_t other_pattern = controls.pattern(2);
  const uint8_t initial_pattern = controls.pattern(1);
  controls.toggle_step(1, 0);
  assert(controls.hand_edited() & (1u << 1));
  assert(controls.pattern(1) == static_cast<uint8_t>(initial_pattern ^ 1u));
  assert(controls.pattern(2) == other_pattern);
  const uint16_t next_rhythm[3] = {3200, 1000, 3800};
  controls.press(1, free_turn);
  controls.turn(next_rhythm);
  controls.release(1, next_rhythm);
  assert(!(controls.hand_edited() & (1u << 1)));
  assert(controls.pattern(1) == music::euclidean_pattern(controls.pulses(1), 2));
  assert(controls.pattern(2) == other_pattern);

  // Editing a silent part makes its new step audible in the running loop.
  assert(!(controls.enabled() & (1u << 6)));
  controls.toggle_step(6, 3);
  assert(controls.enabled() & (1u << 6));
  assert(controls.pattern(6) & (1u << 3));
  const uint8_t saved = controls.enabled();
  controls.press(5, next_rhythm);
  controls.cancel(5, next_rhythm);
  controls.release(5, next_rhythm);
  assert(controls.enabled() == saved);

  // Selecting a sound in sequencer mode must not move Bounce.
  const uint8_t bounce = controls.bounce();
  const uint16_t sequencer_knob[3] = {3200, 4000, 3800};
  controls.turn(sequencer_knob, true);
  assert(controls.bounce() == bounce);
}
