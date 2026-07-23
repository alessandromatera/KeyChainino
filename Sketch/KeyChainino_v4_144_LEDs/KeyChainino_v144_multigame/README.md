# KeyChainino 144 Multi-Game — Rethought Edition

The same seventeen games (fifteen, plus Muncher and Steroids) and the
same two-button language as
`KeyChainino_v144_multigame`, rebuilt from scratch: new display driver,
new text engine, new game framework. No library to install — everything
is in this folder.

## What was rethought

### 1. Charlieplexing driver with greyscale (`display.*`)

The classic driver called `pinMode()`/`digitalWrite()` ~26 times per
Timer2 interrupt and kept two byte-per-LED arrays in RAM (313 bytes).

The new driver splits the work differently:

* **`display::flush()` compiles** the framebuffer into ready-made
  DDR/PORT register images, one per (scanline, brightness-plane). The
  scan table is **double-buffered**: flush builds the back table and
  flips an index, so the ISR never displays a half-built frame.
* **The ISR only writes registers** — blank the matrix, load 6 bytes,
  set the next compare value. A few microseconds, fixed cost.
* **Greyscale** by binary code modulation: each scanline is shown twice
  per frame. Because charlieplexing shares the anode pin's current
  between all lit LEDs of a line (absolute brightness already varies
  with line load — the classic firmware too), the two planes are
  weighted **1:7**, giving three honest levels: off, dim (~1/8 duty)
  and bright. Dim reads unmistakably dimmer than bright on every line,
  whatever the load.
* Line timing is **cycle-hardcoded like the classic driver** (6144 CPU
  cycles per scanline → ~100 Hz at a true 8 MHz), so the refresh matches
  the proven classic firmware on the real board even if the build's
  clock setting doesn't match the fuses.
* **Anti-ghosting**: a line is energized only in planes where it has lit
  pixels, and between slots every matrix line is clamped low for an
  instant to dump the charge parked on floating pins — otherwise that
  charge leaks through neighbour LEDs and shows up as phantom "double
  LEDs" next to dim pixels. Dim pixels are still best used as *scenery*
  (many together); a single isolated dim pixel remains the worst case on
  charlieplexed hardware.

The games use it everywhere: scenery (bricks, pipes, cars, walls, the
settled Blocks stack) is dim, the player and the action are bright.
Echo shows dim pad guides on your turn.

### 2. Zero-heap text engine (`text.*`, `font_data.h`)

The classic scroller `malloc()`ed a rendered buffer of the whole string
and `free()`d it *inside the Timer2 ISR*. The new one keeps three
pointers and streams glyph columns straight from PROGMEM as it scrolls:

* no allocation anywhere in the firmware, no length limit;
* PROGMEM strings scroll directly — no RAM staging copies;
* two-segment messages ("SCORE " from flash + digits from a 6-byte
  buffer) need no string assembly;
* every blocking scroll is **skippable with any button**.

### 3. A real game engine (`engine.*`)

A game used to be four entry points plus `done`/`over` flags plus
`millis()` bookkeeping plus its own game-over/score-scroll code. Now a
game is **two functions**:

```cpp
void reset();      // fresh round, screen already cleared
uint8_t frame();   // ONE fixed 20ms step; GAME_ON / GAME_LOSE / GAME_WIN
```

The engine owns the fixed 50 Hz timestep, the intro/outro faces, the
score scroll, the EEPROM records ("RECORD!"), display flushing and the
universal exit chord. Games put their score in `engine::score` and pace
sub-speeds with millisecond accumulators (`acc += FRAME_MS`), so all the
original tuning constants carried over unchanged.

Nobody uses Timer1 anymore (the Bricks ball is frame-paced with the
same ms curve as its old hardware ticks), so Timer1 is powered off for
good, along with ADC/SPI/UART/TWI.

### 4. Everything constant lives in flash

* The game registry table itself is in PROGMEM — a game entry costs zero
  RAM (the classic table lived in RAM).
* Icons are packed 12-word bitmaps (24 bytes instead of 144 — the same
  drawings, 6x smaller), drawn via `gfx::drawIconP()`.
* Names, score labels, fonts, tetrominoes, lane patterns: PROGMEM.

