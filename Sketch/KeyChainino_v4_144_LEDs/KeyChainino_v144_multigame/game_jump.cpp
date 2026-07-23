/*
 * JUMP — endless runner. Press any button to jump the cacti (fixed-point
 * arc). Jumps fire on the press edge of the button levels — no release
 * latency, no hold-to-autojump — and a press mid-air buffers ~150ms and
 * re-fires the instant the runner lands. Obstacles: two 12-bit masks,
 * ground row + the row above for tall ones. Score = obstacles survived.
 *
 * Greyscale: ground line dim, cacti mid, runner bright.
 */
#include "display.h"
#include "games.h"
#include "gfx.h"
#include "input.h"

namespace jump {

#define RUNNER_X 2
#define GROUND_Y 11
#define STAND_Y8 (10 * 8)  // standing row, in eighths
#define PHYS_MS 70
#define OBS_START_MS 180
#define OBS_MIN_MS 90
#define JUMP_VY -7  // a bit more hop: clears the tall cactus with room to
                    // spare instead of by a single row, closer to the menu
                    // icon's mid-air pose
#define MAX_VY 6

static uint16_t lowMask;   // cactus pixels on row 10
static uint16_t tallMask;  // cactus pixels on row 9
static int8_t y8, vy;
static bool airborne;
static bool pHeld;       // press-edge gate (also blocks menu-chord holdover)
static uint8_t bufMs;    // jump buffer: a mid-air press re-fires on landing
static uint8_t gapLeft;  // columns until the next spawn
static uint16_t obsMs, physAcc, obsAcc;

static inline int8_t runnerRow() { return y8 >> 3; }

static void draw() {
  gfx::clear();
  gfx::hline(0, 11, GROUND_Y, BR_DIM);
  for (uint8_t x = 0; x < MATRIX_W; x++) {
    if (lowMask & ((uint16_t)1 << x)) gfx::px(x, 10, BR_MID);
    if (tallMask & ((uint16_t)1 << x)) gfx::px(x, 9, BR_MID);
  }
  gfx::px(RUNNER_X, runnerRow(), BR_MAX);
}

static inline bool crashed() {
  int8_t row = runnerRow();
  if (row == 10 && (lowMask & ((uint16_t)1 << RUNNER_X))) return true;
  if (row == 9 && (tallMask & ((uint16_t)1 << RUNNER_X))) return true;
  return false;
}

static uint8_t die() {
  for (uint8_t i = 0; i < 2; i++) {  // death flash
    gfx::fillAll(BR_MAX); display::flush(); delay(70);
    gfx::clear(); display::flush(); delay(70);
  }
  return GAME_LOSE;
}

void reset() {
  lowMask = tallMask = 0;
  y8 = STAND_Y8;
  vy = 0;
  airborne = false;
  pHeld = true;  // a button still held from the menu chord must not auto-jump
  bufMs = 0;
  gapLeft = 3;  // first cactus shows up quickly (the icon promises a
                // runner already mid-jump over one, not an empty track)
  obsMs = OBS_START_MS;
  physAcc = obsAcc = 0;
  draw();
}

uint8_t frame() {
  /* Jump on the press edge of the levels: no release latency, and holding a
     button can't auto-jump via auto-repeat. A press mid-air buffers ~150ms
     (counted down per frame) so a pre-landing tap is no longer swallowed. */
  bool p = input::downA() || input::downB();
  if (p && !pHeld) {
    if (!airborne) {
      vy = JUMP_VY;
      airborne = true;
    } else {
      bufMs = 150;
    }
  }
  pHeld = p;
  if (bufMs > FRAME_MS) bufMs -= FRAME_MS; else bufMs = 0;

  if (airborne) {
    /* The exit chord freezes the world: it must not kill you mid-hold. */
    if (!input::chordHeld()) physAcc += FRAME_MS;
    if (physAcc >= PHYS_MS) {
      physAcc = 0;
      vy += 1;  // gravity
      if (vy > MAX_VY) vy = MAX_VY;
      y8 += vy;
      if (y8 >= STAND_Y8) {  // landed
        y8 = STAND_Y8;
        vy = 0;
        airborne = false;
        if (bufMs) {  // buffered press: jump again on touchdown
          bufMs = 0;
          vy = JUMP_VY;
          airborne = true;
        }
      }
      if (crashed()) return die();
    }
  }

  if (!input::chordHeld()) obsAcc += FRAME_MS;
  if (obsAcc >= obsMs) {
    obsAcc = 0;
    if (lowMask & 1) {  // an obstacle scrolled off: survived it
      engine::score++;
      if (obsMs > OBS_MIN_MS + 2) obsMs -= 2;
    }
    lowMask >>= 1;
    tallMask >>= 1;
    if (gapLeft == 0) {
      lowMask |= (uint16_t)1 << 11;
      if (random(10) < 3) tallMask |= (uint16_t)1 << 11;  // 30% tall
      /* Breathing room between cacti; min gap +2 near top speed, where
         back-to-back tall cacti were near-impossible. */
      gapLeft = (uint8_t)random(obsMs < 130 ? 7 : 5, 9);
    } else {
      gapLeft--;
    }
    if (crashed()) return die();
  }

  draw();
  return GAME_ON;
}

}  // namespace jump
