/*
 * BRICKS — A/B move the paddle; clear the 4-row wall,
 * 3 lives.
 *
 * The original ran the ball from a Timer1 ISR (79.5us hardware ticks,
 * counter 1000 -> ~80ms per ball step, floor ~48ms). Rethought, the ball
 * is paced from the frame loop on the same curve, tuned ~25% slower
 * (100ms start, 60ms floor) after playtesting called the original too
 * fast — no interrupt, no volatile/atomic dance, and Timer1 stays off.
 *
 * Greyscale: bricks dim, ball and paddle bright.
 */
#include "display.h"
#include "games.h"
#include "gfx.h"
#include "input.h"

namespace bricks {

#define WALL_H 4
#define TOTAL_BRICKS (MATRIX_W * WALL_H)
#define BALL_START_MS 100
#define BALL_MIN_MS 60
#define BALL_STEP_MS 3  // every 5 bricks
#define BAR_MS 60  // paddle steps faster than the late ball (~73ms)
#define BAR_Y (MATRIX_H - 1)

static uint16_t wall[WALL_H];  // bit x = brick alive
static uint8_t barX;           // left pixel of the 2px paddle
static int8_t ballX, ballY, dirX, dirY;
static uint8_t lives;
static uint16_t ballMs, ballAcc, barAcc;
static bool won;

static bool isBrick(int8_t x, int8_t y) {
  if (x < 0 || x >= MATRIX_W || y < 0 || y >= WALL_H) return false;
  return (wall[y] & ((uint16_t)1 << x)) != 0;
}

static bool hitBrick(int8_t x, int8_t y) {
  if (!isBrick(x, y)) return false;
  wall[y] &= ~((uint16_t)1 << x);
  engine::score++;
  if ((engine::score % 5) == 0 && ballMs > BALL_MIN_MS) {
    ballMs -= BALL_STEP_MS;
  }
  if (engine::score >= TOTAL_BRICKS) won = true;
  return true;
}

static void draw() {
  gfx::clear();
  for (uint8_t y = 0; y < WALL_H; y++) {
    for (uint8_t x = 0; x < MATRIX_W; x++) {
      if (wall[y] & ((uint16_t)1 << x)) gfx::px(x, y, BR_DIM);
    }
  }
  gfx::px(barX, BAR_Y, BR_MAX);
  gfx::px(barX + 1, BAR_Y, BR_MAX);
  gfx::px(ballX, ballY, BR_MAX);
}

static void serve() {
  ballY = MATRIX_H - 2;
  ballX = (int8_t)random(barX, barX + 2);
  dirY = -1;
  dirX = random(2) ? 1 : -1;  // slanted serve: dead-vertical rallies are dull
  ballAcc = 0;
}

/* One ball step: brick bounces resolved by axis (no tunneling), then
   walls, paddle, floor. Returns GAME_ON / GAME_LOSE / GAME_WIN. */
static uint8_t moveBall() {
  bool hitV = false, hitH = false;

  if (dirY != 0 && hitBrick(ballX, ballY + dirY)) {
    dirY = -dirY;
    hitV = true;
  }
  if (dirX != 0 && hitBrick(ballX + dirX, ballY)) {
    dirX = -dirX;
    hitH = true;
  }
  if (!hitV && !hitH && dirY != 0 && dirX != 0 &&
      hitBrick(ballX + dirX, ballY + dirY)) {
    dirY = -dirY;
    dirX = -dirX;
  }

  if (won) return GAME_WIN;

  int8_t ny = ballY + dirY;
  int8_t nx = ballX + dirX;

  if (nx < 0) { nx = 0; dirX = 1; }
  else if (nx >= MATRIX_W) { nx = MATRIX_W - 1; dirX = -1; }
  if (ny < 0) { ny = 0; dirY = 1; }

  /* Paddle: which pixel you catch with picks the return angle. */
  if (dirY > 0 && ny >= BAR_Y) {
    if (nx == (int8_t)barX || nx == (int8_t)barX + 1) {
      ny = BAR_Y - 1;
      dirY = -1;
      dirX = (nx == (int8_t)barX) ? -1 : 1;
    } else {
      /* Missed: flash the lost ball on the floor row */
      ballX = nx;
      ballY = BAR_Y;
      draw();
      display::flush();
      for (uint8_t i = 0; i < 3; i++) {
        gfx::px(nx, BAR_Y, BR_OFF); display::flush(); delay(80);
        gfx::px(nx, BAR_Y, BR_MAX); display::flush(); delay(80);
      }
      if (--lives == 0) return GAME_LOSE;
      engine::scrollNumP(PSTR("LIVES "), lives);
      serve();
      return GAME_ON;
    }
  }

  ballX = nx;
  ballY = ny;
  return GAME_ON;
}

void reset() {
  for (uint8_t y = 0; y < WALL_H; y++) wall[y] = 0x0FFF;
  barX = 5;
  lives = 3;
  ballMs = BALL_START_MS;
  barAcc = 0;
  won = false;
  serve();
  draw();
}

uint8_t frame() {
  /* Paddle: continuous while exactly one button is held (chord freezes
     it, so holding the exit chord never drifts the bar). */
  barAcc += FRAME_MS;
  if (barAcc >= BAR_MS) {
    if (input::downB() && !input::downA() && barX < MATRIX_W - 2) {
      barX++;
      barAcc = 0;
    } else if (input::downA() && !input::downB() && barX > 0) {
      barX--;
      barAcc = 0;
    }
  }

  /* The ball freezes too while the exit chord is held: the A+B gesture
     mustn't kill you mid-exit. */
  if (!input::chordHeld()) ballAcc += FRAME_MS;
  if (ballAcc >= ballMs) {
    ballAcc = 0;
    uint8_t r = moveBall();
    if (r == GAME_WIN) {
      /* brief fireworks, then the engine takes over the ceremony */
      for (uint8_t i = 0; i < 6; i++) {
        uint8_t x = random(2, 10), y = random(2, 10);
        for (uint8_t rad = 1; rad < 7; rad++) {
          gfx::clear();
          gfx::circle(x, y, rad, BR_MAX);
          gfx::circle(x, y, rad - 1, BR_DIM);
          display::flush();
          delay(30);
        }
      }
      return GAME_WIN;
    }
    if (r != GAME_ON) return r;
  }

  draw();
  return GAME_ON;
}

}  // namespace bricks
