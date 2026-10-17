#include "text.h"

#include "display.h"
#include "font_data.h"  // include from this file ONLY (see its header)

namespace text {

#define N_GLYPHS (sizeof(char_table) / sizeof(char_table[0]))
#define TEXT_TOP_ROW 2  // glyphs are 7 rows tall: rows 2..8 centers them

static const char* seg0;  // PROGMEM segment (or NULL)
static const char* seg1;  // RAM segment (or NULL)

static const unsigned char* glyph;  // current glyph (PROGMEM), NULL if none
static uint8_t glyphW, glyphCol;
static uint8_t gapCols;  // blank columns still owed after the current glyph
static uint8_t endCols;  // blank columns after the last glyph (full screen)
static bool run;

static void startCommon() {
  glyph = NULL;
  glyphW = glyphCol = 0;
  gapCols = 0;
  endCols = MATRIX_W;
  run = true;
}

void startP(const char* msgP) {
  seg0 = msgP;
  seg1 = NULL;
  startCommon();
}

void startP2(const char* msgP, const char* ram) {
  seg0 = msgP;
  seg1 = ram;
  startCommon();
}

void stop() { run = false; }

bool active() { return run; }

static char nextChar() {
  if (seg0) {
    char c = (char)pgm_read_byte(seg0);
    if (c) { seg0++; return c; }
    seg0 = NULL;
  }
  if (seg1) {
    char c = *seg1;
    if (c) { seg1++; return c; }
    seg1 = NULL;
  }
  return 0;
}

/* Produce the next 7-bit column of the message (bit 0 = top row). */
static uint8_t nextColumn() {
  for (;;) {
    if (glyphCol < glyphW) {
      return pgm_read_byte(glyph + 1 + glyphCol++);
    }
    if (gapCols) {
      gapCols--;
      return 0;
    }
    char c = nextChar();
    if (c) {
      uint8_t idx = (uint8_t)c - 0x20;
      if (idx >= N_GLYPHS) continue;  // unprintable: skip
      glyph = (const unsigned char*)pgm_read_word(&char_table[idx]);
      glyphW = pgm_read_byte(glyph);
      glyphCol = 0;
      gapCols = 1;  // one blank column between glyphs
      continue;
    }
    if (endCols) {
      endCols--;
      return 0;
    }
    run = false;
    return 0;
  }
}

bool step() {
  if (!run) return false;

  /* Shift the whole framebuffer one column left (fb is column-major)... */
  memmove(display::fb, display::fb + MATRIX_H,
          (MATRIX_W - 1) * MATRIX_H);
  memset(display::fb + (MATRIX_W - 1) * MATRIX_H, 0, MATRIX_H);

  /* ...and stream the next column in on the right. */
  uint8_t col = nextColumn();
  for (uint8_t b = 0; b < 7; b++) {
    if (col & (1 << b)) {
      display::fb[(MATRIX_W - 1) * MATRIX_H + TEXT_TOP_ROW + b] = BR_MAX;
    }
  }
  display::flush();
  return run;
}

}  // namespace text
