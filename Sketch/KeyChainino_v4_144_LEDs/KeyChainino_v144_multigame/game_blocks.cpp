/*
 * BLOCKS — A/B shift (auto-repeat), chord-tap rotates
 * (with wall kicks), gravity only. Board: one 12-bit mask per row;
 * tetrominoes: 4x4-bit masks in PROGMEM.
 *
 * Greyscale: the falling piece is bright, the settled stack is dim — you
 * read the board state at a glance.
 */
#include "display.h"
#include "games.h"
#include "gfx.h"
#include "input.h"

namespace blocks {

#define FALL_START_MS 550
#define FALL_STEP_MS 25
#define FALL_MIN_MS 160

/* bit index = row*4 + col (row 0 on top). Order: I, O, T, S, Z, J, L. */
static const uint16_t TETROMINOES[7][4] PROGMEM = {
  { 0x00F0, 0x4444, 0x0F00, 0x2222 },  // I
  { 0x0066, 0x0066, 0x0066, 0x0066 },  // O
  { 0x0072, 0x0262, 0x0270, 0x0232 },  // T
  { 0x0036, 0x0462, 0x0036, 0x0462 },  // S
  { 0x0063, 0x0264, 0x0063, 0x0264 },  // Z
  { 0x0071, 0x0226, 0x0470, 0x0322 },  // J
  { 0x0074, 0x0622, 0x0170, 0x0223 },  // L
};

static uint16_t board[MATRIX_H];  // bit x = cell (x, row) occupied
static uint8_t piece, rot;
static int8_t pieceX, pieceY;  // top-left of the 4x4 box (can be negative)
static uint16_t fallMs, fallAcc;
static bool topOut;

static uint16_t shapeMask(uint8_t p, uint8_t r) {
  return pgm_read_word(&TETROMINOES[p][r]);
}

static bool collides(uint8_t r, int8_t nx, int8_t ny) {
  uint16_t m = shapeMask(piece, r);
  for (uint8_t i = 0; i < 16; i++) {
    if (m & ((uint16_t)1 << i)) {
      int8_t x = nx + (i & 3);
      int8_t y = ny + (i >> 2);
      if (x < 0 || x >= MATRIX_W || y >= MATRIX_H) return true;
      if (y >= 0 && (board[y] & ((uint16_t)1 << x))) return true;
    }
  }
  return false;
}

static void draw() {
  gfx::clear();
  for (uint8_t y = 0; y < MATRIX_H; y++) {
    uint16_t rowBits = board[y];
    for (uint8_t x = 0; x < MATRIX_W; x++) {
      if (rowBits & ((uint16_t)1 << x)) gfx::px(x, y, BR_DIM);
    }
  }
  uint16_t m = shapeMask(piece, rot);
  for (uint8_t i = 0; i < 16; i++) {
    if (m & ((uint16_t)1 << i)) {
      int8_t y = pieceY + (i >> 2);
      if (y >= 0) gfx::px(pieceX + (i & 3), y, BR_MAX);
    }
  }
}

static void spawn() {
  piece = random(0, 7);
  rot = 0;
  pieceX = 4;
  pieceY = 0;
  if (collides(rot, pieceX, pieceY)) topOut = true;
}

static void clearFullLines() {
  uint8_t cleared = 0;
  for (int8_t y = MATRIX_H - 1; y >= 0; y--) {
    if (board[y] == 0x0FFF) {
      cleared++;
      for (int8_t yy = y; yy > 0; yy--) board[yy] = board[yy - 1];
      board[0] = 0;
      y++;  // recheck the same row after the shift down
    }
  }
  if (cleared) {
    engine::score += cleared;
    uint16_t dec = FALL_STEP_MS * cleared;
    fallMs = (fallMs > FALL_MIN_MS + dec) ? fallMs - dec : FALL_MIN_MS;
    gfx::fillAll(BR_MAX);  // reward flash
    display::flush();
    delay(60);
  }
}

static void lockPiece() {
  uint16_t m = shapeMask(piece, rot);
  for (uint8_t i = 0; i < 16; i++) {
    if (m & ((uint16_t)1 << i)) {
      int8_t y = pieceY + (i >> 2);
      if (y >= 0) board[y] |= ((uint16_t)1 << (pieceX + (i & 3)));
    }
  }
  clearFullLines();
  spawn();
}

static void tryRotate() {
  uint8_t newRot = (rot + 1) & 3;
  static const int8_t kicks[] = { 0, -1, 1, -2, 2 };
  for (uint8_t k = 0; k < sizeof(kicks); k++) {
    if (!collides(newRot, pieceX + kicks[k], pieceY)) {
      rot = newRot;
      pieceX += kicks[k];
      return;
    }
  }
}

void reset() {
  for (uint8_t y = 0; y < MATRIX_H; y++) board[y] = 0;
  fallMs = FALL_START_MS;
  fallAcc = 0;
  topOut = false;
  spawn();
  draw();
}

uint8_t frame() {
  if (input::tapA() && !collides(rot, pieceX - 1, pieceY)) pieceX--;
  if (input::tapB() && !collides(rot, pieceX + 1, pieceY)) pieceX++;
  if (input::chordTap()) tryRotate();

  if (!input::chordHeld()) fallAcc += FRAME_MS;  // frozen during the exit chord
  if (fallAcc >= fallMs) {
    fallAcc = 0;
    if (!collides(rot, pieceX, pieceY + 1)) {
      pieceY++;
    } else {
      lockPiece();
      if (topOut) return GAME_LOSE;
    }
  }

  draw();
  return GAME_ON;
}

}  // namespace blocks
