#include "gfx.h"

namespace gfx {

void hline(int8_t x0, int8_t x1, int8_t y, uint8_t v) {
  if (x0 > x1) { int8_t t = x0; x0 = x1; x1 = t; }
  for (int8_t x = x0; x <= x1; x++) px(x, y, v);
}

void vline(int8_t x, int8_t y0, int8_t y1, uint8_t v) {
  if (y0 > y1) { int8_t t = y0; y0 = y1; y1 = t; }
  for (int8_t y = y0; y <= y1; y++) px(x, y, v);
}

/* Bresenham, sufficient for the few diagonals the games draw. */
void line(int8_t x0, int8_t y0, int8_t x1, int8_t y1, uint8_t v) {
  int8_t dx = abs(x1 - x0), dy = -abs(y1 - y0);
  int8_t sx = (x0 < x1) ? 1 : -1, sy = (y0 < y1) ? 1 : -1;
  int8_t err = dx + dy;
  for (;;) {
    px(x0, y0, v);
    if (x0 == x1 && y0 == y1) break;
    int8_t e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

void rect(int8_t x, int8_t y, int8_t w, int8_t h, uint8_t v) {
  if (w <= 0 || h <= 0) return;
  hline(x, x + w - 1, y, v);
  hline(x, x + w - 1, y + h - 1, v);
  vline(x, y, y + h - 1, v);
  vline(x + w - 1, y, y + h - 1, v);
}

void fillRect(int8_t x, int8_t y, int8_t w, int8_t h, uint8_t v) {
  for (int8_t yy = y; yy < y + h; yy++) hline(x, x + w - 1, yy, v);
}

void circle(int8_t cx, int8_t cy, int8_t r, uint8_t v) {
  int8_t x = 0, y = r;
  int8_t f = 1 - r;
  px(cx, cy + r, v);
  px(cx, cy - r, v);
  px(cx + r, cy, v);
  px(cx - r, cy, v);
  while (x < y) {
    if (f >= 0) { y--; f += 2 - 2 * y; }
    x++;
    f += 1 + 2 * x;
    px(cx + x, cy + y, v); px(cx - x, cy + y, v);
    px(cx + x, cy - y, v); px(cx - x, cy - y, v);
    px(cx + y, cy + x, v); px(cx - y, cy + x, v);
    px(cx + y, cy - x, v); px(cx - y, cy - x, v);
  }
}

void drawIconP(const uint16_t* rows, uint8_t v) {
  for (uint8_t y = 0; y < MATRIX_H; y++) {
    uint16_t w = pgm_read_word(&rows[y]);
    for (uint8_t x = 0; x < MATRIX_W; x++) {
      if (w & ((uint16_t)1 << x)) px(x, y, v);
    }
  }
}

void invert() {
  for (uint8_t i = 0; i < MATRIX_W * MATRIX_H; i++) {
    uint8_t v = display::fb[i];
    if (v == BR_DIM) display::fb[i] = BR_MAX;
    else if (v == BR_MAX) display::fb[i] = BR_DIM;
  }
}

}  // namespace gfx
