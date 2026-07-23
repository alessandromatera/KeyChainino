/*
 * STEROIDS — the owner's own heritage name from the original KeyChainino.
 * Rocks (dim, 1-2px clusters) tumble down from the top at varying x and
 * fall speeds; A/B strafe your ship along the bottom. The ship AUTO-FIRES
 * a bright shot straight up on a steady timer — no button to press, no
 * chord to coordinate: you dodge and you aim by *where you stand*, and
 * being lined up under a rock when the next shot goes up is the whole
 * skill. A shot destroys whatever rock it touches (+score); a rock that
 * reaches your row and overlaps you costs a life (3 lives) — dodge it and
 * it just recycles off the bottom, no penalty. Speed ramps with score.
 *
 * The old scheme fired on an A+B chord-tap, and the owner's playtest was
 * blunt: coordinating a two-button chord *while* strafing away from a
 * falling rock is too much for two thumbs at once. Auto-fire drops the
 * firing button entirely so both thumbs do one job — move — and the
 * dodge-vs-aim tension lands where it belongs: a rock bearing down on your
 * column forces you off the very spot you had to hold to kill it.
 *
 * The ship is a nub-over-base sprite, same idiom as the new Invaders ship
 * (and Racer's nose/rear car): a single pixel nub one row above a 3-wide
 * base, so it reads as a ship instead of a lone pixel, and the nub is
 * where shots spawn from.
 *
 * Controls: A strafes left, B strafes right (a chord freezes the ship,
 * same idiom as Invaders/Racer/Tennis/Bricks). Firing is automatic. A
 * *held* A+B chord is still the engine's universal back-to-menu exit —
 * with auto-fire there's no chord-tap fire left for it to conflict with,
 * so the hold exits cleanly and holding it never drifts the frozen ship.
 *
 * Greyscale: 2-wide rocks dim, lone 1px rocks mid (a lone dim pixel ghosts
 * and vanishes on charlieplex hardware), your ship and shots bright. A
 * dying rock flashes bright for one frame.
 */
#include "display.h"
#include "games.h"
#include "gfx.h"
#include "input.h"

