# Laurencore

Simple synth firmware for the **Pikocore XL (RP2040)**, designed for a young child to explore sounds and rhythms. Combine five pentatonic notes and two drums into loops, play them by hand, or make a pattern one step at a time.

[Download firmware](https://github.com/v3rm0n/laurencore/raw/refs/heads/main/firmware/littlecore.uf2) · [Install](#install) · [Build from source](docs/development.md)

## Start playing

1. Set the volume low and turn the first control knob to the middle.
2. Tap **button 6** to start a kick drum loop.
3. Tap **buttons 1 and 4** to add notes. Each sound has its own repeating rhythm.
4. Turn the second control knob to make the rhythm swing. Turn the third to add echo and reverb.
5. Tap a sound button again to stop its loop.

A lit sound button means its loop is on. The synth starts silent. Patterns and sound edits reset when you switch the power off.

## Buttons

In normal play, buttons 1–7 play their sound immediately and switch its loop on or off when released.

| Button | Sound |
| --- | --- |
| 1 | C4 (middle C) |
| 2 | D4 |
| 3 | E4 |
| 4 | G4 |
| 5 | A4 |
| 6 | Kick drum |
| 7 | Snare drum |
| 8 | Hold for a melody; tap for a fill |

**Hold button 8** for a changing melody drawn from the current notes on buttons 1–5. It follows the tempo and uses those sounds' pitch and length settings. Releasing it after a hold stops new notes and lets the last one fade.

**Quickly tap button 8** for a short “Magic Fill”: a flourish of notes and drums. It works even with all loops off, as long as the tempo knob is above its minimum.

## Knobs

Turn a knob normally to change the whole performance. **Hold a sound button (1–7) while turning** to edit that sound instead.

| Control | Turn normally | Hold a sound button and turn |
| --- | --- | --- |
| First knob (ADC0) | Tempo, from 65 to 150 BPM. Fully down enters free play. | Pitch or tone |
| Second knob (ADC1) | **Bounce:** swing, plus occasional extra drum hits at high settings | Rhythm: fewer or more hits |
| Third knob (ADC2) | **Space:** dry sound → echo → echo and reverb | Length: short taps → longer sounds |

The fourth knob controls volume.

Pitch changes keep the five notes in the C major pentatonic scale. For the drums, this control changes kick pitch or snare brightness. The rhythm control spreads 1–7 hits evenly across eight steps; this is a *Euclidean rhythm*.

You can hold several sound buttons to edit them together. After a hold-and-turn edit, releasing the buttons keeps their loops on or off as they were.

## Play by hand

With no sound buttons held, turn the **first knob all the way down**. The loops pause, and buttons 1–5 become notes that sustain while held. Buttons 6 and 7 play drums. Holding button 8 still generates a melody; a quick tap plays a note without starting a fill.

Turn the first knob back up to resume the loops you were playing.

## Edit an eight-step pattern

Use the step editor to place hits exactly where you want them. Each sound has its own eight-step pattern, repeating every two beats.

1. Keep the tempo knob above its minimum. Hold **buttons 6 and 7 together for one second**, then release them. A high chime confirms that the editor is open.
2. Turn the **second knob** to choose a sound. From low to high, the choices follow buttons 1–7: C, D, E, G, A, kick, snare.
3. Buttons **1–8 now represent steps**. Tap a button to turn that step on or off. Each press also previews the chosen sound. Lit steps belong to the pattern; a moving blink shows playback.
4. Choose another sound with the second knob to edit its pattern. Adding a hit starts that sound's loop.
5. Hold **6 and 7 together for one second** again to return to normal play. A lower chime confirms the change. Your edited patterns keep playing.

In the editor, the first knob still controls tempo and the third still controls Space. Bounce keeps its previous setting, but its extra drum hits are omitted for patterns you edit by hand.

To replace an edited pattern with an evenly spaced rhythm, leave the editor, hold that sound's button, and turn the second knob.

## Install

1. [Download `littlecore.uf2`](https://github.com/v3rm0n/laurencore/raw/refs/heads/main/firmware/littlecore.uf2).
2. Hold **BOOTSEL** while connecting the Pikocore XL by USB.
3. Copy the UF2 file to the **RPI-RP2** drive. The device restarts with Laurencore.

For build instructions, tests, and wiring details, see the [developer guide](docs/development.md).
