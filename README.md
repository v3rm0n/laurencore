# Littlecore

A simple, self-contained firmware for the Erica Synths Pikocore XL (RP2040). No samples, storage, or computer are needed while playing. It starts silent. Seven lit keys combine repeating sounds; the eighth plays a generative melody or Magic Fill.

## Play

- Keys 1–5: C, D, E, G, A pentatonic notes. Keys 6–7: kick and snare. Tap a key to hear its sound immediately and start or stop its repeating part when you release it. A lit key has an active part. Its LED briefly blinks on each programmed hit.
- Key 8: **Hold** to play a continuing, changing melody from the notes on keys 1–5. It starts immediately, plays a new note about every two steps, and stops choosing notes when released. Pitch and length edits to keys 1–5 also shape the melody. A **quick tap** starts Magic Fill when the tempo is running: a short pentatonic and drum flourish over the loops. Four repeatable fills cycle in order; no loops are erased. Its LED flashes while the melody or fill plays. It works even when all loops are off.
- First control knob (ADC0): speed, 65–150 BPM. At the very bottom it enters **free-play**: the loop parts pause, keys 1–5 become playable notes that sustain while held, 6–7 trigger kick and snare, and holding key 8 still plays the melody. A quick tap of key 8 plays one note in free-play. Turn the knob back up to resume the loops. Hold a sound key while turning it to move that sound lower or higher. Notes stay in the C major pentatonic scale; the kick changes pitch and the snare becomes darker or brighter.
- Second control knob (ADC1): **Bounce**. Turn clockwise for increasing swing; higher settings also add occasional, repeatable drum pickups. Hold a sound key while turning it to edit just that sound's Euclidean rhythm (1–7 evenly spaced hits in eight steps). Every part has its own offset, so combinations vary.
- Third control knob (ADC2): **Space**. Turn clockwise from dry sound through tempo-synced echo to echo plus a short room reverb. Hold a key while turning it to change only that sound's length.
- The fourth, physical volume knob controls output level as wired on the XL. Start it low, especially with headphones or an amplifier.

## Eight-step sequencer

Hold keys **6 and 7 together for one second** to enter or leave the step editor. In the editor, the eight buttons are the eight steps of one sound. Turn ADC1 to choose a sound from keys 1–7, left to right. Lit steps play; press and release a step to switch it on or off. Pressing it also previews the selected sound. The moving LED marks the current step. Editing an unlit sound automatically starts its loop when you add a step.

ADC0 still sets the tempo and ADC2 still sets Space. The bottom stop of ADC0 still pauses loops for free-play, so turn it up to hear the sequence. Bounce holds its last setting while ADC1 selects sounds in the editor. The 6+7 mode gesture does not toggle their loops or steps. When you leave the editor, all edited steps keep playing. Hold a sound key and turn ADC1 in normal mode to generate a fresh Euclidean pattern for just that sound.

In normal mode, hold several keys from 1–7 while turning any control knob to edit those sounds together. Held keys flash. Turning a knob while keys are held does not change that knob's shared function. A tap auditions immediately and toggles the loop on release; a hold-and-turn leaves the loop state as it was. In free-play, tapping does not change stored loops. Tone and length changes audition the edited sounds as you turn.

All loops use eight sixteenth-note steps, so each pattern repeats every two beats. The melody's timing follows the tempo knob, including in free-play. The Magic Fill lasts eight steps. The echo repeats every eighth note and follows the tempo. There is no save function; switching power off clears the loops.

The five notes and the generated melody use a gentle low-pass filter and an eased attack to soften sharp note starts. The fill's chime has a faster attack; kick and snare keep their crisp attack.

## How the synth works

