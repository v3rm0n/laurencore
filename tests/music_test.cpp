#include "music.h"
#include <cassert>
#include <initializer_list>
int main() {
  for (unsigned pulses = 0; pulses <= 8; ++pulses) {
    for (unsigned rotation = 0; rotation < 8; ++rotation) {
      unsigned count = 0;
      for (unsigned step = 0; step < 8; ++step)
        count += music::hit(step, pulses, rotation);
      assert(count == pulses);
      uint8_t pattern = music::euclidean_pattern(pulses, rotation);
      for (unsigned step = 0; step < 8; ++step)
        assert(((pattern >> step) & 1u) == music::hit(step, pulses, rotation));
    }
  }
  assert(music::hit(0, 1));
  assert(music::tempo_bpm(0) == 65 && music::tempo_bpm(4095) == 150);
  assert(music::density(0) == 1 && music::density(4095) == 7);
  for (uint8_t bounce : {uint8_t(0), uint8_t(64), uint8_t(255)}) {
    const uint32_t even = music::step_samples(1000, 0, bounce);
    const uint32_t odd = music::step_samples(1000, 1, bounce);
    assert(even + odd == 2000);
    assert(even >= 1000 && odd <= 1000);
  }
  assert(music::groove_extra_hits(7, 3, 200, 1u << 5) == (1u << 5));
  assert(music::groove_extra_hits(7, 3, 169, 1u << 5) == 0);
  assert(music::groove_extra_hits(7, 3, 200, 0) == 0);
  assert(music::groove_extra_hits(6, 3, 225, 1u << 6) == (1u << 6));
  for (uint8_t variant = 0; variant < 4; ++variant) {
    uint8_t sounds = 0;
    for (uint8_t step = 0; step < music::kSteps; ++step)
      sounds |= music::fill_mask(variant, step);
    assert((sounds & 0x1f) != 0);       // at least one pentatonic note
    assert((sounds & ((1u << 5) | (1u << 6))) != 0); // a drum
    assert((sounds & (1u << 7)) != 0); // the chime
  }
  for (uint8_t note = 0; note < 5; ++note) {
    assert(music::melody_pitch_index(note, 4) == 5 + note);
    for (uint8_t tone = 0; tone <= 8; ++tone)
      assert(music::melody_pitch_index(note, tone) < 23);
    for (uint32_t random = 0; random < 1000; ++random)
      assert(music::next_melody_note(note, random) < 5);
  }
  uint32_t random = 0xAC56823Du;
  uint8_t note = 2;
  uint8_t heard = 0;
  for (uint16_t i = 0; i < 1000; ++i) {
    random ^= random << 13;
    random ^= random >> 17;
    random ^= random << 5;
    note = music::next_melody_note(note, random);
    heard |= 1u << note;
  }
  assert(heard == 0x1f);
}
