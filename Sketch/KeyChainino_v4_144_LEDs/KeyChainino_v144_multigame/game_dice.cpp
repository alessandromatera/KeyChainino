/*
 * DICE — tap any button to roll: the die tumbles, slows, then blinks
 * once as it settles on the answer. Input stays live while tumbling:
 * a tap queues an immediate re-roll and the A+B exit hold keeps
 * counting. No score, no game over: leave with the usual hold chord.
 *
 * Greyscale: dim die outline, bright pips.
 */
#include "display.h"
#include "games.h"
#include "gfx.h"
#include "input.h"

namespace dice {

/* Pip layout bits: TL=1 TR=2 ML=4 MR=8 BL=16 BR=32 C=64 (2x2 blocks) */
static const uint8_t FACES[6] PROGMEM = {
  64,                        // 1
  2 | 16,                    // 2
  2 | 64 | 16,               // 3
  1 | 2 | 16 | 32,           // 4
  1 | 2 | 64 | 16 | 32,      // 5
  1 | 2 | 4 | 8 | 16 | 32,   // 6
};

static uint8_t face;

static void drawFace(uint8_t f) {
  uint8_t bits = pgm_read_byte(&FACES[f]);
  gfx::clear();
  gfx::rect(0, 0, 12, 12, BR_DIM);  // die outline
  if (bits & 1) gfx::fillRect(2, 2, 2, 2, BR_MAX);   // TL
  if (bits & 2) gfx::fillRect(8, 2, 2, 2, BR_MAX);   // TR
  if (bits & 4) gfx::fillRect(2, 5, 2, 2, BR_MAX);   // ML
  if (bits & 8) gfx::fillRect(8, 5, 2, 2, BR_MAX);   // MR
  if (bits & 16) gfx::fillRect(2, 8, 2, 2, BR_MAX);  // BL
  if (bits & 32) gfx::fillRect(8, 8, 2, 2, BR_MAX);  // BR
  if (bits & 64) gfx::fillRect(5, 5, 2, 2, BR_MAX);  // C
  display::flush();
}

/* Tumble `steps` frames, decelerating. Input is polled while waiting, so
   a mid-tumble tap queues an immediate re-roll (full tumble) instead of
   vanishing; exitHold is left latched for the engine. Ends with a settle
   blink so the final face reads as the answer, not another tumble frame. */
static void roll(uint8_t steps) {
  bool again;
  do {
    again = false;
    for (uint8_t i = 0; i < steps; i++) {  // tumble, decelerating
      uint8_t f;
      do {
        f = random(6);
      } while (f == face);  // always visibly change
      face = f;
      drawFace(face);
      uint32_t t0 = millis();
      while (millis() - t0 < 40u + i * 22u) {
        input::poll();
        if (input::tapA() | input::tapB() | input::chordTap()) again = true;
        delay(10);
      }
    }
    steps = 10;  // queued re-rolls run the full tumble
  } while (again);
  /* settle blink: pips out for a beat, outline stays */
  gfx::clear();
  gfx::rect(0, 0, 12, 12, BR_DIM);
  display::flush();
  delay(60);
  drawFace(face);
}

void reset() {
  face = random(6);
  roll(6);  // shorter entry tumble: the toy is live sooner
}

uint8_t frame() {
  if (input::tapA() | input::tapB() | input::chordTap()) roll(10);
  return GAME_ON;  // exit is the hold chord, handled by the engine
}

}  // namespace dice
