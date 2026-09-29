#pragma once
#include <cstdint>

namespace music {
constexpr uint8_t kTracks = 8;
constexpr uint8_t kLoopTracks = 7;
constexpr uint8_t kSteps = 8;
// A Euclidean rhythm has its hits spread as evenly as possible across a cycle.
bool hit(uint8_t step, uint8_t pulses, uint8_t rotation = 0);
uint8_t euclidean_pattern(uint8_t pulses, uint8_t rotation = 0);
uint16_t tempo_bpm(uint16_t adc);
uint8_t density(uint16_t adc);
uint32_t step_samples(uint32_t straight_samples, uint8_t step, uint8_t bounce);
uint8_t groove_extra_hits(uint8_t step, uint32_t phrase, uint8_t bounce,
                          uint8_t enabled);
uint8_t fill_mask(uint8_t variant, uint8_t step);
uint8_t next_melody_note(uint8_t previous, uint32_t random);
uint8_t melody_pitch_index(uint8_t note, uint8_t tone);
}
