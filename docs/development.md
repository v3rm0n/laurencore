# Building Laurencore

The firmware targets the RP2040 in the Pikocore XL. It synthesizes its sounds directly; no sample files are needed.

## Build

Install CMake, an ARM embedded C/C++ toolchain, and the [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk). With the toolchain on your `PATH`, run these commands from the repository root:

```sh
export PICO_SDK_PATH=/path/to/pico-sdk
cmake -S . -B build
cmake --build build -j
```

The output is `build/littlecore.uf2`. The ready-to-install firmware is tracked at [`firmware/littlecore.uf2`](../firmware/littlecore.uf2). The build target and filenames retain the original name, `littlecore`.

## Tests

These tests run on a computer without the device or Pico SDK. Run them from the repository root with a C++17 compiler:

```sh
c++ -std=c++17 -Isrc tests/music_test.cpp src/music.cpp -o /tmp/littlecore-music-test && /tmp/littlecore-music-test
c++ -std=c++17 -Isrc tests/performance_controls_test.cpp src/performance_controls.cpp src/music.cpp -o /tmp/littlecore-controls-test && /tmp/littlecore-controls-test
c++ -std=c++17 -Isrc tests/space_effect_test.cpp src/space_effect.cpp -o /tmp/littlecore-space-test && /tmp/littlecore-space-test
```

## Source files

| File | Purpose |
| --- | --- |
| [`src/main.cpp`](../src/main.cpp) | Hardware input, mode switching, playback timing, sound synthesis, LEDs, and audio output |
| [`src/performance_controls.cpp`](../src/performance_controls.cpp) | Knob gestures, per-sound settings, loop state, and editable patterns |
| [`src/music.cpp`](../src/music.cpp) | Euclidean patterns, swing timing, fills, and melody note selection |
| [`src/space_effect.cpp`](../src/space_effect.cpp) | Tempo-following echo and room reverb |

## Hardware connections

| Function | RP2040 pins |
| --- | --- |
| Buttons 1–8 | GPIO 4–11, active low |
| LEDs 1–8 | GPIO 12–19, active high |
| PWM audio | GPIO 20, through the XL output circuit |
| Control knobs | ADC0–2 on GPIO 26–28 |

The fourth knob is analog volume. If the second and third control knobs are reversed on your build, set `kSwapFunctionKnobs` in `src/main.cpp` to `true`, then rebuild and flash.
