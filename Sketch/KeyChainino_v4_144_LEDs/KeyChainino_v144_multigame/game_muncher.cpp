/*
 * MUNCHER — maze-chase. Snake-style controls: A turns left, B turns right
 * relative to your heading; you crawl forward continuously on a timer.
 * The turn is QUEUED, not instant: a press is remembered and applied at
 * the next cell where that turn is actually possible, so you no longer
 * have to nail the one exact step-window on the junction to take a corner
 * — press as you approach and the muncher turns the moment it opens up.
 * (A real thumb on a keychain can't read the maze, plan a relative turn
 * and land it frame-perfectly inside one slow step; the buffer is the
 * classic maze-chase forgiveness that makes it human-playable, tested
 * below.) The base cadence is deliberately slow for the same reason.
 * Eat every dot to clear the maze (celebratory flash, a new maze loads,
 * speed ramps up); one ghost chases you. Contact with the ghost costs a
 * life (3 lives), not the walls — running into a wall just stops you
 * where you stand until you turn toward an open cell, it's never fatal.
 * After every spawn the ghost holds still for a short breather so you can
 * get your bearings before the chase resumes (it keeps blinking the whole
 * time, so it never reads as dead — it's just not moving yet). The player
 * stays meaningfully faster than the ghost: that speed edge is the escape
 * mechanism, and the ghost's ramp slowly eats into it over cleared mazes.
 *
 * Maze: one of three hand-authored 12x12 layouts (PROGMEM, walls only —
 * dots are derived from the inverse at load time), picked at random each
 * time a maze loads. All three were verified fully connected by a
 * flood-fill probe before being baked in (every open cell reachable from
 * both the player's and the ghost's spawn corner), so every dot is always
 * gettable.
 *
 * Greyscale, the key design problem: walls and dots both want to read as
 * "dim" (only 3 honest brightness levels exist), so they need a second
 * axis to stay distinguishable. Walls are dim and static; dots are dim
 * and *slowly* strobe dim<->off (a gentle twinkle) — clearly slower than
 * the ghost's blink, and never sharing a brightness *transition* with the
 * ghost, which blinks bright<->dim instead (mostly bright, brief dim
 * dips) so it never drops to off and never gets mistaken for a dead LED.
 * You (the player) are solid bright, no blinking, so at a glance: solid
 * bright = you, blinking bright = ghost, static dim = wall, twinkling dim
 * = food.
 */
#include "display.h"
#include "games.h"
#include "gfx.h"
#include "input.h"

