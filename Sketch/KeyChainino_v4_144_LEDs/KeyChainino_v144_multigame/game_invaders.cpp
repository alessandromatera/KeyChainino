/*
 * INVADERS — A/B move the ship, that's it: the cannon auto-fires on its
 * own timer (2 shots in the air, max) so both thumbs stay free to strafe.
 * The fire timer works like Steroids': primed from frame one, and when
 * both shot slots are busy it stays primed and looses the instant one
 * frees. Playtest feedback on the old chord-tap-to-fire scheme was that
 * landing a clean simultaneous A+B tap while also holding a strafe
 * direction was too fiddly for a keychain toy; auto-fire removes the
 * coordination problem entirely. A+B held (~1.2s) still exits to the
 * menu — that's the engine's universal gesture and doesn't touch this
 * file at all.
 *
 * A 4x3 formation (one alive-mask + an offset) marches side to side and
 * steps down at the edges; endless waves, each a little faster. You lose
 * only if the formation reaches your row.
 *
 * Greyscale: aliens dim — mid when the lowest alive row is one step from
 * yours (the danger telegraph) — your ship and shots bright. The ship is
 * a 2-row sprite — a single nub over a 3-wide base (same idiom as
 * Steroids' ship), so it reads as a ship instead of a lone "missing"
 * pixel; shots spawn from the nub's tip.
 */
#include "display.h"
#include "games.h"
#include "gfx.h"
#include "input.h"

