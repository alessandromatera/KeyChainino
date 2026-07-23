/*
 * RACER — walls scroll down, steer through the gaps (continuous while
 * held). Every other row is a wall with a 3-wide gap drifting at most one
 * column per wall, so the track is always drivable. Speed ramps forever.
 *
 * Greyscale: walls dim, your car bright. The car is a 1x2 vertical sprite
 * (nose over rear) instead of a single pixel; the nose is the one that
 * meets oncoming walls first, so it's the one that decides a crash (see
 * carHit()). reset() pre-fills 8 rows of track so the round doesn't open
 * on ~3s of empty screen.
 */
#include "display.h"
#include "games.h"
#include "gfx.h"
#include "input.h"

namespace racer {

#define GAP_W 3
#define GAP_MAX_POS (MATRIX_W - GAP_W)
#define STEP_START_MS 280
#define STEP_DEC_MS 3  // was 2: reach the speed floor in a sane time
#define STEP_MIN_MS 100
#define CAR_MS 90
#define CAR_Y (MATRIX_H - 1)      // rear pixel (also where a passed wall exits)
#define CAR_NOSE_Y (CAR_Y - 1)    // nose pixel: first to meet an oncoming wall

static uint16_t track[MATRIX_H];  // bit x = wall pixel
static uint8_t carX;
static int8_t gapPos;   // gap position of the last spawned wall
static bool wallNext;   // alternate wall / empty rows
static uint16_t stepMs, stepAcc, carAcc;

static void draw() {
  gfx::clear();
  for (uint8_t r = 0; r < MATRIX_H; r++) {
    uint16_t m = track[r];
    if (!m) continue;
    for (uint8_t x = 0; x < MATRIX_W; x++) {
      if (m & ((uint16_t)1 << x)) gfx::px(x, r, BR_DIM);
    }
  }
  gfx::px(carX, CAR_NOSE_Y, BR_MAX);
  gfx::px(carX, CAR_Y, BR_MAX);
}

/* PLAYTEST: wall/gap rows strictly alternate (wallNext), so on any given
   tick exactly one of the car's two rows can hold a wall pixel and the
   other is guaranteed clear -- checking BOTH pixels means every tick has
   a live hazard instead of every other one, roughly doubling how often a
   steer can crash you (worst case: a wall sitting only at the rear is
   invisible to a nose check but real to a both-pixel check, so a player
   who steers on nose-only information can clip the rear). Measured this
   in the emulator with a look-ahead bot (reads the gap a few rows out and
   steers early) across 8 seeded runs: both modes crashed at identical
   times every run, so the doubling didn't bite a bot that commits to a
   column early -- but it stays a real hazard for reactive/late steering,
   and nose-only exactly reproduces the original single-pixel game's
   per-tick hazard cadence (same alternation, just checked one row higher)
   instead of adding a new failure mode. Shipping nose-only. */
static inline bool carHit() {
  return (track[CAR_NOSE_Y] & ((uint16_t)1 << carX)) != 0;
}

static void scrollTrack() {
  if (track[CAR_Y]) {  // a wall leaves the bottom: it was passed
    engine::score++;
    if (stepMs > STEP_MIN_MS + STEP_DEC_MS) stepMs -= STEP_DEC_MS;
  }
  for (int8_t r = MATRIX_H - 1; r > 0; r--) track[r] = track[r - 1];
  if (wallNext) {
    gapPos += (int8_t)random(-1, 2);  // drift keeps gaps reachable
    if (gapPos < 0) gapPos = 0;
    if (gapPos > GAP_MAX_POS) gapPos = GAP_MAX_POS;
    track[0] = (uint16_t)(0x0FFF & ~(((1 << GAP_W) - 1) << gapPos));
  } else {
    track[0] = 0;
  }
  wallNext = !wallNext;
}

static uint8_t crash() {
  for (uint8_t i = 0; i < 3; i++) {  // crash flash
    gfx::fillAll(BR_MAX); display::flush(); delay(60);
    gfx::clear(); display::flush(); delay(60);
  }
  return GAME_LOSE;
}

void reset() {
  for (uint8_t r = 0; r < MATRIX_H; r++) track[r] = 0;
  carX = MATRIX_W / 2;
  gapPos = MATRIX_W / 2 - 1;
  wallNext = true;
  stepMs = STEP_START_MS;
  stepAcc = carAcc = 0;
  for (uint8_t i = 0; i < 8; i++) scrollTrack();  // pre-fill the track
  engine::score = 0;  // walls "passed" by the pre-fill don't count
  draw();
}

uint8_t frame() {
  /* steer: continuous while held, frozen during the exit chord */
  carAcc += FRAME_MS;
  if (carAcc >= CAR_MS) {
    bool moved = false;
    if (input::downB() && !input::downA() && carX < MATRIX_W - 1) {
      carX++;
      moved = true;
    } else if (input::downA() && !input::downB() && carX > 0) {
      carX--;
      moved = true;
    }
    if (moved) {
      carAcc = 0;
      if (carHit()) return crash();
    }
  }

  if (!input::chordHeld()) stepAcc += FRAME_MS;  // track frozen during the exit chord
  if (stepAcc >= stepMs) {
    stepAcc = 0;
    scrollTrack();
    if (carHit()) return crash();
  }

  draw();
  return GAME_ON;
}

}  // namespace racer
