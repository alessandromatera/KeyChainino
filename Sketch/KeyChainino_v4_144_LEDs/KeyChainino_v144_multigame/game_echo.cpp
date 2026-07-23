/*
 * ECHO — watch the pads flash, repeat with A (left) / B
 * (right). Each level adds a step and plays back faster; wrong press or 5
 * idle seconds ends it. Sequence: one bit per step, 64 steps max in 8
 * bytes.
 *
 * Greyscale rethink: during your turn both pads glow dim as touch guides;
 * flashes (playback and your echoes) are bright. No more guessing where
 * the pads are on a black screen.
 */
#include "display.h"
#include "games.h"
#include "gfx.h"
#include "input.h"

namespace echo {

#define ECHO_MAX 64
#define SHOW_BASE_MS 300
#define SHOW_MIN_MS 130
#define SHOW_GAP_MS 130
#define SHOW_FIRST_GAP_MS 400  // longer dark beat before a round's FIRST step:
                               // step one is easy to miss right after the pause
#define INPUT_TIMEOUT_MS 5000
#define FEEDBACK_MS 140

enum Phase : uint8_t { SHOW_ON, SHOW_OFF, AWAIT };

static uint8_t seq[ECHO_MAX / 8];  // bit per step: 0 = left, 1 = right
static uint8_t level;               // current sequence length
static uint8_t step;                // position within the sequence
static Phase phase;
static uint16_t phaseAcc;

static inline bool seqBit(uint8_t i) { return (seq[i >> 3] >> (i & 7)) & 1; }

static inline void seqPush(uint8_t i, bool right) {
  if (right) seq[i >> 3] |= (uint8_t)(1 << (i & 7));
  else seq[i >> 3] &= (uint8_t)~(1 << (i & 7));
}

static uint16_t showMs() {
  int16_t ms = SHOW_BASE_MS - 8 * level;
  return ms < SHOW_MIN_MS ? SHOW_MIN_MS : (uint16_t)ms;
}

/* pads: left = A side, right = B side. lit = 0 none, 1 left, 2 right;
   guides draws the unlit pads dim (player's turn). */
static void drawPads(uint8_t lit, bool guides) {
  gfx::clear();
  uint8_t base = guides ? BR_DIM : BR_OFF;
  gfx::fillRect(1, 3, 4, 6, lit == 1 ? BR_MAX : base);
  gfx::fillRect(7, 3, 4, 6, lit == 2 ? BR_MAX : base);
}

static void extendAndShow() {
  seqPush(level, random(2));
  level++;
  step = 0;
  phase = SHOW_OFF;  // dark pause before playback starts
  phaseAcc = 0;
  drawPads(0, false);
}

void reset() {
  level = 0;
  extendAndShow();
}

uint8_t frame() {
  if (!input::chordHeld()) phaseAcc += FRAME_MS;  // frozen during the exit chord

  switch (phase) {
    case SHOW_OFF:
      /* step==0 here means we're in the dark beat before a round's first
         flash: give that one a longer gap, it's easy to miss. */
      if (phaseAcc >= (step == 0 ? SHOW_FIRST_GAP_MS : SHOW_GAP_MS)) {
        drawPads(seqBit(step) ? 2 : 1, false);
        phase = SHOW_ON;
        phaseAcc = 0;
      }
      break;

    case SHOW_ON:
      if (phaseAcc >= showMs()) {
        step++;
        if (step >= level) {  // playback finished: player's turn
          step = 0;
          phase = AWAIT;
          drawPads(0, true);  // dim guides on
          input::flush();     // drop taps made during playback
        } else {
          phase = SHOW_OFF;
          drawPads(0, false);
        }
        phaseAcc = 0;
      }
      break;

    case AWAIT: {
      bool pressedA = input::tapA();
      bool pressedB = input::tapB();
      input::chordTap();  // ambiguous here: discard

      if (pressedA || pressedB) {
        bool right = pressedB;
        input::flush();  // one physical press = one sequence step: swallow the
                         // auto-repeat tail of a slow release before it lands
                         // as the next (wrong) entry
        drawPads(right ? 2 : 1, true);  // echo the press
        display::flush();
        delay(FEEDBACK_MS);
        drawPads(0, true);
        if (right != seqBit(step)) {
          engine::score = level > 0 ? level - 1 : 0;
          drawPads(seqBit(step) ? 2 : 1, true);  // errors should teach: flash
          display::flush();                      // the pad you SHOULD have hit
          delay(300);
          return GAME_LOSE;
        }
        step++;
        phaseAcc = 0;
        if (step >= level) {  // sequence repeated
          if (level >= ECHO_MAX) {
            engine::score = level;
            return GAME_WIN;
          }
          display::flush();
          delay(250);
          extendAndShow();
        }
      } else if (phaseAcc >= INPUT_TIMEOUT_MS) {
        engine::score = level > 0 ? level - 1 : 0;
        return GAME_LOSE;
      }
      break;
    }
  }

  return GAME_ON;
}

}  // namespace echo
