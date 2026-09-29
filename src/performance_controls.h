#pragma once
#include <cstdint>
#include "music.h"

// Runs only on the control loop; the audio interrupt reads a copied snapshot.
class PerformanceControls {
 public:
  void init(const uint16_t adc[3]);
  void press(uint8_t track, const uint16_t adc[3]);
  void release(uint8_t track, const uint16_t adc[3]);
  void cancel(uint8_t track, const uint16_t adc[3]);
  void toggle_step(uint8_t track, uint8_t step);
  // Returns parts whose tone or length changed, so they can be auditioned.
  uint8_t turn(const uint16_t adc[3], bool sequencer_mode = false);
  uint8_t enabled() const { return enabled_; }
  uint8_t held() const { return held_; }
  uint8_t pulses(uint8_t track) const { return pulses_[track]; }
  uint8_t pattern(uint8_t track) const { return pattern_[track]; }
  uint8_t hand_edited() const { return hand_edited_; }
  uint8_t tone(uint8_t track) const { return tone_[track]; }
  uint8_t length(uint8_t track) const { return length_[track]; }
  uint16_t tempo_bpm() const { return tempo_bpm_; }
  uint8_t bounce() const { return bounce_; }
  bool manual_mode() const { return manual_mode_; }
  uint8_t space() const { return space_; }

 private:
  static constexpr uint16_t kMoveThreshold = 80;
  uint8_t enabled_ = 0;
  uint8_t held_ = 0;
  uint8_t edited_ = 0;
  uint8_t pulses_[music::kTracks] = {};
  uint8_t pattern_[music::kLoopTracks] = {};
  uint8_t hand_edited_ = 0;
  uint8_t tone_[music::kTracks] = {};
  uint8_t length_[music::kTracks] = {};
  uint16_t anchors_[3][music::kTracks] = {};
  uint16_t free_anchor_[3] = {};
  uint16_t tempo_bpm_ = 100;
  uint8_t bounce_ = 0;
  bool manual_mode_ = false;
  uint8_t space_ = 0;
};