### 5. Same player-facing behavior

Menu (A/B browse, chord-tap launch, name scroll after settling, 25 s
idle → power-down, A+B wakes), hold A+B ~1.2 s to exit any game, EEPROM
high score per game, last game remembered.

## V1.1 — the playability pass

Same machine, same button language; every change below makes the
existing controls behave more fairly or read more clearly.

### Engine-wide

* **The world freezes while the exit chord is held** (new
  `input::chordHeld()` level). Every free-running hazard — balls,
  traffic, tracks, rocks, formations, the ghost, gravity itself — pauses
  during the 1.2 s A+B hold, which used to keep killing players
  mid-gesture.
* New `gfx::invert()` (DIM↔MAX swap) for hit/death feedback that keeps
  the scenery on screen.

### Controls

* **Press-edge controls** in JUMP and FLAP: the action fires on *press*,
  not release, and holding a button no longer auto-jumps or hovers at
  the ceiling via auto-repeat. JUMP also buffers a press made just
  before landing (~150 ms).
* **One action per physical press** where auto-repeat cheated or
  double-fired: TOWER locks (arm-on-release replaces the 250 ms guard —
  no more hold-to-autolock and no more swallowed taps), MUNCHER turns,
  ECHO sequence entries, WHACK pops (a press must start after the pop —
  anticipation holds no longer strike).

### Fairness

* TENNIS paddle contact is tested on the column the ball was seen in and
  the ball lands IN the paddle row for a frame — a clean readable catch
  instead of rebounding off thin air one row above and beside it. The
  ball also runs faster (190ms start, 90ms floor).
* MUNCHER spawned facing a wall in **all three** mazes — now spawns at
  (0,0) heading down the always-open left column.
* INVADERS shots spawned inside low formations and tunneled through —
  fixed; kills also reset the fire timer and the endgame speed floor
  rose 100→180 ms, so the last alien is aimable.
* Speed floors respect human reaction time (STEROIDS 110→150 ms;
  BRICKS' paddle 70→60 ms now outruns the fastest ball).
* FLAP gaps random-walk (±2) instead of uniform — no impossible 6-row
  jumps; JUMP's minimum cactus gap grows +2 near top speed; RACER ramps
  faster and pre-fills its track (no 3 s dead open).
* LIFE detects real stagnation (exact period-1/2 repeats, ~2.4 s)
  instead of the constant-population proxy that killed crawling gliders.

### Feedback

* Flashes where events were silent: catches (CATCH), kills (STEROIDS),
  cuts and misses (TOWER), deaths (JUMP, FLAP), the correct pad on a
  loss (ECHO), the settle blink on a dice result.
* Status you can finally see: TENNIS lives scroll, INVADERS wave number,
  CATCH miss pips, WHACK strike pips.
* Danger telegraphs in BR_MID: the frog about to be run over (FROG), a
  formation one step from lethal (INVADERS), single-pixel rocks
  (STEROIDS), the mole about to burrow (WHACK).
* DICE polls the buttons mid-tumble: a tap during a roll queues an
  instant re-roll, and the exit chord responds immediately.

## Layout

| File | Role |
| --- | --- |
| `*.ino` | boot, wake-sleep cycle, menu/game top loop |
| `board.h` | pins, matrix size, brightness constants |
| `display.*` | charlieplex scan, greyscale, Timer2 ISR |
| `gfx.*` | pixels, lines, rects, circle, packed icons |
| `text.*` + `font_data.h` | streaming scroller + glyphs |
| `input.*` | taps, chord tap, exit hold, auto-repeat |
| `engine.*` | fixed timestep, round lifecycle, score flow |
| `menu.*`, `icons.*`, `save.*`, `power.*` | picker, bitmaps, EEPROM, sleep |
| `games.h`, `registry.cpp` | game entry points and the PROGMEM table |
| `game_*.cpp` | one file per game (17) |

## Building

Arduino IDE with MiniCore (`MiniCore:avr:328`), exactly like the classic
multigame sketch. Open the `.ino`, select the board, upload. Adding a
game: write `game_foo.cpp` (two functions), declare it in `games.h`,
append a row in `registry.cpp` — done.
