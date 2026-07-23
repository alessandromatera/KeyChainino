/*
 * LIFE — Conway's Life on a 12x12 torus; more lava lamp than game. Tap
 * any button to reseed; it also reseeds on its own — after a dark beat —
 * when the colony dies, repeats itself (period 1-2, ~2.4s) or keeps a
 * constant population too long (~30s fallback). Leave with the hold chord.
 *
 * Every cell draws at full brightness: the dim-survivors age shading was
 * nice in theory, but on the charlieplex matrix a mostly-dim colony just
 * reads as weak (owner's call).
 */
#include "display.h"
#include "games.h"
#include "gfx.h"
#include "input.h"

namespace life {

#define STEP_MS 150
#define SEED_DENSITY 4   // 1-in-4 cells alive at seed
#define CYCLE_STEPS 16   // period-1/2 repeat this long -> reseed (~2.4s)
#define STALE_STEPS 200  // fallback: constant population this long -> reseed
                         // (~30s; catches period-3+ oscillators the exact
                         // repeat check can't, while gliders get a long ride)

static uint16_t grid[MATRIX_H];
static uint16_t nextg[MATRIX_H];
static uint16_t prev2[MATRIX_H];  // generation before last (period-2 check)
static uint8_t lastPop, staleCount, cycleCount;
static uint16_t stepAcc;

static void draw() {
  gfx::clear();
  for (uint8_t y = 0; y < MATRIX_H; y++) {
    for (uint8_t x = 0; x < MATRIX_W; x++) {
      if (grid[y] & ((uint16_t)1 << x)) gfx::px(x, y, BR_MAX);
    }
  }
}

static void seed() {
  for (uint8_t y = 0; y < MATRIX_H; y++) {
    uint16_t m = 0;
    for (uint8_t x = 0; x < MATRIX_W; x++) {
      if (random(SEED_DENSITY) == 0) m |= (uint16_t)1 << x;
    }
    grid[y] = m;
  }
  staleCount = 0;
  cycleCount = 0;
  lastPop = 0;
  stepAcc = 0;
  draw();
}

/* automatic reseeds get a dark beat, so the swap reads as an event */
static void reseed() {
  gfx::clear();
  display::flush();
  delay(400);
  seed();
}

static void step() {
  uint8_t pop = 0;
  for (uint8_t y = 0; y < MATRIX_H; y++) {
    uint16_t m = 0;
    for (uint8_t x = 0; x < MATRIX_W; x++) {
      uint8_t n = 0;
      for (int8_t dy = -1; dy <= 1; dy++) {
        for (int8_t dx = -1; dx <= 1; dx++) {
          if (dx == 0 && dy == 0) continue;
          uint8_t ny = (uint8_t)((y + dy + MATRIX_H) % MATRIX_H);
          uint8_t nx = (uint8_t)((x + dx + MATRIX_W) % MATRIX_W);
          if (grid[ny] & ((uint16_t)1 << nx)) n++;
        }
      }
      bool alive = grid[y] & ((uint16_t)1 << x);
      if (n == 3 || (alive && n == 2)) {
        m |= (uint16_t)1 << x;
        pop++;
      }
    }
    prev2[y] = nextg[y];  // generation before last, saved before overwrite
    nextg[y] = m;
  }
  /* swap: grid <- next generation, nextg <- previous (for the repeat checks) */
  for (uint8_t y = 0; y < MATRIX_H; y++) {
    uint16_t t = grid[y];
    grid[y] = nextg[y];
    nextg[y] = t;
  }

  if (pop == 0) {
    reseed();
    return;
  }
  /* real stagnation: an exact repeat of the previous generation (still
     life) or of the one before it (period-2 oscillator), CYCLE_STEPS in
     a row. Moving patterns (gliders) never repeat exactly and live on. */
  bool sameAsPrev = true, sameAsPrev2 = true;
  for (uint8_t y = 0; y < MATRIX_H; y++) {
    if (grid[y] != nextg[y]) sameAsPrev = false;
    if (grid[y] != prev2[y]) sameAsPrev2 = false;
  }
  if (sameAsPrev || sameAsPrev2) {
    if (++cycleCount >= CYCLE_STEPS) {
      reseed();
      return;
    }
  } else {
    cycleCount = 0;
  }
  if (pop == lastPop) {  // slow fallback for the rest
    if (++staleCount >= STALE_STEPS) {
      reseed();
      return;
    }
  } else {
    staleCount = 0;
    lastPop = pop;
  }
  draw();
}

void reset() {
  seed();
}

uint8_t frame() {
  if (input::tapA() | input::tapB() | input::chordTap()) {
    seed();
    return GAME_ON;
  }
  stepAcc += FRAME_MS;
  if (stepAcc >= STEP_MS) {
    stepAcc = 0;
    step();
  }
  return GAME_ON;
}

}  // namespace life
