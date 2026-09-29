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
}
