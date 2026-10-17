/*
 * gfx.h — drawing on the greyscale framebuffer. Every routine takes a
 * brightness 0..3 (BR_OFF..BR_MAX); nothing here touches the hardware,
 * call display::flush() (or let the engine do it) to show the result.
 */
#ifndef RETHINK_GFX_H
#define RETHINK_GFX_H

#include "board.h"
#include "display.h"

namespace gfx {

inline void px(int8_t x, int8_t y, uint8_t v) {
  if ((uint8_t)x < MATRIX_W && (uint8_t)y < MATRIX_H) {
    display::fb[(uint8_t)x * MATRIX_H + (uint8_t)y] = v;
  }
}

inline uint8_t get(int8_t x, int8_t y) {
  if ((uint8_t)x < MATRIX_W && (uint8_t)y < MATRIX_H) {
    return display::fb[(uint8_t)x * MATRIX_H + (uint8_t)y];
  }
  return 0;
}

inline void clear() { memset(display::fb, 0, MATRIX_W * MATRIX_H); }
inline void fillAll(uint8_t v) { memset(display::fb, v, MATRIX_W * MATRIX_H); }

/* Swap BR_DIM<->BR_MAX across the framebuffer (off/mid untouched): a hit or
   death flash that keeps the scenery readable. Flush to show. */
void invert();

void hline(int8_t x0, int8_t x1, int8_t y, uint8_t v);
void vline(int8_t x, int8_t y0, int8_t y1, uint8_t v);
void line(int8_t x0, int8_t y0, int8_t x1, int8_t y1, uint8_t v);
void rect(int8_t x, int8_t y, int8_t w, int8_t h, uint8_t v);
void fillRect(int8_t x, int8_t y, int8_t w, int8_t h, uint8_t v);
void circle(int8_t cx, int8_t cy, int8_t r, uint8_t v);

/* Draw a packed 12-row PROGMEM bitmap (bit x of word y = pixel x,y). */
void drawIconP(const uint16_t* rows, uint8_t v = BR_MAX);

}  // namespace gfx

#endif
