#include "music.h"
namespace music {
bool hit(uint8_t step, uint8_t pulses, uint8_t rotation) {
  if (pulses == 0) return false;
  if (pulses >= kSteps) return true;
  return (((step + rotation) % kSteps) * pulses) % kSteps < pulses;
}
uint8_t euclidean_pattern(uint8_t pulses, uint8_t rotation) {
  uint8_t pattern = 0;
  for (uint8_t step = 0; step < kSteps; ++step)
    if (hit(step, pulses, rotation)) pattern |= 1u << step;
  return pattern;
}
uint16_t tempo_bpm(uint16_t adc) { return 65 + static_cast<uint32_t>(adc) * 85 / 4095; }
uint8_t density(uint16_t adc) { return 1 + static_cast<uint32_t>(adc) * 6 / 4095; }
uint32_t step_samples(uint32_t straight_samples, uint8_t step, uint8_t bounce) {
  const uint32_t swing = straight_samples * bounce / 1024;
  return step & 1 ? straight_samples - swing : straight_samples + swing;
}
uint8_t groove_extra_hits(uint8_t step, uint32_t phrase, uint8_t bounce,
                          uint8_t enabled) {
  if (bounce < 170 || phrase % 4 != 3) return 0;
  if (step == 7 && (enabled & (1u << 5))) return 1u << 5;
  if (step == 6 && bounce >= 225 && (enabled & (1u << 6))) return 1u << 6;
  return 0;
}
uint8_t fill_mask(uint8_t variant, uint8_t step) {
  static constexpr uint8_t patterns[4][kSteps] = {
      {1u << 0, 0, 1u << 2, 0, 1u << 3, 1u << 5, 1u << 4, 1u << 7},
      {1u << 5, 1u << 1, 1u << 2, 1u << 6, 1u << 3, 1u << 4,
       1u << 7, 1u << 5},
      {1u << 0, 1u << 1, 1u << 6, 1u << 2, 1u << 3, 1u << 4,
       1u << 5, 1u << 7},
      {(1u << 5) | (1u << 0), 1u << 2, 1u << 7, 1u << 6,
       1u << 3, 1u << 4, (1u << 5) | (1u << 6), 1u << 7}};
  return patterns[variant % 4][step % kSteps];
}
uint8_t next_melody_note(uint8_t previous, uint32_t random) {
  // Mostly move to a neighbour, sometimes repeat or make a wider jump.
  const uint8_t current = previous % 5;
  const uint8_t choice = random % 10;
  if (choice == 0) return current;
  const uint8_t distance = choice >= 8 ? 2 : 1;
  const int8_t direction = (random & 16u) ? 1 : -1;
  const int8_t candidate = static_cast<int8_t>(current) + direction * distance;
  if (candidate < 0 || candidate >= 5)
    return static_cast<uint8_t>(static_cast<int8_t>(current) - direction * distance);
  return static_cast<uint8_t>(candidate);
}
uint8_t melody_pitch_index(uint8_t note, uint8_t tone) {
  return static_cast<uint8_t>(5 + note % 5 + tone % 9 - 4);
}
}
