#include "performance_controls.h"

namespace {
constexpr uint8_t kRotations[music::kLoopTracks] = {0, 2, 4, 1, 3, 0, 4};
uint16_t distance(uint16_t a, uint16_t b) { return a > b ? a - b : b - a; }
uint8_t position9(uint16_t adc) { return static_cast<uint32_t>(adc) * 9 / 4096; }
}

void PerformanceControls::init(const uint16_t adc[3]) {
  for (uint8_t knob = 0; knob < 3; ++knob) free_anchor_[knob] = adc[knob];
  tempo_bpm_ = music::tempo_bpm(adc[0]);
  bounce_ = adc[1] >> 4;
  manual_mode_ = adc[0] <= 120;
  space_ = adc[2] >> 4;
  for (uint8_t i = 0; i < music::kTracks; ++i) {
    pulses_[i] = i < 5 ? 3 : i == 5 ? 4 : i == 6 ? 2 : 0;
    tone_[i] = 4;    // Center leaves each voice at its original height.
    length_[i] = 4;  // Center leaves each voice at its original decay.
    if (i < music::kLoopTracks)
      pattern_[i] = music::euclidean_pattern(pulses_[i], kRotations[i]);
  }
}

void PerformanceControls::press(uint8_t track, const uint16_t adc[3]) {
  if (track >= music::kLoopTracks) return;
  const uint8_t bit = 1u << track;
  held_ |= bit;
  edited_ &= ~bit;
  for (uint8_t knob = 0; knob < 3; ++knob) anchors_[knob][track] = adc[knob];
}

void PerformanceControls::release(uint8_t track, const uint16_t adc[3]) {
  if (track >= music::kLoopTracks) return;
  const uint8_t bit = 1u << track;
  if (!(held_ & bit)) return;
  held_ &= ~bit;
  if (!manual_mode_ && !(edited_ & bit)) enabled_ ^= bit;
  edited_ &= ~bit;
  if (!held_)
    for (uint8_t knob = 0; knob < 3; ++knob) free_anchor_[knob] = adc[knob];
}

void PerformanceControls::cancel(uint8_t track, const uint16_t adc[3]) {
  if (track >= music::kLoopTracks) return;
  const uint8_t bit = 1u << track;
  held_ &= ~bit;
  edited_ &= ~bit;
  if (!held_)
    for (uint8_t knob = 0; knob < 3; ++knob) free_anchor_[knob] = adc[knob];
}

void PerformanceControls::toggle_step(uint8_t track, uint8_t step) {
  if (track >= music::kLoopTracks || step >= music::kSteps) return;
  pattern_[track] ^= 1u << step;
  hand_edited_ |= 1u << track;
  if (pattern_[track] & (1u << step)) enabled_ |= 1u << track;
}

uint8_t PerformanceControls::turn(const uint16_t adc[3], bool sequencer_mode) {
  uint8_t audition = 0;
  if (held_) {
    for (uint8_t knob = 0; knob < 3; ++knob) {
      free_anchor_[knob] = adc[knob];
      for (uint8_t i = 0; i < music::kTracks; ++i) {
        const uint8_t bit = 1u << i;
        if (!(held_ & bit) ||
            distance(adc[knob], anchors_[knob][i]) < kMoveThreshold) continue;
        if (knob == 0) {
          const uint8_t next = position9(adc[knob]);
          if (tone_[i] != next) audition |= bit;
          tone_[i] = next;
        } else if (knob == 1) {
          pulses_[i] = music::density(adc[knob]);
          pattern_[i] = music::euclidean_pattern(pulses_[i], kRotations[i]);
          hand_edited_ &= ~bit;
        } else {
          const uint8_t next = position9(adc[knob]);
          if (length_[i] != next) audition |= bit;
          length_[i] = next;
        }
        edited_ |= bit;
        anchors_[knob][i] = adc[knob];
      }
    }
  } else {
    if (distance(adc[0], free_anchor_[0]) >= kMoveThreshold) {
      tempo_bpm_ = music::tempo_bpm(adc[0]);
      if (adc[0] <= 120) manual_mode_ = true;
      else if (adc[0] >= 240) manual_mode_ = false;
      free_anchor_[0] = adc[0];
    }
    if (!sequencer_mode && distance(adc[1], free_anchor_[1]) >= kMoveThreshold) {
      bounce_ = adc[1] >> 4;
      free_anchor_[1] = adc[1];
    }
    if (sequencer_mode) free_anchor_[1] = adc[1];
    if (distance(adc[2], free_anchor_[2]) >= kMoveThreshold) {
      space_ = adc[2] >> 4;
      free_anchor_[2] = adc[2];
    }
  }
  return audition;
}
