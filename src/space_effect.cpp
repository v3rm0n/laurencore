#include "space_effect.h"

namespace {
int16_t bounded(int32_t sample) {
  if (sample > 4096) return 4096;
  if (sample < -4096) return -4096;
  return static_cast<int16_t>(sample);
}

int16_t comb(int16_t* buffer, uint16_t length, uint16_t& head, int16_t input) {
  int16_t delayed = buffer[head];
  buffer[head] = bounded(input + delayed * 3 / 4);
  if (++head == length) head = 0;
  return delayed;
}
}

int16_t SpaceEffect::process(int16_t dry, uint16_t delay_samples,
                             uint8_t amount) {
  if (delay_samples == 0) delay_samples = 1;
  if (delay_samples >= kEchoCapacity) delay_samples = kEchoCapacity - 1;
  // Glide the tap when tempo changes so it cannot jump to an unrelated sample.
  if (current_delay_ == 0) current_delay_ = delay_samples;
  else if (current_delay_ < delay_samples) ++current_delay_;
  else if (current_delay_ > delay_samples) --current_delay_;
  uint16_t tap = echo_head_ >= current_delay_
                     ? echo_head_ - current_delay_
                     : echo_head_ + kEchoCapacity - current_delay_;
  int16_t echo = echo_[tap];
  echo_[echo_head_] = bounded(dry + echo * 3 / 8);
  if (++echo_head_ == kEchoCapacity) echo_head_ = 0;

  int16_t room_input = dry / 2;
  int32_t room = comb(comb_a_, kCombA, head_a_, room_input);
  room += comb(comb_b_, kCombB, head_b_, room_input);
  room += comb(comb_c_, kCombC, head_c_, room_input);
  room /= 3;
  int16_t delayed_room = allpass_[allpass_head_];
  allpass_[allpass_head_] = bounded(room + delayed_room / 2);
  if (++allpass_head_ == kAllpass) allpass_head_ = 0;
  int32_t reverb = delayed_room - room / 2;

  int32_t echo_wet = amount <= 128 ? amount * 96 / 128 : 96;
  int32_t room_wet = amount <= 128 ? 0 : (amount - 128) * 128 / 127;
  int32_t dry_gain = 256 - (echo_wet + room_wet) / 2;
  int32_t output =
      (dry * dry_gain + echo * echo_wet + reverb * room_wet) / 256;
  if (output > 500) output = 500;
  if (output < -500) output = -500;
  return static_cast<int16_t>(output);
}