1. **Read the controls.** The main loop debounces the eight buttons and reads ADC0–2. A tap on keys 1–7 auditions the sound and toggles its loop when released. Holding a sound key while turning a knob edits that sound's pitch, rhythm, or length; turning a knob with no sound key held changes tempo, Bounce, or Space. The physical volume knob acts on the output circuit.
2. **Keep an eight-step pattern for each sound.** Keys 1–7 each have an on/off pattern and an enabled state. A rhythm edit with ADC1 generates evenly spaced Euclidean hits for the held sound. In the step editor, button presses change individual steps in that same pattern. The edited pattern keeps playing after leaving the editor; another Euclidean rhythm edit replaces it. Patterns and settings live in RAM and reset on power-off.
3. **Schedule the sounds.** A timer advances through eight sixteenth-note steps at the selected tempo. At each step, enabled sounds play where their pattern has a hit. Bounce shifts alternate step timing and can add occasional drum pickups to sounds still using generated rhythms. Free-play pauses the loops but keeps the patterns. Holding key 8 chooses successive notes from the current pitches and lengths of keys 1–5; a quick tap starts a one-cycle fill when the loops are running.
4. **Make and mix audio.** The pitched keys use C-major-pentatonic oscillator notes; kick and snare use their own synthesized voices. Each hit has an envelope, and pitched voices have a softened attack and low-pass filter. The audio interrupt mixes the voices, applies the tempo-following echo and room effect set by Space, and sends the result to the RP2040 PWM audio output.

For example, tap key 1 to start its C-note loop, hold key 1 and turn ADC1 to choose an evenly spaced rhythm, then enter the step editor with keys 6+7 to add or remove individual hits. Leave the editor with the same gesture; the changed pattern continues playing.

## Build

Install the [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) and an ARM embedded C++ toolchain, then:

```sh
export PICO_SDK_PATH=/path/to/pico-sdk
cmake -S . -B build
cmake --build build -j
```

The verified build artifact is [`firmware/littlecore.uf2`](firmware/littlecore.uf2). Hold BOOTSEL while connecting the RP board by USB and copy that file to its mass-storage drive. A local build also creates `build/littlecore.uf2`. The build is designed for the RP2040 board on the Pikocore XL. Host-only tests run with:

```sh
c++ -std=c++17 -Isrc tests/music_test.cpp src/music.cpp -o /tmp/littlecore-music-test && /tmp/littlecore-music-test
c++ -std=c++17 -Isrc tests/performance_controls_test.cpp src/performance_controls.cpp src/music.cpp -o /tmp/littlecore-controls-test && /tmp/littlecore-controls-test
c++ -std=c++17 -Isrc tests/space_effect_test.cpp src/space_effect.cpp -o /tmp/littlecore-space-test && /tmp/littlecore-space-test
```

## Hardware map

The map follows [the original Pikocore firmware](https://github.com/schollz/pikocore), whose functionality the [XL assembly manual](https://www.ericasynths.lv/media/Pikocore_manual_Kb94E4q.pdf) says the XL shares: switches on GPIO 4–11 (active low), their LEDs on GPIO 12–19 (active high), audio PWM on GPIO 20, controls on ADC0–2 / GPIO 26–28. The fourth knob is analog volume. The audio PWM waveform expects the XL's existing output circuit. No external clock, MIDI, trigger or sample loading is used.

Some PCB V2 boards swap the two function knobs. If the rhythm and Space controls appear exchanged, set `kSwapFunctionKnobs` in `src/main.cpp` to `true`, rebuild and flash. Confirm key order and audio level on your specific assembled unit before giving it to a child; this hardware has no measured hearing-safe limit.

## Restore the previous firmware

A full, verified 16 MB image of the device before this upload is stored locally at `backups/pikocore-original-2026-09-26.bin` (SHA-256 `b2c9b83fc0e2cd278cf3bf5cb127d6dc38c3a7892d7b7519468f5403084c0834`). The `backups/` directory is excluded from Git, so keep a separate copy of this file. To restore it, put the board in BOOTSEL mode and use a USB-capable `picotool`:

```sh
picotool load -v -x backups/pikocore-original-2026-09-26.bin
```
