# Developing for the KeyChainino 144

Notes for building, flashing and testing firmware for the
[KeyChainino 144](https://www.keychainino.com/product/keychainino-144/).

## Hardware

- ATmega328P @ 8 MHz **internal RC oscillator** (no external crystal).
- 12×12 charlieplexed LED matrix driven over 13 pins: D4–D10, A0–A5.
- 2 buttons: A = D3 (PD3), B = D2 (PD2).
- No buzzer.
- Flash ceiling is **32768 bytes**. The board is programmed over ISP with no
  bootloader, so arduino-cli's reported maximum of 30720 is wrong for it.
  RAM ceiling is 2048 bytes.
- Timer2 is owned by the display driver for LED refresh, so never repurpose
  it. Timer0 = `millis()`. Timer1 is free for game logic.
- Battery: CR2032. The device spends nearly all its life in
  `SLEEP_MODE_PWR_DOWN` and wakes when A and B are pressed together.
- Fuses: lfuse 0xE2, hfuse 0xD7, efuse 0xFF. Brown-out detection is off by
  fuse.

## Sketch layout

All 144 sketches are in `Sketch/KeyChainino_v4_144_LEDs/`. Every sketch
folder is self-contained: everything it needs sits next to the `.ino`, and
there is no library to install.

- **`KeyChainino_v144_*`**: sketches on the shared engine (greyscale
  charlieplex driver, gfx, power-down sleep). The engine files
  (`board.h`, `display.*`, `gfx.*`, `power.*`) are the same in every one of
  these folders: Arduino only compiles files in the sketch folder, so each
  sketch carries its own copy. If you change one of them, copy it into the
  other sketch folders too and recompile them. `input.*` is deliberately
  per-sketch, because every game tunes its own button handling.
- **`KeyChainino_pet`, `KeyChainino_watch`**: built on the original
  KeyChainino driver (`KeyChainino.h/.cpp`), copied into the folder.
- **`test_charlie`**: a bare-metal LED matrix test.

New sketches go in their own `KeyChainino_v144_<name>/` folder, holding
the sketch plus any tools, tests or web pages it needs.

## Building

No Arduino board definition matches the real hardware exactly. The closest
profile is:

```sh
arduino-cli compile --fqbn arduino:avr:pro:cpu=8MHzatmega328 \
  --output-dir build Sketch/KeyChainino_v4_144_LEDs/<sketch>
```

`build/` is git-ignored. `avr-nm` and `avr-size` live under
`~/Library/Arduino15/packages/arduino/tools/avr-gcc/*/bin/` (macOS).

## Flashing

Over ISP with a USBasp, writing the fuses and the flash together:

```sh
avrdude -patmega328p -cusbasp -Pusb \
  -U lfuse:w:0xe2:m -U hfuse:w:0xd7:m -U efuse:w:0xff:m \
  -U flash:w:build/<sketch>.ino.hex:i
```

## Testing without hardware

Drop a compiled `.hex` onto `simulator/index.html`. The page runs the real
firmware in an AVR emulator and draws the LEDs from the actual pin drive,
so what you see is what the board would show. See
[`simulator/README.md`](../simulator/README.md).
