# KeyChainino 144 shared engine

The canonical copy of the engine files every v144 sketch carries:

| File | What |
|---|---|
| `board.h` | pins, matrix size, brightness levels, 8 MHz clock guard |
| `display.h/.cpp` | compiled-scanline charlieplex driver, 4-level greyscale, Timer2 |
| `gfx.h/.cpp` | drawing on the framebuffer (pixels, lines, rects, circles, packed icons, invert) |
| `power.h/.cpp` | power-down sleep, A+B wake |

This is **not** an Arduino library: Arduino only compiles files that sit
in the sketch folder, so each sketch keeps its own copy. Edit here, then:

```sh
./sync.sh          # copy into every sketch, list what changed
./sync.sh --check  # just compare; exit 1 if a sketch is out of date
```

The sketch list lives in `sync.sh`: the multigame, pictures, quest, snake
and bubble sketches next to this folder. (The folder is not called
`library` because `Library/` — the classic KeyChainino library — sits
right beside it, and macOS treats the two names as the same.) `input.*` is deliberately
not shared — each game tunes its own button grammar (the pictures sketch
has a toddler version: chord fires on press, slow capped auto-repeat).

## Changes since the multigame's original copy

- **power**: wakes on pin-change (PCINT18/19) instead of level-triggered
  INT0/INT1. A level interrupt keeps re-firing while its button is held,
  so a keychain squeezed in a pocket spun the CPU at full current; the old
  "wait for release" before sleeping was also a busy loop. Now every edge
  wakes the chip for a few microseconds to look, and it sleeps otherwise.
  Wake still needs both buttons released first (so the hold-to-exit chord
  does not restart the game) and then a chord held ~20 ms (contact bounce
  can't fake one). Sketches' empty `ISR(INT0_vect)`/`ISR(INT1_vect)` are
  now unused but harmless.
- **power, after a datasheet review** (ATmega328P automotive datasheet):
  waits for any EEPROM write still in flight before sleeping (~3.4ms
  write; completion in power-down is not guaranteed), reads the buttons
  with interrupts off so an edge between the read and SLEEP can't be lost,
  and no longer runs the software BOD-disable (BODS) sequence: the
  brown-out detector is off by fuse (efuse 0xFF) from power-on, which is
  the only way to turn it off — BODS merely suspends a fused-on BOD for
  the next SLEEP, and does nothing anywhere else. Timer0/2 stay PRR-gated through sleep (no power effect in power-down,
  but it freezes millis() during the awake checks and in the simulator).
- **display**: `resume()` clears a Timer2 compare flag left pending at
  `park()` (it cut the first slot after wake short); `begin()` clears
  MCUSR before `wdt_disable()`, as the datasheet requires after a
  watchdog reset.
- **gfx**: snake and bubble had the older copy without `invert()`; all
  sketches now share the multigame's (unused functions are dropped by the
  linker, so no flash cost).

Verified: all six sketches compile; the multigame (`simulator/src/test.mjs`)
and quest (`test-quest.mjs`, `test-quest-story.mjs`) simulator suites pass
on hexes rebuilt with this engine.
