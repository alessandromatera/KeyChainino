/*
 * BUBBLE — a bubble shooter.
 *
 * Tap A / tap B: rotate the aim. Tap A+B: fire. The bubble bounces off
 * the side walls and sticks when it touches the cluster or the ceiling.
 * 3+ connected bubbles of the same shade pop (+10 each); whatever is
 * left hanging with no path to the ceiling drops (+20 each). Every 6
 * landed shots the ceiling pushes a new row down. Clear the board to
 * win; a bubble reaching the bottom grid row ends the round.
 *
 * Shades are dim vs bright — the only two levels this display tells
 * apart at a glance (see board.h). The aim guide blinks so it is never
 * mistaken for a static bright bubble.
 *
 * The grid is square (4-connectivity), not the usual hex stagger:
 * with one LED per bubble the stagger is invisible, and diagonal
 * connections would play as surprises.
 */
#include "games.h"
#include "gfx.h"
#include "input.h"

namespace bubble {

#define GRID_W 12
#define GRID_H 10  // screen rows 0..9; row 10 stays empty, launcher is row 11
#define LAUNCH_X 88   // launcher at (5.5, 11), in 1/16 px
#define LAUNCH_Y 176
#define X_MAX 191     // 12*16 - 1
#define START_ROWS 4
#define SHOTS_PER_DROP 6
#define FLIGHT_MS 50  // one flight tick = 1px of travel
#define BLINK_MS 120

#define CELL_EMPTY 0
#define CELL_DIM 1
#define CELL_BRIGHT 2

#define AIM 0
#define FLIGHT 1

/* Aim directions in 1/16-px per flight tick, |v| = 16 (1 px/tick):
   25°, 45°, 65°, 90°, 115°, 135°, 155° from horizontal. */
static const int8_t ANGLES[7][2] PROGMEM = {
  { 15, -7 }, { 11, -11 }, { 7, -15 }, { 0, -16 },
  { -7, -15 }, { -11, -11 }, { -15, -7 },
};

static uint8_t grid[GRID_H][GRID_W];
static uint8_t occupied;    // bubbles on the grid
static uint8_t aim;         // index into ANGLES
static uint8_t nextColor;   // the loaded bubble
static uint8_t shots;       // landed shots till the ceiling descends
static uint8_t state;       // AIM or FLIGHT
static int16_t flyX, flyY;  // 1/16 px
static int8_t flyVX, flyVY;
static uint8_t flyColor;
static uint16_t flyAcc, blinkAcc;
static bool blinkOn;

static uint8_t brightOf(uint8_t cell) {
  return cell == CELL_BRIGHT ? BR_MAX : BR_DIM;
}

/* --- 120-cell bitsets (grid is 12x10) ---------------------------------- */

static void bset(uint8_t* b, uint8_t i) { b[i >> 3] |= (uint8_t)(1 << (i & 7)); }
static bool bget(const uint8_t* b, uint8_t i) { return b[i >> 3] & (1 << (i & 7)); }

static uint8_t countBits(const uint8_t* b) {
  uint8_t n = 0;
  for (uint8_t i = 0; i < GRID_W * GRID_H; i++)
    if (bget(b, i)) n++;
  return n;
}

/* One BFS step into cell i. color != 0: same-color only; color == 0: any
   occupied cell. `out` doubles as the visited set. */
static void reach(uint8_t* out, uint8_t* next, uint8_t i, uint8_t color, bool* grew) {
  uint8_t c = grid[i / GRID_W][i % GRID_W];
  if (bget(out, i)) return;
  if (color ? c != color : c == CELL_EMPTY) return;
  bset(out, i);
  bset(next, i);
  *grew = true;
}

/* Flood fill from the seeds already set in `out` (4-connectivity,
   frontier expansion — no deep stack). Returns the final cluster size. */
static uint8_t flood(uint8_t* out, uint8_t color) {
  uint8_t front[15], next[15];
  memcpy(front, out, sizeof(front));
  for (;;) {
    memset(next, 0, sizeof(next));
    bool grew = false;
    for (uint8_t i = 0; i < GRID_W * GRID_H; i++) {
      if (!bget(front, i)) continue;
      uint8_t cx = i % GRID_W, cy = i / GRID_W;
      if (cx > 0)          reach(out, next, i - 1, color, &grew);
      if (cx < GRID_W - 1) reach(out, next, i + 1, color, &grew);
      if (cy > 0)          reach(out, next, i - GRID_W, color, &grew);
      if (cy < GRID_H - 1) reach(out, next, i + GRID_W, color, &grew);
    }
    if (!grew) break;
    memcpy(front, next, sizeof(front));
  }
  return countBits(out);
}

/* --- drawing ------------------------------------------------------------ */

static void drawScene() {
  gfx::clear();
  for (uint8_t y = 0; y < GRID_H; y++)
    for (uint8_t x = 0; x < GRID_W; x++)
      if (grid[y][x] != CELL_EMPTY) gfx::px(x, y, brightOf(grid[y][x]));

  if (state == FLIGHT) {
    gfx::px((int8_t)(flyX >> 4), (int8_t)(flyY >> 4), brightOf(flyColor));
    return;
  }
  /* AIM: the loaded bubble plus a blinking 3-dot guide. Dots skip
     occupied cells so a dim bubble is never painted bright. */
  gfx::px(5, 11, brightOf(nextColor));
  if (!blinkOn) return;
  int8_t vx = (int8_t)pgm_read_byte(&ANGLES[aim][0]);
  int8_t vy = (int8_t)pgm_read_byte(&ANGLES[aim][1]);
  for (uint8_t k = 1; k <= 3; k++) {
    int16_t gx = LAUNCH_X + vx * 2 * k;
    int16_t gy = LAUNCH_Y + vy * 2 * k;
    if (gx < 0 || gx > X_MAX || gy < 0) continue;
    int8_t cx = (int8_t)(gx >> 4);
    int8_t cy = (int8_t)(gy >> 4);
    if (cy < GRID_H && grid[cy][cx] != CELL_EMPTY) continue;
    gfx::px(cx, cy, BR_MAX);
  }
}

/* --- effects (short blocking animations; the engine resyncs after) ------ */

static void clearBits(const uint8_t* bits) {
  for (uint8_t i = 0; i < GRID_W * GRID_H; i++)
    if (bget(bits, i)) grid[i / GRID_W][i % GRID_W] = CELL_EMPTY;
}

static void flashBits(const uint8_t* bits) {
  for (uint8_t t = 0; t < 2; t++) {
    for (uint8_t i = 0; i < GRID_W * GRID_H; i++)
      if (bget(bits, i)) gfx::px(i % GRID_W, i / GRID_W, BR_OFF);
    display::flush();
    delay(80);
    for (uint8_t i = 0; i < GRID_W * GRID_H; i++)
      if (bget(bits, i)) gfx::px(i % GRID_W, i / GRID_W, BR_MAX);
    display::flush();
    delay(80);
  }
}

/* Hanging bubbles fall straight down off the screen. They stay in `grid`
   (their color rides along) and are only cleared afterwards. */
static void dropAnim(const uint8_t* bits) {
  for (uint8_t step = 1;; step++) {
    bool any = false;
    gfx::clear();
    for (uint8_t i = 0; i < GRID_W * GRID_H; i++) {
      uint8_t c = grid[i / GRID_W][i % GRID_W];
      if (c == CELL_EMPTY || bget(bits, i)) continue;
      gfx::px(i % GRID_W, i / GRID_W, brightOf(c));
    }
    for (uint8_t i = 0; i < GRID_W * GRID_H; i++) {
      if (!bget(bits, i)) continue;
      int8_t ny = i / GRID_W + step;
      if (ny >= MATRIX_H) continue;
      gfx::px(i % GRID_W, ny, brightOf(grid[i / GRID_W][i % GRID_W]));
      any = true;
    }
    display::flush();
    if (!any) break;
    delay(40);
  }
}

/* --- rules -------------------------------------------------------------- */

static bool rowHas(uint8_t y) {
  for (uint8_t x = 0; x < GRID_W; x++)
    if (grid[y][x] != CELL_EMPTY) return true;
  return false;
}

static void pushDown() {
  /* Row GRID_H-1 is always empty here — reaching it ends the round
     before the ceiling ever moves. */
  for (int8_t y = GRID_H - 1; y > 0; y--) memcpy(grid[y], grid[y - 1], GRID_W);
  for (uint8_t x = 0; x < GRID_W; x++)
    grid[0][x] = random(0, 2) ? CELL_BRIGHT : CELL_DIM;
  occupied += GRID_W;
}

/* The loaded bubble always matches a shade still on the board, so the
   player never gets a shot that cannot possibly pop anything. */
static uint8_t pickColor() {
  bool dim = false, bright = false;
  for (uint8_t i = 0; i < GRID_W * GRID_H; i++) {
    uint8_t c = grid[i / GRID_W][i % GRID_W];
    dim |= (c == CELL_DIM);
    bright |= (c == CELL_BRIGHT);
  }
  if (dim && bright) return random(0, 2) ? CELL_BRIGHT : CELL_DIM;
  return dim ? CELL_DIM : CELL_BRIGHT;
}

/* Resolve a landing: snap to a cell, pop, drop, descend, re-arm.
   Returns GAME_ON / GAME_WIN / GAME_LOSE. */
static uint8_t land(uint8_t cx, uint8_t cy) {
  /* Nearest empty cell around the contact point. A collision always
     leaves at least one candidate; the full-grid fallback only covers a
     completely surrounded contact point. */
  int8_t lx = -1, ly = -1;
  int32_t best = 0x7FFFFFFF;
  for (int8_t dy = -1; dy <= 1; dy++)
    for (int8_t dx = -1; dx <= 1; dx++) {
      int8_t nx = cx + dx, ny = cy + dy;
      if (nx < 0 || nx >= GRID_W || ny < 0 || ny >= GRID_H) continue;
      if (grid[ny][nx] != CELL_EMPTY) continue;
      int32_t ddx = nx * 16 + 8 - flyX;
      int32_t ddy = ny * 16 + 8 - flyY;
      int32_t d = ddx * ddx + ddy * ddy;
      if (d < best) { best = d; lx = nx; ly = ny; }
    }
  if (lx < 0) {
    for (uint8_t i = 0; i < GRID_W * GRID_H; i++) {
      if (grid[i / GRID_W][i % GRID_W] != CELL_EMPTY) continue;
      int32_t ddx = (i % GRID_W) * 16 + 8 - flyX;
      int32_t ddy = (i / GRID_W) * 16 + 8 - flyY;
      int32_t d = ddx * ddx + ddy * ddy;
      if (d < best) { best = d; lx = i % GRID_W; ly = i / GRID_W; }
    }
  }
  if (lx < 0) return GAME_LOSE;  // no empty cell left at all

  grid[ly][lx] = flyColor;
  occupied++;

  uint8_t bits[15];
  memset(bits, 0, sizeof(bits));
  bset(bits, ly * GRID_W + lx);
  uint8_t n = flood(bits, flyColor);
  if (n >= 3) {
    engine::score += 10 * n;
    flashBits(bits);
    clearBits(bits);
    occupied -= n;

    /* Whatever is left hanging with no path to the ceiling drops. */
    uint8_t seen[15], drops[15];
    memset(seen, 0, sizeof(seen));
    for (uint8_t x = 0; x < GRID_W; x++)
      if (grid[0][x] != CELL_EMPTY) bset(seen, x);
    flood(seen, 0);
    memset(drops, 0, sizeof(drops));
    for (uint8_t i = 0; i < GRID_W * GRID_H; i++)
      if (grid[i / GRID_W][i % GRID_W] != CELL_EMPTY && !bget(seen, i))
        bset(drops, i);
    uint8_t nd = countBits(drops);
    if (nd) {
      engine::score += 20 * nd;
      dropAnim(drops);
      clearBits(drops);
      occupied -= nd;
    }
  }
  drawScene();

  if (!occupied) return GAME_WIN;         // win outranks the deadline row
  if (rowHas(GRID_H - 1)) return GAME_LOSE;
  if (--shots == 0) {
    shots = SHOTS_PER_DROP;
    pushDown();
    drawScene();
    if (rowHas(GRID_H - 1)) return GAME_LOSE;
  }
  nextColor = pickColor();
  state = AIM;
  drawScene();
  return GAME_ON;
}

/* --- the engine contract ------------------------------------------------- */

void reset() {
  memset(grid, 0, sizeof(grid));
  for (uint8_t y = 0; y < START_ROWS; y++)
    for (uint8_t x = 0; x < GRID_W; x++)
      grid[y][x] = random(0, 2) ? CELL_BRIGHT : CELL_DIM;
  occupied = START_ROWS * GRID_W;
  aim = 3;  // straight up
  shots = SHOTS_PER_DROP;
  state = AIM;
  blinkOn = true;
  flyAcc = blinkAcc = 0;
  nextColor = pickColor();
  drawScene();
}

uint8_t frame() {
  if (state == AIM) {
    bool dirty = false;
    if (input::tapA()) { aim = (aim + 6) % 7; dirty = true; }
    if (input::tapB()) { aim = (aim + 1) % 7; dirty = true; }
    if (input::chordTap()) {
      flyX = LAUNCH_X;
      flyY = LAUNCH_Y;
      flyVX = (int8_t)pgm_read_byte(&ANGLES[aim][0]);
      flyVY = (int8_t)pgm_read_byte(&ANGLES[aim][1]);
      flyColor = nextColor;
      state = FLIGHT;
      dirty = true;
    }
    blinkAcc += FRAME_MS;
    if (blinkAcc >= BLINK_MS) {
      blinkAcc = 0;
      blinkOn = !blinkOn;
      dirty = true;
    }
    if (dirty) drawScene();
    return GAME_ON;
  }

  /* FLIGHT — drain the buttons so a tap queued mid-flight can't fire (or
     re-aim) the next shot the instant this one lands. */
  input::tapA();
  input::tapB();
  input::chordTap();

  flyAcc += FRAME_MS;
  if (flyAcc < FLIGHT_MS) return GAME_ON;
  flyAcc = 0;

  flyX += flyVX;
  flyY += flyVY;
  if (flyX < 0) { flyX = -flyX; flyVX = -flyVX; }
  else if (flyX > X_MAX) { flyX = 2 * X_MAX - flyX; flyVX = -flyVX; }

  if (flyY <= 8) {  // touched the ceiling
    int8_t cx = (int8_t)(flyX >> 4);
    if (cx < 0) cx = 0;
    else if (cx >= GRID_W) cx = GRID_W - 1;
    return land((uint8_t)cx, 0);
  }

  uint8_t cx = (uint8_t)(flyX >> 4);
  uint8_t cy = (uint8_t)(flyY >> 4);
  if (cy < GRID_H &&
      (grid[cy][cx] != CELL_EMPTY ||
       (cx > 0 && grid[cy][cx - 1] != CELL_EMPTY) ||
       (cx < GRID_W - 1 && grid[cy][cx + 1] != CELL_EMPTY) ||
       (cy > 0 && grid[cy - 1][cx] != CELL_EMPTY) ||
       (cy < GRID_H - 1 && grid[cy + 1][cx] != CELL_EMPTY))) {
    return land(cx, cy);
  }

  drawScene();
  return GAME_ON;
}

}  // namespace bubble
