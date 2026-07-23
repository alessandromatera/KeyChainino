/*
 * TOWER — the LED arcade classic. Any button locks the
 * sliding row on the stack; overhang is cut off; rows speed up as the
 * tower climbs. Land a block on row 0 to win. Score = rows placed.
 *
 * Lock input is arm-on-release: the lock re-arms only with both thumbs
 * off the buttons, so a held button's auto-repeat taps are all swallowed
 * (no hold-to-autolock) and each physical press locks at most once —
 * unlike the old post-lock guard window, which also ate honest taps.
 *
 * Greyscale: locked tower dim, the sliding row bright; cut overhang and
 * a total miss blink before they go.
 */
#include "display.h"
#include "games.h"
#include "gfx.h"
#include "input.h"

namespace tower {

#define START_MS 150  // faster than the old 180: a wider start piece is more
                       // forgiving per pixel of timing error, so the base
                       // slide speed is upped to keep early rows honest
#define STEP_MS 12     // faster per placed row (was 10): reaches MIN_MS by
                       // the 8th row (150-12*8=54, clamped to 60) instead of
                       // never within an 11-row game (180-10*11=70, always
                       // above the old floor), so the last third of the
                       // tower plays at max speed instead of just easing up
#define MIN_MS 60
#define START_W 7      // matches the icon's 7-wide base (see icons.cpp
                       // ICON_TOWER row 10)

static uint16_t tower[MATRIX_H];
static int8_t y;      // row being placed: 11 (bottom) up to 0
static uint8_t width;
static int8_t pos, dir;
static uint16_t stepMs, stepAcc;
static bool armed;  // a lock is taken only with both thumbs off the buttons

static uint16_t curMask() {
  return (uint16_t)(((1 << width) - 1) << pos);
}

static void draw() {
  gfx::clear();
  for (uint8_t r = 0; r < MATRIX_H; r++) {
    for (uint8_t x = 0; x < MATRIX_W; x++) {
      if (tower[r] & ((uint16_t)1 << x)) gfx::px(x, r, BR_DIM);
    }
  }
  uint16_t m = curMask();
  for (uint8_t x = 0; x < MATRIX_W; x++) {
    if (m & ((uint16_t)1 << x)) gfx::px(x, y, BR_MAX);
  }
}

static uint8_t rowsPlaced() { return (uint8_t)(11 - y); }

/* Blink the given cells of the current row twice (~80ms per phase). The
   framebuffer already shows them lit (frame() drew the sliding row), so
   each cycle starts dark. */
static void blinkRow(uint16_t cells) {
  for (uint8_t i = 0; i < 2; i++) {
    for (uint8_t x = 0; x < MATRIX_W; x++)
      if (cells & ((uint16_t)1 << x)) gfx::px(x, y, BR_OFF);
    display::flush(); delay(80);
    for (uint8_t x = 0; x < MATRIX_W; x++)
      if (cells & ((uint16_t)1 << x)) gfx::px(x, y, BR_MAX);
    display::flush(); delay(80);
  }
}

static uint8_t lockRow() {
  uint16_t mask = curMask();
  uint16_t overlap = (y == 11) ? mask : (uint16_t)(mask & tower[y + 1]);

  if (overlap == 0) {  // completely missed the stack: flash where it fell
    engine::score = rowsPlaced();
    blinkRow(mask);
    return GAME_LOSE;
  }

  if (overlap != mask) blinkRow(mask ^ overlap);  // the cut overhang
  tower[y] = overlap;
  width = __builtin_popcount(overlap);

  if (y == 0) {  // topped out: full tower
    engine::score = 12;
    draw();
    display::flush();
    delay(300);
    for (uint8_t i = 0; i < 5; i++) {  // victory flash
      gfx::fillAll(BR_MAX); display::flush(); delay(100);
      gfx::clear(); display::flush(); delay(100);
    }
    return GAME_WIN;
  }

  y--;
  stepMs = START_MS - STEP_MS * rowsPlaced();
  if (stepMs < MIN_MS) stepMs = MIN_MS;
  pos = 0;
  dir = 1;
  return GAME_ON;
}

void reset() {
  for (uint8_t r = 0; r < MATRIX_H; r++) tower[r] = 0;
  y = 11;
  width = START_W;
  pos = 0;
  dir = 1;
  stepMs = START_MS;
  stepAcc = 0;
  armed = false;
  draw();
}

uint8_t frame() {
  /* Re-arm only with both thumbs off the buttons (see header) */
  if (!input::downA() && !input::downB()) armed = true;

  bool tap = input::tapA() | input::tapB() | input::chordTap();
  if (tap && armed) {
    armed = false;
    uint8_t r = lockRow();
    if (r != GAME_ON) return r;
  }

  stepAcc += FRAME_MS;
  if (stepAcc >= stepMs) {
    stepAcc = 0;
    pos += dir;
    if (pos < 0) { pos = 0; dir = 1; }
    else if (pos > MATRIX_W - (int8_t)width) { pos = MATRIX_W - width; dir = -1; }
  }

  draw();
  return GAME_ON;
}

}  // namespace tower
