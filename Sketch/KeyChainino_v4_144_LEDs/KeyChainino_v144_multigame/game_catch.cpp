/*
 * CATCH — slide the 2px basket under the falling drops (up to two at
 * once). Each catch speeds things up; three misses end it.
 *
 * Greyscale: drops mid while falling, basket bright (flashes mid ~120ms
 * on a catch — non-blocking, a second drop may still be falling). A miss
 * blinks a splash one row above the basket and shows the miss count as dim
 * pips on the free top row during the pause.
 */
#include "display.h"
#include "games.h"
#include "gfx.h"
#include "input.h"

namespace catcher {

#define MAX_ITEMS 2
#define ITEM_NONE 0xFF
#define FALL_START_MS 260
#define FALL_MIN_MS 120
#define FALL_STEP_MS 3
#define SPAWN_MS 700  // was 900: fixes the slow, lull-heavy early game
#define BASKET_MS 40  // was 80: at the hardest fall speed (FALL_MIN_MS+
                       // FALL_STEP_MS = 123ms/row) the old basket only beat
                       // the drop by ~1.5x once reaction time was spent, so
                       // a cross-board dash barely made it; 40ms doubles
                       // both tap latency and hold-repeat rate
#define BASKET_Y (MATRIX_H - 1)

static uint8_t items[MAX_ITEMS];  // packed x<<4|y
static uint8_t basketX;           // left pixel
static uint8_t misses;
static uint8_t flashMs;           // catch flash countdown (non-blocking)
static uint16_t fallMs, fallAcc, spawnAcc, basketAcc;

static void draw() {
  gfx::clear();
  uint8_t bb = flashMs ? BR_MID : BR_MAX;  // brief mid flash on a catch
  gfx::px(basketX, BASKET_Y, bb);
  gfx::px(basketX + 1, BASKET_Y, bb);
  for (uint8_t i = 0; i < MAX_ITEMS; i++) {
    if (items[i] != ITEM_NONE) {
      gfx::px(items[i] >> 4, items[i] & 0x0F, BR_MID);
    }
  }
}

void reset() {
  for (uint8_t i = 0; i < MAX_ITEMS; i++) items[i] = ITEM_NONE;
  items[0] = (uint8_t)(random(0, 12) << 4);  // first drop falls immediately
  basketX = MATRIX_W / 2 - 1;
  misses = 0;
  fallMs = FALL_START_MS;
  fallAcc = spawnAcc = basketAcc = 0;
  flashMs = 0;
  draw();
}

uint8_t frame() {
  if (flashMs) flashMs = (flashMs > FRAME_MS) ? flashMs - FRAME_MS : 0;

  basketAcc += FRAME_MS;
  if (basketAcc >= BASKET_MS) {
    if (input::downB() && !input::downA() && basketX < MATRIX_W - 2) {
      basketX++;
      basketAcc = 0;
    } else if (input::downA() && !input::downB() && basketX > 0) {
      basketX--;
      basketAcc = 0;
    }
  }

  /* The exit chord freezes the world: it must not kill you mid-hold. */
  if (!input::chordHeld()) spawnAcc += FRAME_MS;
  if (spawnAcc >= SPAWN_MS) {
    spawnAcc = 0;
    for (uint8_t i = 0; i < MAX_ITEMS; i++) {
      if (items[i] == ITEM_NONE) {
        items[i] = (uint8_t)(random(0, 12) << 4);  // y = 0
        break;
      }
    }
  }

  if (!input::chordHeld()) fallAcc += FRAME_MS;
  if (fallAcc >= fallMs) {
    fallAcc = 0;
    for (uint8_t i = 0; i < MAX_ITEMS; i++) {
      if (items[i] == ITEM_NONE) continue;
      uint8_t x = items[i] >> 4;
      uint8_t y = (items[i] & 0x0F) + 1;
      if (y >= BASKET_Y) {  // reached the basket row
        if (x == basketX || x == basketX + 1) {
          engine::score++;
          if (fallMs > FALL_MIN_MS + FALL_STEP_MS) fallMs -= FALL_STEP_MS;
          flashMs = 120;  // basket flash (counted down per frame, no delay:
                          // a second drop may still be falling)
        } else {
          misses++;
          gfx::px(x, BASKET_Y - 1, BR_MAX);  // splash one row up, not the basket
          for (uint8_t m = 0; m < misses; m++) gfx::px(m, 0, BR_DIM);  // miss pips
          display::flush();
          delay(80);
          if (misses >= 3) return GAME_LOSE;
        }
        items[i] = ITEM_NONE;
      } else {
        items[i] = (uint8_t)((x << 4) | y);
      }
    }
  }

  draw();
  return GAME_ON;
}

}  // namespace catcher