namespace steroids {

#define MAX_ROCKS 4
#define MAX_SHOTS 2
#define SHIP_Y (MATRIX_H - 1)     // base row (bottom of the matrix)
#define SHIP_NUB_Y (SHIP_Y - 1)   // nub row: the ship's topmost pixel
#define SHIP_HALF_W 1             // base spans shipX-1..shipX+1 (3px)
#define SHIP_MS 90
#define SHOT_MS 70
#define AUTOFIRE_MS 340   // ship fires on its own this often: slow enough that
                          // WHERE you stand is the aim, fast enough to kill
#define SPAWN_START_MS 700
#define SPAWN_MIN_MS 260
#define SPAWN_STEP_MS 15   // spawns get faster each rock destroyed
#define FALL_START_MS 260
/* 150ms floor: at the old 110ms a rock crossed the last 3 rows in 330ms --
   under reaction + strafe time, so some deaths were flat-out inescapable. */
#define FALL_MIN_MS 150
#define FALL_STEP_MS 6     // rocks get faster each rock destroyed
#define FALL_JITTER_MS 60  // per-rock speed jitter -- "varying speeds"

struct Rock {
  uint8_t x;
  int8_t y;         // -1 = slot free
  uint8_t shape;     // 0 = single pixel, 1 = 2-wide horizontal cluster
  uint16_t fallMs;
  uint16_t fallAcc;
};

static Rock rocks[MAX_ROCKS];
static uint8_t shotX[MAX_SHOTS];
static int8_t shotY[MAX_SHOTS];  // -1 = inactive

static uint8_t shipX;
static uint8_t lives;
static uint16_t spawnMs, fallBaseMs;
static uint16_t shipAcc, shotAcc, spawnAcc, fireAcc;

static inline uint8_t rockWidth(uint8_t r) { return rocks[r].shape ? 2 : 1; }

static void draw() {
  gfx::clear();
  for (uint8_t r = 0; r < MAX_ROCKS; r++) {
    if (rocks[r].y < 0) continue;
    /* lone 1px rocks draw MID, not DIM: a single dim pixel is the
       charlieplex ghosting worst case and nearly invisible on hardware.
       2-wide clusters stay dim (multi-pixel dim reads fine). */
    gfx::px(rocks[r].x, rocks[r].y, rocks[r].shape ? BR_DIM : BR_MID);
    if (rocks[r].shape) gfx::px(rocks[r].x + 1, rocks[r].y, BR_DIM);
  }
  for (uint8_t s = 0; s < MAX_SHOTS; s++) {
    if (shotY[s] >= 0) gfx::px(shotX[s], shotY[s], BR_MAX);
  }
  gfx::px(shipX, SHIP_NUB_Y, BR_MAX);
  for (int8_t dx = -SHIP_HALF_W; dx <= SHIP_HALF_W; dx++) {
    gfx::px(shipX + dx, SHIP_Y, BR_MAX);
  }
}

static void spawnRock() {
  for (uint8_t r = 0; r < MAX_ROCKS; r++) {
    if (rocks[r].y >= 0) continue;  // slot busy
    rocks[r].shape = (uint8_t)random(0, 2);
    uint8_t w = rocks[r].shape ? 2 : 1;
    rocks[r].x = (uint8_t)random(0, MATRIX_W - w + 1);
    rocks[r].y = 0;
    int16_t ms = (int16_t)fallBaseMs + (int16_t)random(-FALL_JITTER_MS, FALL_JITTER_MS + 1);
    if (ms < FALL_MIN_MS) ms = FALL_MIN_MS;
    rocks[r].fallMs = (uint16_t)ms;
    rocks[r].fallAcc = 0;
    return;
  }
}

/* Destroy rock r that shot s just struck: award the point, ramp the pace,
   free both. Shared by the shot's step and the rock's step so a hit lands
   whichever one closed the last cell of gap between them. The kill gets a
   one-frame bright flash first: the framebuffer still holds the live scene,
   so this just repaints the rock's cells. The 20ms hold is NOT optional --
   the engine flushes again the instant frame() returns, so without it the
   flash would be overwritten within the same tick and never scan out. */
static void hitRock(uint8_t r, uint8_t s) {
  gfx::px(rocks[r].x, rocks[r].y, BR_MAX);
  if (rocks[r].shape) gfx::px(rocks[r].x + 1, rocks[r].y, BR_MAX);
  display::flush();
  delay(FRAME_MS);  // hold the flash for exactly one frame (a tiny hit-stop)
  rocks[r].y = -1;
  shotY[s] = -1;
  engine::score++;
  if (spawnMs > SPAWN_MIN_MS + SPAWN_STEP_MS) spawnMs -= SPAWN_STEP_MS;
  if (fallBaseMs > FALL_MIN_MS + FALL_STEP_MS) fallBaseMs -= FALL_STEP_MS;
}

static uint8_t shipHit() {
  for (uint8_t i = 0; i < 3; i++) {  // hit flash
    gfx::fillAll(BR_MAX); display::flush(); delay(70);
    gfx::clear(); display::flush(); delay(70);
  }
  if (--lives == 0) return GAME_LOSE;
  for (uint8_t r = 0; r < MAX_ROCKS; r++) rocks[r].y = -1;  // clear the field
  for (uint8_t s = 0; s < MAX_SHOTS; s++) shotY[s] = -1;
  spawnAcc = 0;
  draw();
  display::flush();
  delay(300);  // breather before the field refills
  return GAME_ON;
}

void reset() {
  shipX = MATRIX_W / 2;
  lives = 3;
  for (uint8_t r = 0; r < MAX_ROCKS; r++) rocks[r].y = -1;
  for (uint8_t s = 0; s < MAX_SHOTS; s++) shotY[s] = -1;
  spawnMs = SPAWN_START_MS;
  fallBaseMs = FALL_START_MS;
  shipAcc = shotAcc = spawnAcc = 0;
  fireAcc = AUTOFIRE_MS;  // primed: first shot goes up on frame one, so the
                          // ship reads as "always shooting" from the start
  draw();
}

uint8_t frame() {
  /* Strafe: continuous while exactly one button is held (chord freezes
     it, same idiom as Invaders/Racer/Tennis/Bricks). */
  shipAcc += FRAME_MS;
  if (shipAcc >= SHIP_MS) {
    if (input::downB() && !input::downA() && shipX < MATRIX_W - 1 - SHIP_HALF_W) {
      shipX++;
      shipAcc = 0;
    } else if (input::downA() && !input::downB() && shipX > SHIP_HALF_W) {
      shipX--;
      shipAcc = 0;
    }
  }

  /* Auto-fire, from the tip of the nub: no button, no chord. The ship
     spits a shot straight up every AUTOFIRE_MS, so aiming is purely a
     matter of standing under the rock you want dead. If both shot slots
     are already in the air the timer stays primed (clamped, never
     overflows) and looses the next shot the instant a slot frees. The A+B
     chord is now the engine's exit gesture alone -- nothing here reads
     chordTap(), so a held chord can never be misread as a fire. */
  fireAcc += FRAME_MS;
  if (fireAcc >= AUTOFIRE_MS) {
    for (uint8_t s = 0; s < MAX_SHOTS; s++) {
      if (shotY[s] < 0) {
        shotX[s] = shipX;
        shotY[s] = SHIP_NUB_Y - 1;
        fireAcc = 0;
        break;
      }
    }
    if (fireAcc > AUTOFIRE_MS) fireAcc = AUTOFIRE_MS;  // slots full: stay primed
  }

  /* Shots climb; a hit destroys the whole rock cluster. */
  shotAcc += FRAME_MS;
  if (shotAcc >= SHOT_MS) {
    shotAcc = 0;
    for (uint8_t s = 0; s < MAX_SHOTS; s++) {
      if (shotY[s] < 0) continue;
      shotY[s]--;
      if (shotY[s] < 0) continue;
      for (uint8_t r = 0; r < MAX_ROCKS; r++) {
        if (rocks[r].y < 0 || rocks[r].y != shotY[s]) continue;
        uint8_t w = rockWidth(r);
        if (shotX[s] < rocks[r].x || shotX[s] >= rocks[r].x + w) continue;
        hitRock(r, s);
        break;
      }
    }
  }

  /* New rocks. */
  spawnAcc += FRAME_MS;
  if (spawnAcc >= spawnMs) {
    spawnAcc = 0;
    spawnRock();
  }

  /* Rocks fall, each at its own pace; check the ship on arrival. The fall
     accumulator freezes while the exit chord is held (chordHeld), so the
     1.2s exit gesture can't walk a rock into you. */
  for (uint8_t r = 0; r < MAX_ROCKS; r++) {
    if (rocks[r].y < 0) continue;
    if (!input::chordHeld()) rocks[r].fallAcc += FRAME_MS;
    if (rocks[r].fallAcc < rocks[r].fallMs) continue;
    rocks[r].fallAcc = 0;
    rocks[r].y++;

    uint8_t w = rockWidth(r);
    /* A shot hops up faster than a rock falls, so between the shot's hops a
       rock can drop straight onto a climbing shot's cell. The shot->rock test
       above only runs on the shot's own step, so without this mirror a rock
       slips clean THROUGH a dead-on shot -- a wasted shot that reads as a bug.
       Check here on the rock's step too. */
    bool shotKilled = false;
    for (uint8_t s = 0; s < MAX_SHOTS; s++) {
      if (shotY[s] < 0 || shotY[s] != rocks[r].y) continue;
      if (shotX[s] < rocks[r].x || shotX[s] >= rocks[r].x + w) continue;
      hitRock(r, s);
      shotKilled = true;
      break;
    }
    if (shotKilled) continue;

    int8_t rockMin = (int8_t)rocks[r].x, rockMax = (int8_t)(rocks[r].x + w - 1);
    bool hitsNub = (rocks[r].y == SHIP_NUB_Y) && (shipX >= rocks[r].x && shipX < rocks[r].x + w);
    int8_t shipMin = (int8_t)shipX - SHIP_HALF_W, shipMax = (int8_t)shipX + SHIP_HALF_W;
    bool hitsBase = (rocks[r].y == SHIP_Y) && !(rockMax < shipMin || rockMin > shipMax);

    if (hitsNub || hitsBase) {
      rocks[r].y = -1;
      uint8_t res = shipHit();
      if (res != GAME_ON) return res;
    } else if (rocks[r].y > SHIP_Y) {
      rocks[r].y = -1;  // dodged: reached the bottom, recycle, no penalty
    }
  }

  draw();
  return GAME_ON;
}

}  // namespace steroids
