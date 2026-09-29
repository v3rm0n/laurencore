#include "space_effect.h"
#include <cassert>

int main() {
  // Dry mode preserves the input exactly.
  SpaceEffect dry;
  assert(dry.process(400, 100, 0) == 400);
  assert(dry.process(0, 100, 0) == 0);

  // An eighth-note tap echoes once after the requested sample delay.
  SpaceEffect echo;
  echo.process(400, 100, 128);
  for (int i = 1; i < 100; ++i) assert(echo.process(0, 100, 128) == 0);
  assert(echo.process(0, 100, 128) > 0);

  // The roomy end has a tail before the tempo echo returns.
  SpaceEffect room;
  room.process(400, 3000, 255);
  bool tail = false;
  for (int i = 1; i < 1500; ++i)
    if (room.process(0, 3000, 255) != 0) tail = true;
  assert(tail);

  // With feedback enabled, a single hit dies away without sustaining itself.
  int16_t final_sample = 0;
  for (int i = 0; i < 60000; ++i)
    final_sample = room.process(0, 3000, 255);
  assert(final_sample == 0);

  // Percussion retains its level and produces no tail at any Space setting.
  const uint8_t amounts[] = {0, 128, 255};
  for (uint8_t amount : amounts) {
    SpaceEffect percussion;
    assert(percussion.process(0, 100, amount, 400) == 400);
    for (int i = 0; i < 15000; ++i)
      assert(percussion.process(0, 100, amount, 0) == 0);
  }

  // Adding drums leaves the pitched effect and its tail intact.
  SpaceEffect pitched;
  SpaceEffect combined;
  for (int i = 0; i < 15000; ++i) {
    const int16_t note = i == 0 ? 160 : 0;
    const int16_t drum = i % 17 == 0 ? 60 : -30;
    const int16_t wet_note = pitched.process(note, 100, 255);
    assert(combined.process(note, 100, 255, drum) == wet_note + drum);
  }
  // Limit the final sum, including the dry percussion.
  assert(combined.process(400, 100, 0, 400) == 500);
  assert(combined.process(-400, 100, 0, -400) == -500);
}
