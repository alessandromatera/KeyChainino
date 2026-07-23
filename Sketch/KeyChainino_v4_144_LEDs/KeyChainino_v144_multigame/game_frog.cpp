/*
 * FROG — A hops forward, B hops back; cross 9 lanes of
 * traffic.
 * Lanes are 12-bit masks that rotate at per-lane speeds; car patterns in
 * PROGMEM get a random rotation each round. 3 lives, score = crossings.
 *
 * Greyscale: cars dim, the frog bright — a single ALWAYS-LIT pixel (never
 * let it go fully dark, or a blink reads as a dead LED). Danger cue: a
 * car one column away on the frog's row turns it mid — an "about to be
 * hit" the player can actually read.
 */
#include "display.h"
#include "games.h"
#include "gfx.h"
#include "input.h"

namespace frog {

#define FROG_X 5
#define GOAL_ROW 0
#define START_ROW (MATRIX_H - 1)  // home row: car-free by pattern
#define TICK_START_MS 200
#define TICK_STEP_MS 2
#define TICK_MIN_MS 150

/* Car patterns and per-lane speed dividers by row; 0 divider = safe row.
   Odd rows drive left, even rows right.
   Two hand-picked patterns only: WIDE (2 single cars, empty-run 5) and
   MED (2 double-wide cars, empty-run 4). They were tuned by a
   human-reaction probe (~300ms reactions) back when the frog was 2 rows
   tall and almost every hop needed TWO rows clear at once; the frog is a
   single pixel now (one row to clear per hop), so these are deliberately
   generous — crossing is about reading the lanes, not threading a
   needle. Density still ramps (WIDE at the block edges next to a safe
   row, MED in the middle) for visual variety. */
static const uint16_t LANE_PATTERN[MATRIX_H] PROGMEM = {
  0x000,  // 0: goal
  0x041, 0x0C3, 0x041, 0x0C3, 0x041,  // WIDE, MED, WIDE, MED, WIDE
  0x000,  // 6: median
  0x041, 0x0C3, 0x0C3, 0x041,         // WIDE, MED, MED, WIDE
  0x000   // 11: start
};
/* Dividers: WIDE lanes get a touch more speed for visual energy; MED lanes
   move slower since two 2-cell cars leave less instantaneous clearance
   (8/12) than WIDE's two singles (10/12). Even the fastest lane (div 4) at
   the slowest post-ramp tick (TICK_MIN_MS) still shifts only every 600ms --
   well clear of a 300ms human reaction. */
static const uint8_t LANE_DIV[MATRIX_H] PROGMEM = {
  0, 4, 5, 4, 5, 4, 0, 4, 5, 5, 4, 0
};

static uint16_t lane[MATRIX_H];
static uint8_t frogY;
static uint8_t lives;
static uint16_t tickMs, tickAcc;
static uint8_t tickCount;
static bool hopHeld;   // gate: one hop per physical press (kills auto-repeat)

static inline bool carAtFrog(uint8_t row) {
  return (lane[row] & ((uint16_t)1 << FROG_X)) != 0;
}

/* Traffic hits the frog when a car shares its row and column. The home
   row (11) is car-free by pattern, so home base — including retreating
   back to it — is never a death trap. */
static inline bool frogHit() { return carAtFrog(frogY); }

static void draw() {
  gfx::clear();
  for (uint8_t r = 0; r < MATRIX_H; r++) {
    uint16_t m = lane[r];
    if (!m) continue;
    for (uint8_t x = 0; x < MATRIX_W; x++) {
      if (m & ((uint16_t)1 << x)) gfx::px(x, r, BR_DIM);
    }
  }
  /* Danger cue: a car one column over on the frog's row -> mid instead of
     bright (the frog never goes fully dark, see the header). */
  bool danger = (lane[frogY] &
                 (((uint16_t)1 << (FROG_X - 1)) | ((uint16_t)1 << (FROG_X + 1)))) != 0;
  gfx::px(FROG_X, frogY, danger ? BR_MID : BR_MAX);
}

static void frogHome() { frogY = START_ROW; }

static uint8_t squash() {
  for (uint8_t i = 0; i < 3; i++) {  // hit flash
    gfx::fillAll(BR_MAX); display::flush(); delay(60);
    gfx::clear(); display::flush(); delay(60);
  }
  if (--lives == 0) return GAME_LOSE;
  frogHome();
  return GAME_ON;
}

static void crossed() {
  engine::score++;
  if (tickMs > TICK_MIN_MS + TICK_STEP_MS) tickMs -= TICK_STEP_MS;
  for (int8_t r = 11; r >= 0; r--) {  // celebratory sweep
    gfx::hline(0, 11, r, BR_MAX);
    display::flush();
    delay(20);
  }
  gfx::clear();
  display::flush();
  frogHome();
}

void reset() {
  for (uint8_t r = 0; r < MATRIX_H; r++) {
    uint16_t p = pgm_read_word(&LANE_PATTERN[r]);
    uint8_t rot = random(0, 12);  // fresh road, fixed density
    lane[r] = (uint16_t)(((p << rot) | (p >> (12 - rot))) & 0x0FFF);
  }
  lives = 3;
  tickMs = TICK_START_MS;
  tickAcc = 0;
  tickCount = 0;
  hopHeld = false;
  frogHome();
  draw();
}

uint8_t frame() {
  /* One hop per physical press. Draining the tap events every frame plus the
     hopHeld gate makes a held button's auto-repeat do nothing, so holding a
     button can't rocket the frog several lanes into traffic — every hop is a
     deliberate, separate press. Taps on BOTH buttons in the same frame
     cancel: an ambiguous A+B press holds the frog still instead of hopping
     forward and silently losing the back-hop. */
  bool hopFwd = input::tapA();
  bool hopBack = input::tapB();
  input::chordTap();  // drain: a stale chord event must never linger
  if (hopFwd && hopBack) hopFwd = hopBack = false;
  if (!input::downA() && !input::downB()) hopHeld = false;
  if (!hopHeld && hopFwd && frogY > GOAL_ROW) {
    hopHeld = true;
    frogY--;
    if (frogY == GOAL_ROW) {
      crossed();
      draw();
      return GAME_ON;
    }
    if (frogHit()) {
      uint8_t r = squash();
      if (r != GAME_ON) return r;
    }
  } else if (!hopHeld && hopBack && frogY < START_ROW) {
    hopHeld = true;
    frogY++;
    if (frogHit()) {
      uint8_t r = squash();
      if (r != GAME_ON) return r;
    }
  }

  /* Traffic advances on the global tick; each lane on its own divider.
     Frozen while the exit chord is held: it must not kill you mid-hold. */
  if (!input::chordHeld()) tickAcc += FRAME_MS;
  if (tickAcc >= tickMs) {
    tickAcc = 0;
    /* Wrap at lcm(4,5): a free-running uint8 wrap at 256 makes div-5 lanes
       skip/stall one shift every wrap (256 % 5 != 0). */
    if (++tickCount >= 20) tickCount = 0;
    bool moved = false;
    for (uint8_t r = 0; r < MATRIX_H; r++) {
      uint8_t div = pgm_read_byte(&LANE_DIV[r]);
      if (div == 0 || (tickCount % div) != 0) continue;
      uint16_t m = lane[r];
      if (r & 1) {
        m = (uint16_t)(((m >> 1) | (m << 11)) & 0x0FFF);  // odd rows: left
      } else {
        m = (uint16_t)(((m << 1) | (m >> 11)) & 0x0FFF);  // even rows: right
      }
      lane[r] = m;
      moved = true;
    }
    if (moved && frogHit()) {
      uint8_t r = squash();
      if (r != GAME_ON) return r;
    }
  }

  draw();
  return GAME_ON;
}

}  // namespace frog