namespace invaders {

#define FORM_COLS 4
#define FORM_ROWS 3
#define FORM_XMAX 5  // formation is 7px wide
#define SHIP_Y (MATRIX_H - 1)     // base row (bottom of the matrix)
#define SHIP_NUB_Y (SHIP_Y - 1)   // nub row: the ship's topmost pixel
#define SHIP_HALF_W 1             // base spans shipX-1..shipX+1 (3px)
#define MAX_SHOTS 2
#define SHOT_NONE 0xFF
#define SHOT_MS 70   // steroids-fast climb: snappy, not floaty
#define SHIP_MS 110
#define FIRE_MS 340  // auto-fire period; MAX_SHOTS is the real cap on density
#define STEP_BASE_MS 120
#define STEP_PER_ALIEN_MS 45
#define WAVE_BONUS_MS 20

static uint16_t alive;  // bit (r*4+c) = alien alive
static int8_t fx, fy;   // formation offset (top-left alien pixel)
static int8_t dir;
static uint8_t shipX;
static uint8_t shots[MAX_SHOTS];  // packed x<<4|y
static uint16_t stepAcc, shotAcc, shipAcc, fireAcc;
static uint8_t wave;

static uint8_t aliveCount() {
  uint8_t n = 0;
  for (uint8_t i = 0; i < FORM_COLS * FORM_ROWS; i++) {
    if (alive & ((uint16_t)1 << i)) n++;
  }
  return n;
}

static uint16_t stepInterval() {
  uint16_t ms = STEP_BASE_MS + STEP_PER_ALIEN_MS * aliveCount();
  uint16_t bonus = WAVE_BONUS_MS * wave;
  return (ms > 180 + bonus) ? ms - bonus : 180;  // floor: endgame stays aimable
}

static int8_t lowestAliveRow() {
  for (int8_t r = FORM_ROWS - 1; r >= 0; r--) {
    for (uint8_t c = 0; c < FORM_COLS; c++) {
      if (alive & ((uint16_t)1 << (r * FORM_COLS + c))) return r;
    }
  }
  return -1;
}

static void draw() {
  gfx::clear();
  /* Danger telegraph: the lowest alive row one down-step from lethal
     (same test stepFormation() loses on) glows mid instead of dim. */
  int8_t low = lowestAliveRow();
  bool danger = (low >= 0 && fy + 2 * low >= SHIP_NUB_Y - 1);
  for (uint8_t r = 0; r < FORM_ROWS; r++) {
    for (uint8_t c = 0; c < FORM_COLS; c++) {
      if (alive & ((uint16_t)1 << (r * FORM_COLS + c))) {
        int8_t y = fy + 2 * r;
        if (y >= 0) gfx::px(fx + 2 * c, y, (danger && r == low) ? BR_MID : BR_DIM);
      }
    }
  }
  for (uint8_t s = 0; s < MAX_SHOTS; s++) {
    if (shots[s] != SHOT_NONE) {
      gfx::px(shots[s] >> 4, shots[s] & 0x0F, BR_MAX);
    }
  }
  gfx::px(shipX, SHIP_NUB_Y, BR_MAX);  // nub
  for (int8_t dx = -SHIP_HALF_W; dx <= SHIP_HALF_W; dx++) {
    gfx::px(shipX + dx, SHIP_Y, BR_MAX);  // 3-wide base
  }
}

static void newWave() {
  alive = (uint16_t)((1UL << (FORM_COLS * FORM_ROWS)) - 1);
  fx = 2;
  fy = 0;
  dir = 1;
  stepAcc = 0;
}

/* Shot at (sx,sy): hits a living alien? They sit on even offsets. */
static bool tryHit(int8_t sx, int8_t sy) {
  int8_t relX = sx - fx;
  int8_t relY = sy - fy;
  if (relX < 0 || relX > 2 * (FORM_COLS - 1) || (relX & 1)) return false;
  if (relY < 0 || relY > 2 * (FORM_ROWS - 1) || (relY & 1)) return false;
  uint8_t bit = (relY / 2) * FORM_COLS + (relX / 2);
  if (!(alive & ((uint16_t)1 << bit))) return false;

  alive &= ~((uint16_t)1 << bit);
  engine::score++;
  fireAcc = 0;  // a kill reloads the cannon: endgame aim beats fire-timer luck
  if (alive == 0) {
    wave++;
    gfx::fillAll(BR_MAX);  // wave-cleared flash
    display::flush();
    delay(120);
    engine::scrollNumP(PSTR("WAVE "), wave + 1);  // the wave about to start
    newWave();
  }
  return true;
}

static bool stepFormation() {  // false = formation reached the ship
  int8_t nx = fx + dir;
  if (nx < 0 || nx > FORM_XMAX) {
    dir = -dir;
    fy++;
    int8_t lowest = lowestAliveRow();
    /* Ship's topmost pixel is the nub (row SHIP_NUB_Y) now, one row above
       the old single-pixel ship's row, so that's the row that ends it. */
    if (lowest >= 0 && fy + 2 * lowest >= SHIP_NUB_Y) return false;
  } else {
    fx = nx;
  }
  return true;
}

void reset() {
  shipX = MATRIX_W / 2;
  for (uint8_t s = 0; s < MAX_SHOTS; s++) shots[s] = SHOT_NONE;
  wave = 0;
  shotAcc = shipAcc = 0;
  fireAcc = FIRE_MS;  // primed: first shot goes up on frame one, so the
                      // ship reads as "always shooting" from the start
  newWave();
  draw();
}

uint8_t frame() {
  /* Ship: continuous movement while held */
  shipAcc += FRAME_MS;
  if (shipAcc >= SHIP_MS) {
    /* Clamp keeps the whole 3-wide base on the matrix, not just the nub. */
    if (input::downB() && !input::downA() && shipX < MATRIX_W - 1 - SHIP_HALF_W) {
      shipX++;
      shipAcc = 0;
    } else if (input::downA() && !input::downB() && shipX > SHIP_HALF_W) {
      shipX--;
      shipAcc = 0;
    }
  }

  /* Auto-fire, from the tip of the nub: no button for this, so strafing
     never competes with shooting. Slots full? The timer stays primed
     (clamped, never overflows) and looses the next shot the instant one
     frees — same fire feel as Steroids. */
  fireAcc += FRAME_MS;
  if (fireAcc >= FIRE_MS) {
    for (uint8_t s = 0; s < MAX_SHOTS; s++) {
      if (shots[s] == SHOT_NONE) {
        shots[s] = (shipX << 4) | (SHIP_NUB_Y - 1);
        /* A low formation can already sit on the spawn cell: test it now,
           else the shot tunnels through the alien it was born inside. */
        if (tryHit(shipX, SHIP_NUB_Y - 1)) shots[s] = SHOT_NONE;
        fireAcc = 0;
        break;
      }
    }
    if (fireAcc > FIRE_MS) fireAcc = FIRE_MS;  // slots full: stay primed
  }

  /* Shots climb */
  shotAcc += FRAME_MS;
  if (shotAcc >= SHOT_MS) {
    shotAcc = 0;
    for (uint8_t s = 0; s < MAX_SHOTS; s++) {
      if (shots[s] == SHOT_NONE) continue;
      int8_t sx = shots[s] >> 4;
      int8_t sy = (shots[s] & 0x0F) - 1;
      if (sy < 0 || tryHit(sx, sy)) {
        shots[s] = SHOT_NONE;
      } else {
        shots[s] = (sx << 4) | sy;
      }
    }
  }

  /* Formation marches */
  if (!input::chordHeld()) stepAcc += FRAME_MS;  // formation frozen during the exit chord
  if (stepAcc >= stepInterval()) {
    stepAcc = 0;
    if (!stepFormation()) return GAME_LOSE;
    /* aliens stomping onto a shot's pixel: the shot wins */
    for (uint8_t s = 0; s < MAX_SHOTS; s++) {
      if (shots[s] != SHOT_NONE && tryHit(shots[s] >> 4, shots[s] & 0x0F)) {
        shots[s] = SHOT_NONE;
      }
    }
  }

  draw();
  return GAME_ON;
}

}  // namespace invaders
