/*
 * FLAP — press any button to flap; clear the pipe gaps.
 * Flaps fire on the press edge of the button levels: no release latency,
 * and holding a button can't hover at the ceiling clamp. The bird hovers
 * until the first flap. Vertical position in 1/8-pixel fixed point so the
 * arc feels smooth in 12 pixels of headroom.
 *
 * Pipes are 2 columns wide (the menu icon promises a chunky two-pixel
 * pipe, not a single scrolling dot), so collision/scoring below always
 * treat a pipe as occupying {x-1, x}. Each new pipe's gap random-walks
 * +-2 rows from the previous one — a uniform 1..7 pick allowed impossible
 * 6-row jumps between pipes.
 *
 * Greyscale: pipes dim, bird bright.
 */
#include "display.h"
#include "games.h"
#include "gfx.h"
#include "input.h"

namespace flap {

#define BIRD_X 2
#define GAP_ROWS 4
#define PHYS_MS 110
#define PIPE_START_MS 210
#define PIPE_MIN_MS 130
#define GRAVITY 1  // eighths of a pixel per physics tick
#define FLAP_VY -5
#define MAX_VY 4
#define PIPE_SPACING 7

struct Pipe {
  int8_t x;
  uint8_t gapY;  // gap occupies rows gapY .. gapY+GAP_ROWS-1
};

static Pipe pipes[2];
static int8_t y8, vy;  // bird row * 8, velocity in eighths
static bool started;   // physics only after the first flap
static bool pHeld;     // press-edge gate (also blocks menu-chord holdover)
static uint8_t lastGapY;  // gap random walk: next pipe stays within +-2 rows
static uint16_t pipeMs, physAcc, pipeAcc;

static inline int8_t birdRow() { return y8 >> 3; }

static uint8_t nextGapY() {
  lastGapY = (uint8_t)constrain((int)lastGapY + (int)random(-2, 3), 1,
                                MATRIX_H - GAP_ROWS - 1);
  return lastGapY;
}

static void draw() {
  gfx::clear();
  for (uint8_t p = 0; p < 2; p++) {
    int8_t x = pipes[p].x;
    for (uint8_t r = 0; r < MATRIX_H; r++) {
      if (r < pipes[p].gapY || r >= pipes[p].gapY + GAP_ROWS) {
        gfx::px(x, r, BR_DIM);      // leading edge
        gfx::px(x - 1, r, BR_DIM);  // trailing edge -- gives the pipe girth
      }
    }
  }
  gfx::px(BIRD_X, birdRow(), BR_MAX);
}

static bool crashed() {
  int8_t row = birdRow();
  if (row >= MATRIX_H) return true;  // ground
  for (uint8_t p = 0; p < 2; p++) {
    int8_t dx = pipes[p].x - BIRD_X;
    if ((dx == 0 || dx == 1) &&  // bird column under either pipe pixel
        (row < (int8_t)pipes[p].gapY ||
         row >= (int8_t)(pipes[p].gapY + GAP_ROWS))) {
      return true;
    }
  }
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
  y8 = 5 * 8;
  vy = 0;
  started = false;
  pHeld = true;  // a button still held from the menu chord must not auto-flap
  pipeMs = PIPE_START_MS;
  physAcc = pipeAcc = 0;
  pipes[0].x = 11;
  pipes[0].gapY = random(1, MATRIX_H - GAP_ROWS);
  lastGapY = pipes[0].gapY;
  pipes[1].x = 11 + PIPE_SPACING;
  pipes[1].gapY = nextGapY();
  draw();
}

uint8_t frame() {
  /* Flap on the press edge of the levels: fires on press, and a held
     button's auto-repeat no longer hovers at the ceiling clamp. */
  bool p = input::downA() || input::downB();
  if (p && !pHeld) {
    vy = FLAP_VY;
    started = true;
  }
  pHeld = p;

  if (started) {
    /* The exit chord freezes the world: it must not kill you mid-hold. */
    if (!input::chordHeld()) physAcc += FRAME_MS;
    if (physAcc >= PHYS_MS) {
      physAcc = 0;
      vy += GRAVITY;
      if (vy > MAX_VY) vy = MAX_VY;
      y8 += vy;
      if (y8 < 0) { y8 = 0; vy = 0; }  // soft ceiling
      if (crashed()) return die();
    }

    if (!input::chordHeld()) pipeAcc += FRAME_MS;
    if (pipeAcc >= pipeMs) {
      pipeAcc = 0;
      for (uint8_t p = 0; p < 2; p++) {
        pipes[p].x--;
        if (pipes[p].x == BIRD_X - 1) {  // cleared this pipe
          engine::score++;
          if (pipeMs > PIPE_MIN_MS) pipeMs -= 4;
        }
        if (pipes[p].x < 0) {  // recycle behind the other pipe
          pipes[p].x += 2 * PIPE_SPACING;
          pipes[p].gapY = nextGapY();
        }
      }
      if (crashed()) return die();
    }
  }

  draw();
  return GAME_ON;
}

}  // namespace flap
