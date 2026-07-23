/*
 * TENNIS — hold/tap A/B to move the bottom paddle; the AI up top tracks
 * the ball and sharpens as you score. Point when the AI misses, life lost
 * when you miss; 3 lives.
 *
 * Greyscale: your paddle and the ball are bright, the AI paddle is dim —
 * you always know instantly which side is yours.
 */
#include "display.h"
#include "games.h"
#include "gfx.h"
#include "input.h"

namespace tennis {

#define BALL_START_MS 190
#define BALL_MIN_MS 90
#define BALL_STEP_MS 6
#define PAD_MS 60
#define AI_START_MS 170
#define AI_MIN_MS 80
#define AI_STEP_MS 10
#define PLAYER_Y (MATRIX_H - 1)
#define AI_Y 0
#define AI_REACT_Y 9  // AI ignores the ball only while it's deep in your half

static int8_t ballX, ballY, dirX, dirY;
static uint8_t playerX, aiX;  // left pixel of each 2px paddle
static uint8_t lives;
static uint16_t ballMs, aiMs;
static uint16_t ballAcc, padAcc, aiAcc;
static bool serveUp;

static void draw() {
  gfx::clear();
  gfx::px(playerX, PLAYER_Y, BR_MAX);
  gfx::px(playerX + 1, PLAYER_Y, BR_MAX);
  gfx::px(aiX, AI_Y, BR_DIM);
  gfx::px(aiX + 1, AI_Y, BR_DIM);
  gfx::px(ballX, ballY, BR_MAX);
}

static void serve() {
  ballX = random(3, 9);
  ballY = 6;
  dirX = (int8_t)random(-1, 2);
  dirY = serveUp ? -1 : 1;
  serveUp = !serveUp;
  draw();
  display::flush();
  delay(400);  // short breather so the serve is readable
  ballAcc = 0;
}

/* Where will the ball cross the AI's row? Replays the same wall-bounce
   rule moveBall() uses (reflect at x=0 / x=MATRIX_W-1) instead of just
   chasing the ball's current x, so the AI gets there ahead of time
   instead of arriving late. Also counts wall bounces on the way: a
   trajectory that bounces is a harder read, which frame() uses to give
   the AI an occasional plausible misjudgement instead of a perfect one. */
static int8_t predictBallX(uint8_t* bounces) {
  int8_t x = ballX;
  int8_t dx = dirX;
  int8_t y = ballY;
  uint8_t n = 0;
  while (y > AI_Y) {
    x += dx;
    if (x < 0) { x = 0; dx = 1; n++; }
    else if (x >= MATRIX_W) { x = MATRIX_W - 1; dx = -1; n++; }
    y += dirY;
  }
  *bounces = n;
  return x;
}

/* Which paddle pixel would catch a ball arriving at column bx: 0 = left,
   1 = right, -1 = none. Which pixel you catch with picks the return
   angle. */
static int8_t paddleCatch(uint8_t padX, int8_t bx) {
  if (bx == (int8_t)padX) return 0;
  if (bx == (int8_t)padX + 1) return 1;
  return -1;
}

/* On a miss the ball visibly lands on the back row and blinks. */
static void missFlash(int8_t y) {
  ballY = y;
  draw();
  display::flush();
  for (uint8_t i = 0; i < 3; i++) {
    gfx::px(ballX, y, BR_OFF);
    display::flush();
    delay(90);
    gfx::px(ballX, y, BR_MAX);
    display::flush();
    delay(90);
  }
}

static uint8_t moveBall() {
  int8_t nx = ballX + dirX;
  if (nx < 0) { nx = 0; dirX = 1; }
  else if (nx >= MATRIX_W) { nx = MATRIX_W - 1; dirX = -1; }

  int8_t ny = ballY + dirY;

  /* Paddles, same scheme as BRICKS: the diagonal arrival column (nx) is
     what gets tested, and a catch bounces off the row NEXT to the
     paddle — the ball never enters the paddle row, so it never vanishes
     into the bar. The rally speeds up a touch on every return. */
  if (dirY > 0 && ny >= PLAYER_Y) {  // reaching the player's row
    int8_t pix = paddleCatch(playerX, nx);
    if (pix >= 0) {
      dirX = pix ? 1 : -1;
      dirY = -1;
      ny = PLAYER_Y - 1;  // rebound visible, one row above the paddle
      if (ballMs > BALL_MIN_MS) ballMs -= BALL_STEP_MS;
    } else {
      ballX = nx;  // lands past the paddle, on the back row
      missFlash(PLAYER_Y);
      if (--lives == 0) return GAME_LOSE;
      engine::scrollNumP(PSTR("LIVES "), lives);
      serveUp = true;  // you just conceded: the next serve goes at the AI
      serve();
      return GAME_ON;
    }
  } else if (dirY < 0 && ny <= AI_Y) {  // reaching the AI's row, mirrored
    int8_t pix = paddleCatch(aiX, nx);
    if (pix >= 0) {
      dirX = pix ? 1 : -1;
      dirY = 1;
      ny = AI_Y + 1;
      if (ballMs > BALL_MIN_MS) ballMs -= BALL_STEP_MS;
    } else {
      ballX = nx;
      missFlash(AI_Y);
      if (engine::score < 999) engine::score++;
      if (aiMs > AI_MIN_MS) aiMs -= AI_STEP_MS;  // AI sharpens
      serve();
      return GAME_ON;
    }
  }

  ballX = nx;
  ballY = ny;
  return GAME_ON;
}

void reset() {
  playerX = aiX = 5;
  lives = 3;
  ballMs = BALL_START_MS;
  aiMs = AI_START_MS;
  serveUp = true;
  padAcc = aiAcc = 0;
  serve();
}

uint8_t frame() {
  /* Player paddle: continuous while held; frozen during an A+B chord */
  padAcc += FRAME_MS;
  if (padAcc >= PAD_MS) {
    if (input::downB() && !input::downA() && playerX < MATRIX_W - 2) {
      playerX++;
      padAcc = 0;
    } else if (input::downA() && !input::downB() && playerX > 0) {
      playerX--;
      padAcc = 0;
    }
  }

  /* AI: predicts where an incoming ball will cross its row (bounces off
     the side walls included) instead of just chasing the ball's current
     x, so once it commits it moves straight toward the right spot
     instead of trailing a moving target one pixel behind. It reacts as
     soon as the ball heads its way (only your last two rows are ignored)
     and hesitates less than it used to — beatable, but no longer a
     pushover: a hard-to-read bounced shot can still be misjudged by a
     pixel, and aiMs drops every time it concedes, so it tightens as you
     score. */
  if (!input::chordHeld()) aiAcc += FRAME_MS;  // world freezes during the exit chord
  if (aiAcc >= aiMs) {
    aiAcc = 0;
    int8_t target;
    if (dirY < 0 && ballY <= AI_REACT_Y) {
      uint8_t bounces;
      target = predictBallX(&bounces);
      if (bounces && random(5) == 0) target += random(2) ? 1 : -1;
    } else {
      target = MATRIX_W / 2 - 1;  // ball not committed yet: drift back to centre
    }
    uint8_t hesitateOutOf = 7 + (AI_START_MS - aiMs) / 22;  // 7 (fresh) .. 11 (ramped)
    if ((uint8_t)random(hesitateOutOf) != 0) {
      if (target < (int8_t)aiX && aiX > 0) aiX--;
      else if (target > (int8_t)aiX + 1 && aiX < MATRIX_W - 2) aiX++;
    }
  }

  if (!input::chordHeld()) ballAcc += FRAME_MS;  // ...so the gesture can't kill you
  if (ballAcc >= ballMs) {
    ballAcc = 0;
    uint8_t r = moveBall();
    if (r != GAME_ON) return r;
  }

  draw();
  return GAME_ON;
}

}  // namespace tennis