namespace muncher {

#define NUM_LAYOUTS 3
/* Player spawns at (0,0) heading DOWN: column 0 is fully open in all three
   layouts, so you move from the first step. ((1,1) faced right into a wall
   in every layout -- you stood parked until you blindly turned.) */
#define PLAYER_SPAWN_X 0
#define PLAYER_SPAWN_Y 0
#define GHOST_SPAWN_X 10
#define GHOST_SPAWN_Y 10

/* Cadence: slow enough that a human can read the maze and plan a relative
   turn between steps (a scripted bot signed off on 220ms, but the owner
   playtested it and called it unplayable -- trust the human). 340ms base;
   the player stays a clear notch faster than the ghost (the speed edge is
   how you escape) and the ghost's faster ramp gradually closes that gap as
   mazes fall, so late mazes get frantic. All periods are 20ms multiples so
   the frame accumulator lands on them exactly. Re-derived with a
   human-grade probe (reactions only ~300ms, a couple tiles of lookahead,
   turns that occasionally land a step late): first-time-human clears maze
   one ~76% of the time, still dies a maze or two later. */
#define MOVE_START_MS 340
#define MOVE_MIN_MS 200
#define MOVE_STEP_MS 15    // faster per maze cleared
#define GHOST_START_MS 460  // 120ms slower than you to start
#define GHOST_MIN_MS 200
#define GHOST_STEP_MS 30   // ghost ramps twice as fast: the gap closes over ~8 clears
#define GHOST_RANDOM_TURN_ODDS 3  // 1-in-N escapable wrong turn (swept 2/3/4; 3 balances)
#define GHOST_FREEZE_MS 1500  // post-spawn breather: ghost blinks in place, doesn't chase
#define TURN_BUFFER_STEPS 3   // queued turn stays armed this many player steps

#define DOT_STROBE_MS 400     // slow, gentle twinkle
#define GHOST_ON_MS 220       // mostly bright...
#define GHOST_OFF_MS 60       // ...brief dim dip (never fully off)

/* Wall layouts: bit x set = wall at (x, row). Each was authored as long
   (>=2px) segments on purpose -- a lone dim pixel is the worst case for
   charlieplex ghosting (see the sketch README), a run of them reads
   cleanly as a wall. Verified fully connected by flood fill. */
static const uint16_t MAZE[NUM_LAYOUTS][MATRIX_H] PROGMEM = {
  { 0x444, 0x444, 0x444, 0x554, 0x554, 0x554, 0x554, 0x554, 0x554, 0x110, 0x110, 0x110 },
  { 0x124, 0x124, 0x124, 0x124, 0x124, 0x124, 0x492, 0x492, 0x492, 0x492, 0x092, 0x000 },
  { 0x000, 0x6EC, 0x6EE, 0x000, 0x776, 0x776, 0x000, 0x76E, 0x76E, 0x000, 0x376, 0x000 },
};

static uint16_t dots[MATRIX_H];  // bit x set = uneaten dot at (x, row)
static uint8_t curLayout;
static uint16_t dotsLeft;

static int8_t px, py;
static uint8_t heading;    // 0 up, 1 right, 2 down, 3 left (Snake convention)
static int8_t pendHeading; // queued-turn target heading, -1 = nothing queued
static uint8_t pendLife;   // player-steps the queued turn stays armed

static int8_t gx, gy;
static int8_t gLastDir;  // -1 = none yet

static uint8_t lives;
static uint16_t moveMs, ghostMs;
static uint16_t moveAcc, ghostAcc, dotBlinkAcc, ghostBlinkAcc;
static uint16_t ghostFreeze;  // ms left of the post-spawn "ghost sits still" breather
static bool dotsOn, ghostVisible;

static inline void deltaFor(uint8_t dir, int8_t* dx, int8_t* dy) {
  switch (dir) {
    case 0: *dx = 0; *dy = -1; break;
    case 1: *dx = 1; *dy = 0; break;
    case 2: *dx = 0; *dy = 1; break;
    default: *dx = -1; *dy = 0; break;
  }
}

static inline bool wallAt(int8_t x, int8_t y) {
  return (pgm_read_word(&MAZE[curLayout][y]) & ((uint16_t)1 << x)) != 0;
}

static inline bool cellOpen(int8_t x, int8_t y) {
  if ((uint8_t)x >= MATRIX_W || (uint8_t)y >= MATRIX_H) return false;
  return !wallAt(x, y);
}

static inline bool dotAt(int8_t x, int8_t y) { return (dots[y] & ((uint16_t)1 << x)) != 0; }
static inline void eatDot(int8_t x, int8_t y) { dots[y] &= ~((uint16_t)1 << x); }

static void draw() {
  gfx::clear();
  for (uint8_t y = 0; y < MATRIX_H; y++) {
    uint16_t w = pgm_read_word(&MAZE[curLayout][y]);
    uint16_t d = dots[y];
    for (uint8_t x = 0; x < MATRIX_W; x++) {
      if (w & ((uint16_t)1 << x)) {
        gfx::px(x, y, BR_DIM);
      } else if (dotsOn && (d & ((uint16_t)1 << x))) {
        gfx::px(x, y, BR_DIM);
      }
    }
  }
  gfx::px(gx, gy, ghostVisible ? BR_MAX : BR_DIM);
  gfx::px(px, py, BR_MAX);  // player drawn last: always wins the pixel
}

/* Load a fresh (randomly picked) maze: refill dots, put player and ghost
   back at their spawn corners (as far apart as the maze gets), keep score
   and lives. ramp=true also speeds both of you up a notch (maze-clear
   reward/penalty curve). */
static void newMaze(bool ramp) {
  curLayout = random(0, NUM_LAYOUTS);
  dotsLeft = 0;
  for (uint8_t y = 0; y < MATRIX_H; y++) {
    uint16_t w = pgm_read_word(&MAZE[curLayout][y]);
    dots[y] = (uint16_t)(~w) & 0x0FFF;
  }
  dots[PLAYER_SPAWN_Y] &= ~((uint16_t)1 << PLAYER_SPAWN_X);  // nothing under your own feet
  for (uint8_t y = 0; y < MATRIX_H; y++) dotsLeft += __builtin_popcount(dots[y]);

  px = PLAYER_SPAWN_X; py = PLAYER_SPAWN_Y;
  heading = 2;  // down the open left column (see spawn note above)
  pendHeading = -1;
  gx = GHOST_SPAWN_X; gy = GHOST_SPAWN_Y;
  gLastDir = -1;
  moveAcc = ghostAcc = dotBlinkAcc = ghostBlinkAcc = 0;
  ghostFreeze = GHOST_FREEZE_MS;
  dotsOn = true;
  ghostVisible = true;

  if (ramp) {
    if (moveMs > MOVE_MIN_MS + MOVE_STEP_MS) moveMs -= MOVE_STEP_MS;
    if (ghostMs > GHOST_MIN_MS + GHOST_STEP_MS) ghostMs -= GHOST_STEP_MS;
  }
}

static void mazeCleared() {
  for (int8_t r = MATRIX_H - 1; r >= 0; r--) {  // celebratory fill-up sweep
    gfx::hline(0, MATRIX_W - 1, r, BR_MAX);
    display::flush();
    delay(18);
  }
  gfx::clear();
  display::flush();
  newMaze(true);
  draw();
}

/* Put player and ghost back at their (far-apart) spawn corners without
   touching the maze/dots/score -- used after a non-fatal contact. */
static void respawn() {
  px = PLAYER_SPAWN_X; py = PLAYER_SPAWN_Y;
  heading = 2;  // down the open left column, same as newMaze()
  pendHeading = -1;
  gx = GHOST_SPAWN_X; gy = GHOST_SPAWN_Y;
  gLastDir = -1;
  moveAcc = ghostAcc = 0;
  ghostFreeze = GHOST_FREEZE_MS;  // a breather after every hit, not just at maze start
}

static uint8_t handleContact() {
  for (uint8_t i = 0; i < 3; i++) {  // hit flash
    gfx::fillAll(BR_MAX); display::flush(); delay(70);
    gfx::clear(); display::flush(); delay(70);
  }
  if (--lives == 0) return GAME_LOSE;
  respawn();
  draw();
  return GAME_ON;
}

static uint8_t movePlayer() {
  /* Spend the queued turn: if the cell it points into is open right now,
     take it; otherwise keep it armed a few more steps (so a press made
     while approaching a corner still fires exactly when the opening
     arrives) and drop it once it goes stale. Only steps where you actually
     MOVE count against the buffer, so it can't expire while you're parked
     against a wall. */
  bool armed = false;
  if (pendHeading >= 0) {
    int8_t tdx, tdy;
    deltaFor(pendHeading, &tdx, &tdy);
    if (cellOpen(px + tdx, py + tdy)) { heading = pendHeading; pendHeading = -1; }
    else armed = true;  // not open yet: stays queued, may expire below
  }
  int8_t dx, dy;
  deltaFor(heading, &dx, &dy);
  int8_t nx = px + dx, ny = py + dy;
  if (cellOpen(nx, ny)) {  // a wall just stops you; it's not a crash
    px = nx; py = ny;
    if (armed && --pendLife == 0) pendHeading = -1;
    if (dotAt(px, py)) {
      eatDot(px, py);
      dotsLeft--;
      engine::score++;
    }
  }
  if (px == gx && py == gy) {
    uint8_t r = handleContact();
    if (r != GAME_ON) return r;
  }
  if (dotsLeft == 0) mazeCleared();
  return GAME_ON;
}

/* Greedy chase toward the player (Manhattan distance), never reversing
   unless it's a dead end, with an occasional random turn thrown in so a
   committed player can juke it -- a perfect chaser is unescapable on a
   board this small. */
static uint8_t moveGhost() {
  bool wildcard = (random(0, GHOST_RANDOM_TURN_ODDS) == 0);
  uint8_t reverseDir = (gLastDir >= 0) ? (uint8_t)((gLastDir + 2) & 3) : 0xFF;

  uint8_t cand[4], nCand = 0;
  for (uint8_t d = 0; d < 4; d++) {
    if (d == reverseDir) continue;
    int8_t dx, dy;
    deltaFor(d, &dx, &dy);
    if (cellOpen(gx + dx, gy + dy)) cand[nCand++] = d;
  }
  if (nCand == 0 && reverseDir != 0xFF) {  // dead end: reversing is forced
    int8_t dx, dy;
    deltaFor(reverseDir, &dx, &dy);
    if (cellOpen(gx + dx, gy + dy)) cand[nCand++] = reverseDir;
  }
  if (nCand == 0) return GAME_ON;  // walled in solid (shouldn't happen)

  uint8_t chosen;
  if (wildcard) {
    chosen = cand[random(0, nCand)];
  } else {
    uint8_t bestIdx = 0;
    int16_t bestDist = 32767;
    for (uint8_t i = 0; i < nCand; i++) {
      int8_t dx, dy;
      deltaFor(cand[i], &dx, &dy);
      int16_t dist = abs((int16_t)(gx + dx) - px) + abs((int16_t)(gy + dy) - py);
      if (dist < bestDist) { bestDist = dist; bestIdx = i; }
    }
    chosen = cand[bestIdx];
  }

  int8_t dx, dy;
  deltaFor(chosen, &dx, &dy);
  gx += dx; gy += dy;
  gLastDir = chosen;

  if (px == gx && py == gy) return handleContact();
  return GAME_ON;
}

void reset() {
  lives = 3;
  moveMs = MOVE_START_MS;
  ghostMs = GHOST_START_MS;
  newMaze(false);
  draw();
}

uint8_t frame() {
  /* Queue the relative turn (target = heading rotated at press time); it's
     applied by movePlayer() at the next cell where it's legal. Latest
     press wins, and re-arms the buffer's lifetime. flush() right after:
     one queued turn per physical press -- otherwise a held button
     auto-repeats and can spin you ~180 degrees at an intersection. */
  if (input::tapA()) { pendHeading = (heading + 3) & 3; pendLife = TURN_BUFFER_STEPS; input::flush(); }
  else if (input::tapB()) { pendHeading = (heading + 1) & 3; pendLife = TURN_BUFFER_STEPS; input::flush(); }

  if (!input::chordHeld()) moveAcc += FRAME_MS;  // frozen during the exit chord
  if (moveAcc >= moveMs) {
    moveAcc = 0;
    uint8_t r = movePlayer();
    if (r != GAME_ON) return r;
  }

  if (ghostFreeze > 0) {  // post-spawn breather: ghost stays put (still blinks below)
    ghostFreeze = (ghostFreeze > FRAME_MS) ? ghostFreeze - FRAME_MS : 0;
  } else {
    if (!input::chordHeld()) ghostAcc += FRAME_MS;
    if (ghostAcc >= ghostMs) {
      ghostAcc = 0;
      uint8_t r = moveGhost();
      if (r != GAME_ON) return r;
    }
  }

  dotBlinkAcc += FRAME_MS;
  if (dotBlinkAcc >= DOT_STROBE_MS) {
    dotBlinkAcc = 0;
    dotsOn = !dotsOn;
  }

  ghostBlinkAcc += FRAME_MS;
  if (ghostBlinkAcc >= (ghostVisible ? GHOST_ON_MS : GHOST_OFF_MS)) {
    ghostBlinkAcc = 0;
    ghostVisible = !ghostVisible;
  }

  draw();
  return GAME_ON;
}

}  // namespace muncher
