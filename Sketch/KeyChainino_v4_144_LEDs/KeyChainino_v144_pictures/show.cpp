#include "show.h"

#include "album.h"
#include "display.h"
#include "gfx.h"
#include "input.h"
#include "pictures_data.h"

namespace show {

#define N (MATRIX_W * MATRIX_H)
#define FX_FRAME_MS 20

static uint16_t pic[12];  // the current entry's still picture
static uint8_t snap[N];   // the outgoing screen while it slides away
static uint8_t trail[N];  // the previous frame, for the motion smear

/* ------------------------------------------------------------------ *
 *  Drawing                                                            *
 * ------------------------------------------------------------------ */

static inline bool lit(int8_t x, int8_t y) {
  return (uint8_t)x < MATRIX_W && (uint8_t)y < MATRIX_H && ((pic[y] >> x) & 1);
}

static void blit(const uint16_t* rows, int8_t dx, int8_t dy, uint8_t v, bool mirror) {
  for (uint8_t y = 0; y < 12; y++) {
    uint16_t w = rows[y];
    if (!w) continue;
    for (uint8_t x = 0; x < 12; x++) {
      if (w & (1u << x)) gfx::px((mirror ? 11 - x : x) + dx, y + dy, v);
    }
  }
}

static void blitSnap(int8_t dx, int8_t dy) {
  for (uint8_t x = 0; x < 12; x++) {
    for (uint8_t y = 0; y < 12; y++) {
      uint8_t v = snap[x * MATRIX_H + y];
      if (v) gfx::px(x + dx, y + dy, v);
    }
  }
}

static void still() {
  gfx::clear();
  blit(pic, 0, 0, BR_MAX, false);
  display::flush();
}

/* Show the frame just drawn, leaving a dim smear wherever the previous
   frame was bright and this one is dark. On a 12-pixel screen that is
   what turns a 2-3 pixel jump per frame into a whoosh. */
static void flushSmeared() {
  for (uint8_t i = 0; i < N; i++) {
    if (!display::fb[i] && trail[i] >= BR_MID) display::fb[i] = BR_DIM;
  }
  display::flush();
}

static void waitUntil(uint32_t t) {
  while ((int32_t)(millis() - t) < 0) input::poll();  // taps queue up meanwhile
}

/* ------------------------------------------------------------------ *
 *  Motion paths (pixels from home, one entry per frame)               *
 * ------------------------------------------------------------------ */

/* fast start, ease-out, one pixel past home and back: a springy push */
static const int8_t SLIDE_PATH[] PROGMEM = {9, 6, 4, 2, 1, 0, -1, -1, 0};
#define SLIDE_FRAME_MS 16

static const int8_t RISE_PATH[] PROGMEM = {10, 8, 6, 4, 3, 2, 1, 0, -1, -1, 0};
#define RISE_FRAME_MS 26

/* a little stretch up, then down and away */
static const int8_t SINK_PATH[] PROGMEM = {-1, -1, 0, 1, 2, 4, 6, 9, 12};
#define SINK_FRAME_MS 40

/* ------------------------------------------------------------------ *
 *  Animation state                                                    *
 * ------------------------------------------------------------------ */

#define NO_FX 0xFF

static bool stepsOn;          // a built-in's hand-made animation is playing
static uint16_t sIdx, sEnd;   // next / end step in STEPS
static uint8_t sBase;         // its first frame in FRAMES
static uint32_t sUntil;       // when the current step ends

static uint8_t fxOn = NO_FX;  // generic effect running (may be inside steps)
static uint32_t fxT0, fxLast;
static uint8_t fxNext;        // next effect for a child's drawing

static void stop() {
  stepsOn = false;
  fxOn = NO_FX;
}

/* ------------------------------------------------------------------ *
 *  Generic effects                                                    *
 * ------------------------------------------------------------------ *
 * Geometry runs in "doubled" coordinates centred on the screen: pixel
 * x maps to 2x-11 (-11..11), so the centre (5.5) is 0 and every
 * transform is plain integer maths. Angles: 256 = one full turn.       */

/* sine, -127..127, by the parabola 4x(pi-x): within a few percent */
static int8_t isin(uint8_t p) {
  uint8_t h = p & 127;
  int16_t v = ((int16_t)h * (128 - h)) >> 5;
  if (v > 127) v = 127;
  return (p & 128) ? -v : v;
}
static inline int8_t icos(uint8_t p) { return isin(p + 64); }

/* doubled coordinate -> nearest pixel */
static inline int8_t px2(int16_t c2) { return (int8_t)(((c2 + 12 + 128) >> 1) - 64); }

/* sparkle particles: twinkling stars around the picture */
#define SPARKS 6
static int8_t spX[SPARKS], spY[SPARKS];
static uint8_t spAge[SPARKS];
static uint8_t spGen;

static void fxBegin(uint8_t id) {
  fxOn = id;
  fxT0 = millis();
  fxLast = fxT0 - FX_FRAME_MS;
  if (id == FX_SPARKLE) {
    for (uint8_t i = 0; i < SPARKS; i++) spAge[i] = 3;  // all dead
    spGen = 0xFF;
  }
}

/* Draw effect `id` at time t (ms since it started). false = finished. */
static bool fxFrame(uint8_t id, uint16_t t) {
  switch (id) {
    case FX_HOP: {
      /* three hops, each lower than the last */
      static const uint16_t HOP_MS[3] = {340, 260, 200};
      static const uint8_t HOP_PX[3] = {4, 2, 1};
      uint16_t start = 0;
      for (uint8_t i = 0; i < 3; i++) {
        if (t < start + HOP_MS[i]) {
          int32_t p = (int32_t)(t - start) * 256 / HOP_MS[i];  // 0..256
          int8_t dy = -(int8_t)((HOP_PX[i] * p * (256 - p) + 8192) >> 14);
          gfx::clear();
          blit(pic, 0, dy, BR_MAX, false);
          display::flush();
          return true;
        }
        start += HOP_MS[i];
      }
      return false;
    }

    case FX_JELLY: {
      /* wobbles like jelly: the bottom stays put, the top sways most */
      const uint16_t T = 1200;
      if (t >= T) return false;
      int16_t amp = (int32_t)(T - t) * 640 / T;              // 2.5 px, fading (1/256 px)
      int16_t top = (int32_t)isin((uint32_t)t * 256 / 240) * amp / 127;
      gfx::clear();
      for (uint8_t y = 0; y < 12; y++) {
        int8_t dx = (int8_t)(((int32_t)top * (11 - y) / 11 + 128) >> 8);
        for (uint8_t x = 0; x < 12; x++) {
          if ((pic[y] >> x) & 1) gfx::px(x + dx, y, BR_MAX);
        }
      }
      display::flush();
      return true;
    }

    case FX_SPARKLE: {
      /* the picture twinkles while little stars pop up around it */
      const uint16_t T = 1500;
      if (t >= T) return false;
      uint8_t gen = t / 70;
      if (gen == spGen) return true;  // hold the frame until the next generation
      spGen = gen;
      for (uint8_t i = 0; i < SPARKS; i++) {
        if (spAge[i] < 3) spAge[i]++;
        if (spAge[i] >= 3 && t < T - 250 && random(2)) {
          for (uint8_t tries = 0; tries < 6; tries++) {
            int8_t x = random(12), y = random(12);
            if (!lit(x, y)) { spX[i] = x; spY[i] = y; spAge[i] = 0; break; }
          }
        }
      }
      gfx::clear();
      blit(pic, 0, 0, BR_MAX, false);
      for (uint8_t i = 0; i < 144; i++) {  // a few lit pixels dip to dim
        if (display::fb[i] && !random(6)) display::fb[i] = BR_DIM;
      }
      for (uint8_t i = 0; i < SPARKS; i++) {
        int8_t x = spX[i], y = spY[i];
        if (spAge[i] == 0) {
          gfx::px(x - 1, y, BR_DIM); gfx::px(x + 1, y, BR_DIM);
          gfx::px(x, y - 1, BR_DIM); gfx::px(x, y + 1, BR_DIM);
        }
        if (spAge[i] < 2) gfx::px(x, y, BR_MAX);
        else if (spAge[i] == 2) gfx::px(x, y, BR_DIM);
      }
      display::flush();
      return true;
    }

    case FX_WAVE: {
      /* a ripple runs through it from left to right */
      const uint16_t T = 1600;
      if (t >= T) return false;
      uint16_t edge = t < T - t ? t : T - t;
      int16_t amp = edge >= 200 ? 384 : (int32_t)edge * 384 / 200;  // 1.5 px
      uint8_t base = (uint32_t)t * 256 / 500;
      gfx::clear();
      for (uint8_t x = 0; x < 12; x++) {
        int8_t dy = (int8_t)(((int32_t)isin(base - x * 26) * amp / 127 + 128) >> 8);
        for (uint8_t y = 0; y < 12; y++) {
          if ((pic[y] >> x) & 1) gfx::px(x, y + dy, BR_MAX);
        }
      }
      display::flush();
      return true;
    }

    case FX_FLIP: {
      /* turns round like a coin: squeezes to a line, comes back mirrored,
         and round again to face front */
      const uint16_t T = 1000;
      if (t >= T) return false;
      int8_t c = icos((uint32_t)t * 256 / T);
      gfx::clear();
      for (uint8_t x = 0; x < 12; x++) {
        int8_t ox = px2(((int16_t)(2 * x - 11) * c) >> 7);
        for (uint8_t y = 0; y < 12; y++) {
          if ((pic[y] >> x) & 1) gfx::px(ox, y, BR_MAX);
        }
      }
      display::flush();
      return true;
    }

    case FX_BEAT: {
      /* swells twice like a heartbeat: lub-dub */
      const uint16_t T = 900;
      if (t >= T) return false;
      int16_t s = 256;  // scale, 1/256
      if (t < 240) s += (int16_t)isin((uint32_t)t * 128 / 240) * 100 / 127;
      else if (t >= 300 && t < 520) s += (int16_t)isin((uint32_t)(t - 300) * 128 / 220) * 64 / 127;
      int8_t src[12];  // the same inverse mapping serves rows and columns
      for (uint8_t o = 0; o < 12; o++) src[o] = px2((int32_t)(2 * o - 11) * 256 / s);
      gfx::clear();
      for (uint8_t ox = 0; ox < 12; ox++) {
        for (uint8_t oy = 0; oy < 12; oy++) {
          if (lit(src[ox], src[oy])) gfx::px(ox, oy, BR_MAX);
        }
      }
      display::flush();
      return true;
    }

    case FX_BURST: {
      /* flies apart in a swirl, then pulls itself back together */
      const uint16_t T = 1300;
      if (t >= T) return false;
      uint8_t k = isin((uint32_t)t * 128 / T);   // 0 -> 127 -> 0
      uint16_t e = (uint16_t)k * k / 127;        // eased
      int16_t s = 256 + e * 5;                   // up to ~3.5x
      uint8_t ang = e >> 1;                      // up to a quarter turn
      int8_t c = icos(ang), sn = isin(ang);
      gfx::clear();
      for (uint8_t y = 0; y < 12; y++) {
        if (!pic[y]) continue;
        int16_t y2 = 2 * y - 11;
        for (uint8_t x = 0; x < 12; x++) {
          if (!((pic[y] >> x) & 1)) continue;
          int16_t x2 = 2 * x - 11;
          int16_t rx = (x2 * c - y2 * sn) >> 7;
          int16_t ry = (x2 * sn + y2 * c) >> 7;
          gfx::px(px2((int32_t)rx * s >> 8), px2((int32_t)ry * s >> 8), BR_MAX);
        }
      }
      display::flush();
      return true;
    }

    case FX_SPIN: {
      /* one full turn, easing in and out */
      const uint16_t T = 1100;
      if (t >= T) return false;
      uint8_t ang = 127 - icos((uint32_t)t * 128 / T);  // 0 .. ~1 turn
      int8_t c = icos(ang), sn = isin(ang);
      gfx::clear();
      for (uint8_t oy = 0; oy < 12; oy++) {
        int16_t y2 = 2 * oy - 11;
        for (uint8_t ox = 0; ox < 12; ox++) {
          int16_t x2 = 2 * ox - 11;
          int8_t sx = px2((x2 * c + y2 * sn) >> 7);
          int8_t sy = px2((y2 * c - x2 * sn) >> 7);
          if (lit(sx, sy)) gfx::px(ox, oy, BR_MAX);
        }
      }
      display::flush();
      return true;
    }
  }
  return false;
}

/* ------------------------------------------------------------------ *
 *  Public                                                             *
 * ------------------------------------------------------------------ */

void animate() {
  stop();
  if (album::isDrawing()) {
    fxBegin(fxNext);
    if (++fxNext >= NUM_FX) fxNext = 0;
    return;
  }
  uint8_t id = album::builtin();
  sBase = pgm_read_byte(&PICS[id].frame);
  sIdx = pgm_read_word(&PICS[id].step);
  sEnd = sIdx + pgm_read_byte(&PICS[id].nsteps);
  stepsOn = true;
  sUntil = millis();
}

void tick() {
  uint32_t now = millis();

  if (fxOn != NO_FX) {
    if (now - fxLast < FX_FRAME_MS) return;
    fxLast = now;
    if (fxFrame(fxOn, now - fxT0)) return;
    fxOn = NO_FX;
    if (!stepsOn) { still(); return; }
    sUntil = now;  // effect was a step of a built-in's show: carry on
  }

  if (!stepsOn || (int32_t)(now - sUntil) < 0) return;
  if (sIdx >= sEnd) {
    stepsOn = false;
    still();
    return;
  }

  uint8_t s[4];
  memcpy_P(s, STEPS[sIdx++], 4);
  if (s[0] & STEP_FX) {
    fxBegin(s[0] & STEP_FRAME);
    return;
  }
  uint16_t rows[12];
  memcpy_P(rows, FRAMES[sBase + (s[0] & STEP_FRAME)], sizeof(rows));
  gfx::clear();
  blit(rows, (int8_t)s[1], (int8_t)s[2], (s[0] & STEP_DIM) ? BR_DIM : BR_MAX, s[0] & STEP_MIRROR);
  display::flush();
  sUntil += s[3] * 10u;
}

void slide(int8_t dir) {
  stop();
  memcpy(snap, display::fb, N);  // whatever is on screen, mid-animation or not
  album::load(pic);
  uint32_t t = millis();
  for (uint8_t k = 0; k < sizeof(SLIDE_PATH); k++) {
    int8_t off = (int8_t)pgm_read_byte(&SLIDE_PATH[k]) * dir;
    memcpy(trail, display::fb, N);
    gfx::clear();
    blitSnap(off - 12 * dir, 0);
    blit(pic, off, 0, BR_MAX, false);
    flushSmeared();
    waitUntil(t += SLIDE_FRAME_MS);
  }
  still();
  animate();  // every new picture says hello; the next press cuts it short
}

void wake() {
  stop();
  album::load(pic);
  gfx::clear();
  uint32_t t = millis();
  for (uint8_t k = 0; k < sizeof(RISE_PATH); k++) {
    memcpy(trail, display::fb, N);
    gfx::clear();
    blit(pic, 0, (int8_t)pgm_read_byte(&RISE_PATH[k]), BR_MAX, false);
    flushSmeared();
    waitUntil(t += RISE_FRAME_MS);
  }
  still();
  animate();  // and say hello with its little show
}

void bedtime() {
  stop();
  memcpy(snap, display::fb, N);
  uint32_t t = millis();
  for (uint8_t k = 0; k < sizeof(SINK_PATH); k++) {
    memcpy(trail, display::fb, N);
    gfx::clear();
    blitSnap(0, (int8_t)pgm_read_byte(&SINK_PATH[k]));
    flushSmeared();
    waitUntil(t += SINK_FRAME_MS);
  }
  gfx::clear();
  display::flush();
}

}  // namespace show
