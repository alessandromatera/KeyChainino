/*
 * SNAKE — tap A: turn left, tap B: turn right.
 *
 * Body: ring buffer of packed positions (x high nibble, y low), 1 byte
 * per segment, 48 = winning length. Greyscale: the snake is bright, the
 * food pulses dim<->bright so it never gets lost against the body.
 *
 * Borders wrap (classic Nokia-style): running off any edge re-enters
 * from the opposite edge. Self-collision is the only way to lose.
 */
#include "games.h"
#include "gfx.h"
#include "input.h"

namespace snake {

#define MAX_LEN 48
#define START_LEN 3
#define MOVE_START_MS 260
#define MOVE_SPEEDUP_MS 8
#define MOVE_MIN_MS 130
#define PULSE_MS 150

static uint8_t body[MAX_LEN];
static uint8_t headIdx, len;
static uint8_t heading;  // 0 up, 1 right, 2 down, 3 left
static bool turnLatched; // one turn per move step
static bool awaitRelease; // gate: one turn per physical press (kills auto-repeat)
static uint8_t food;
static bool foodHot;     // pulse phase
static uint16_t moveMs, moveAcc, pulseAcc;

static inline uint8_t pack(uint8_t x, uint8_t y) { return (x << 4) | y; }
static inline uint8_t px_(uint8_t p) { return p >> 4; }
static inline uint8_t py_(uint8_t p) { return p & 0x0F; }

static bool onBody(uint8_t pos) {
  for (uint8_t k = 0; k < len; k++) {
    if (body[(headIdx + MAX_LEN - k) % MAX_LEN] == pos) return true;
  }
  return false;
}

static void placeFood() {
  do {
    food = pack(random(0, MATRIX_W), random(0, MATRIX_H));
  } while (onBody(food));
  foodHot = true;
  gfx::px(px_(food), py_(food), BR_MAX);
}

static uint8_t move() {
  turnLatched = false;

  int8_t x = px_(body[headIdx]);
  int8_t y = py_(body[headIdx]);
  switch (heading) {
    case 0: y--; break;
    case 1: x++; break;
    case 2: y++; break;
    case 3: x--; break;
  }

  /* Wrap at the edges instead of dying — self-collision is the only kill. */
  if (x < 0) x = MATRIX_W - 1;
  else if (x >= MATRIX_W) x = 0;
  if (y < 0) y = MATRIX_H - 1;
  else if (y >= MATRIX_H) y = 0;

  uint8_t newPos = pack(x, y);
  bool growing = (newPos == food);
  uint8_t tailIdx = (headIdx + MAX_LEN - (len - 1)) % MAX_LEN;

  /* Self collision — moving into the vacating tail cell is allowed */
  for (uint8_t k = 0; k < len; k++) {
    uint8_t idx = (headIdx + MAX_LEN - k) % MAX_LEN;
    if (body[idx] == newPos && !(idx == tailIdx && !growing)) {
      return GAME_LOSE;
    }
  }

  if (growing) {
    len++;
    engine::score++;
    if (moveMs > MOVE_MIN_MS + MOVE_SPEEDUP_MS) moveMs -= MOVE_SPEEDUP_MS;
    if (len >= MAX_LEN) return GAME_WIN;
  } else {
    gfx::px(px_(body[tailIdx]), py_(body[tailIdx]), BR_OFF);
  }

  headIdx = (headIdx + 1) % MAX_LEN;
  body[headIdx] = newPos;
  gfx::px(x, y, BR_MAX);

  if (growing) placeFood();
  return GAME_ON;
}

void reset() {
  len = START_LEN;
  headIdx = len - 1;
  for (uint8_t i = 0; i < len; i++) {
    body[i] = pack(4 + i, 6);  // tail (4,6) ... head (6,6)
    gfx::px(4 + i, 6, BR_MAX);
  }
  heading = 1;  // moving right
  turnLatched = awaitRelease = false;
  moveMs = MOVE_START_MS;
  moveAcc = pulseAcc = 0;
  placeFood();
}

uint8_t frame() {
  /* One turn per physical press. Draining the tap events every frame (even
     when gated) plus the awaitRelease gate makes a held button's auto-repeat
     do nothing, so holding a button a beat too long can't spin the snake 180
     into its own neck. turnLatched still caps it to one turn per move step. */
  bool wantL = input::tapA();
  bool wantR = input::tapB();
  if (!input::downA() && !input::downB()) awaitRelease = false;
  if (!turnLatched && !awaitRelease) {
    if (wantL) {
      heading = (heading + 3) & 3;
      turnLatched = awaitRelease = true;
    } else if (wantR) {
      heading = (heading + 1) & 3;
      turnLatched = awaitRelease = true;
    }
  }

  moveAcc += FRAME_MS;
  if (moveAcc >= moveMs) {
    moveAcc = 0;
    uint8_t r = move();
    if (r != GAME_ON) return r;
  }

  pulseAcc += FRAME_MS;
  if (pulseAcc >= PULSE_MS) {
    pulseAcc = 0;
    foodHot = !foodHot;
    gfx::px(px_(food), py_(food), foodHot ? BR_MAX : BR_DIM);
  }

  return GAME_ON;
}

}  // namespace snake
