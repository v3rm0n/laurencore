#pragma once
#include <cstdint>

// A single mono bus: tempo-synced echo, then a small diffused room tail.
class SpaceEffect {
 public:
  static constexpr uint16_t kEchoCapacity = 12000;
  int16_t process(int16_t dry, uint16_t delay_samples, uint8_t amount);

 private:
  static constexpr uint16_t kCombA = 863;
  static constexpr uint16_t kCombB = 1061;
  static constexpr uint16_t kCombC = 1307;
  static constexpr uint16_t kAllpass = 347;
  int16_t echo_[kEchoCapacity] = {};
  int16_t comb_a_[kCombA] = {};
  int16_t comb_b_[kCombB] = {};
  int16_t comb_c_[kCombC] = {};
  int16_t allpass_[kAllpass] = {};
  uint16_t echo_head_ = 0;
  uint16_t current_delay_ = 0;
  uint16_t head_a_ = 0;
  uint16_t head_b_ = 0;
  uint16_t head_c_ = 0;
  uint16_t allpass_head_ = 0;
};
