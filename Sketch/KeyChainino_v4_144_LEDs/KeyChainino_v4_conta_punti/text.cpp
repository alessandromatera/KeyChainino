#include "text.h"

#include "display.h"
#include "font_data.h"  // include from this file ONLY (see its header)

namespace text {

#define TEXT_TOP_ROW 2  // glyphs are 7 rows tall: rows 2..8 centers them
#define ROLL_ROWS 10     // roll travel: `to` starts just below the screen

static const char* msg;  // rest of the string still to stream (scroll)
static const char* rollFrom;
static const char* rollTo;
static uint8_t rollRow;  // rows rolled so far
static uint8_t rollFromLevel;
static bool rollDown;
static bool rolling;     // the running animation is a roll, not a scroll

static const unsigned char* glyph;  // current glyph (PROGMEM), NULL if none
static uint8_t glyphW, glyphCol;
static uint8_t gapCols;  // blank columns still owed after the current glyph
static uint8_t endCols;  // blank columns after the last glyph (full screen)
static bool run;

/* The font holds digits only; anything else is skipped. */
static const unsigned char* glyphFor(char c) {
  if (c < '0' || c > '9') return NULL;
  return (const unsigned char*)pgm_read_word(&digit_table[c - '0']);
}

/* Light the set bits of one 7-bit glyph column (bit 0 = its top row) in
   framebuffer column x at `level`, with the glyph's top row at `top`.
   Rows that fall off the screen (a roll moves glyphs through the edges)
   are clipped. fb is column-major, so a column is contiguous. */
static void putColumn(uint8_t x, uint8_t col, int8_t top,
                      uint8_t level = BR_MAX) {
  uint8_t* p = display::fb + x * MATRIX_H;
  for (uint8_t b = 0; b < 7; b++) {
    int8_t y = top + b;
    if ((col & (1 << b)) && (uint8_t)y < MATRIX_H) p[y] = level;
  }
}

/* Draw s horizontally centered with its top row at `top`, over what is
   already in fb. No clear, no flush. */
static void render(const char* s, int8_t top, uint8_t level = BR_MAX) {
  /* Width first, to center: glyph widths plus one blank column between. */
  uint8_t w = 0;
  for (const char* p = s; *p; p++) {
    const unsigned char* g = glyphFor(*p);
    if (g) w += pgm_read_byte(g) + 1;
  }
  if (w) w--;  // no gap after the last glyph

  int8_t x = (MATRIX_W - w) / 2;  // what doesn't fit is clipped at the edges
  for (const char* p = s; *p; p++) {
    const unsigned char* g = glyphFor(*p);
    if (!g) continue;
    uint8_t gw = pgm_read_byte(g);
    for (uint8_t c = 0; c < gw; c++, x++) {
      if ((uint8_t)x < MATRIX_W) {
        putColumn(x, pgm_read_byte(g + 1 + c), top, level);
      }
    }
    x++;  // gap
  }
}

void draw(const char* s) {
  run = false;  // an animation in progress is replaced by this
  display::clear();
  render(s, TEXT_TOP_ROW);
  display::flush();
}

void paint(const char* s, uint8_t level) { render(s, TEXT_TOP_ROW, level); }

void start(const char* s) {
  msg = s;
  glyph = NULL;
  glyphW = glyphCol = 0;
  gapCols = 0;
  endCols = MATRIX_W;
  rolling = false;
  run = true;
}

/* One frame of the roll: `from` rollRow rows up from its place, `to`
   ROLL_ROWS - rollRow rows below its own (mirrored when rolling down). */
static void rollFrame() {
  int8_t off = rollDown ? -(int8_t)rollRow : rollRow;
  int8_t span = rollDown ? -ROLL_ROWS : ROLL_ROWS;
  display::clear();
  render(rollFrom, TEXT_TOP_ROW - off, rollFromLevel);
  render(rollTo, TEXT_TOP_ROW + span - off);
  display::flush();
}

void roll(const char* from, const char* to, bool down, uint8_t fromLevel) {
  rollFrom = from;
  rollTo = to;
  rollDown = down;
  rollFromLevel = fromLevel;
  rollRow = 0;
  rollFrame();  // `from` in place: the roll starts from the number shown
  rolling = true;
  run = true;
}

void stop() { run = false; }

bool active() { return run; }

uint8_t stepMs() { return rolling ? TEXT_ROLL_MS : TEXT_STEP_MS; }

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
    if (*msg) {
      glyph = glyphFor(*msg++);
      if (!glyph) continue;  // unprintable: skip
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

  if (rolling) {
    rollRow++;
    rollFrame();
    if (rollRow >= ROLL_ROWS) run = false;  // `to` has landed in place
    return run;
  }

  /* Shift the whole framebuffer one column left (fb is column-major)... */
  memmove(display::fb, display::fb + MATRIX_H,
          (MATRIX_W - 1) * MATRIX_H);
  memset(display::fb + (MATRIX_W - 1) * MATRIX_H, 0, MATRIX_H);

  /* ...and stream the next column in on the right. */
  putColumn(MATRIX_W - 1, nextColumn(), TEXT_TOP_ROW);
  display::flush();
  return run;
}

}  // namespace text
