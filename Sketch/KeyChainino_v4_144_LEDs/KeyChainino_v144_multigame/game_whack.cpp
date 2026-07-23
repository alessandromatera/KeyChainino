/*
 * WHACK — two dim hole markers sit on screen at all times, left and
 * right. After a random blank pause the mole fills one of them; hit the
 * matching button (A = left, B = right) before it burrows. Each hit
 * shortens the window; wrong button, an A+B chord, or timeout = strike,
 * 3 strikes and it's over. Strikes show as bright pips on the free top
 * row, and the mole blinks dim just before it burrows so the timeout
 * never feels arbitrary. A press only counts if it starts after the pop
 * (input::flush at pop time) — no pre-held anticipation strikes.
 */
#include "display.h"
#include "games.h"
#include "gfx.h"
#include "input.h"

namespace whack {

#define WINDOW_START_MS 1200
#define WINDOW_MIN_MS 450
#define WINDOW_STEP_MS 25
#define PAUSE_MIN_MS 300
#define PAUSE_MAX_MS 900
#define BURROW_WARN_MS 200   // mole blinks dim for the last this-many ms of its window
#define BURROW_BLINK_MS 100  // dim/bright toggle period during the warning

/* Fixed hole positions (top-left of the 2x2 mole footprint), one per
 * button/half. Same row, mirrored columns — the menu icon shows exactly
 * this: a mole block on one side, an empty ring on the other. */
#define HOLE_Y 5
#define HOLE_LX 2
#define HOLE_RX 8

enum Phase : uint8_t { PAUSE, MOLE };

static Phase phase;
static bool moleRight;
static uint16_t windowMs, pauseMs;
static uint16_t phaseAcc;
static uint8_t strikes;

/* Both holes as dim rings, drawn a cell larger than the mole all round —
 * scenery, so BR_DIM, always visible whether or not the mole is up. The
 * free top row carries the strike pips (BR_MID): without them strikes are
 * invisible between events and game-over feels arbitrary. */
static void drawHoles() {
  gfx::rect(HOLE_LX - 1, HOLE_Y - 1, 4, 4, BR_DIM);
  gfx::rect(HOLE_RX - 1, HOLE_Y - 1, 4, 4, BR_DIM);
  for (uint8_t i = 0; i < strikes; i++) gfx::px(i, 0, BR_MID);
}

static uint8_t strike() {
  strikes++;
  gfx::clear();  // strike feedback: brief X flash
  gfx::line(2, 2, 9, 9, BR_MAX);
  gfx::line(9, 2, 2, 9, BR_MAX);
  display::flush();
  delay(250);
  gfx::clear();
  display::flush();
  return (strikes >= 3) ? GAME_LOSE : GAME_ON;
}

static void startPause() {
  phase = PAUSE;
  pauseMs = random(PAUSE_MIN_MS, PAUSE_MAX_MS + 1);
  phaseAcc = 0;
  gfx::clear();
  drawHoles();  // holes stay put between pops, per the icon
  input::flush();  // early taps don't carry into the next mole
}

static void popMole() {
  moleRight = random(2);
  gfx::clear();
  drawHoles();  // the other hole stays visible, empty, while this fills
  gfx::fillRect(moleRight ? HOLE_RX : HOLE_LX, HOLE_Y, 2, 2, BR_MAX);
  phase = MOLE;
  phaseAcc = 0;
  input::flush();  // the press must START after the pop: a button held in
                   // anticipation otherwise auto-repeats into an instant strike
}

void reset() {
  strikes = 0;
  windowMs = WINDOW_START_MS;
  startPause();
}

uint8_t frame() {
  if (!input::chordHeld()) phaseAcc += FRAME_MS;  // frozen during the exit chord

  switch (phase) {
    case PAUSE:
      input::tapA();  // taps during the blank are swallowed
      input::tapB();
      input::chordTap();
      if (phaseAcc >= pauseMs) popMole();
      break;

    case MOLE: {
      bool pressedA = input::tapA();
      bool pressedB = input::tapB();
      bool chord = input::chordTap();  // a fat-fingered A+B is an immediate
                                       // strike, not a discarded event that
                                       // silently becomes a timeout strike

      if (chord) {
        if (strike() == GAME_LOSE) return GAME_LOSE;
        startPause();
      } else if (pressedA || pressedB) {
        if (pressedB == moleRight && pressedA != pressedB) {
          engine::score++;
          if (windowMs > WINDOW_MIN_MS + WINDOW_STEP_MS) {
            windowMs -= WINDOW_STEP_MS;
          }
          startPause();
        } else {
          if (strike() == GAME_LOSE) return GAME_LOSE;
          startPause();
        }
      } else if (phaseAcc >= windowMs) {  // burrowed away
        if (strike() == GAME_LOSE) return GAME_LOSE;
        startPause();
      } else if (phaseAcc >= windowMs - BURROW_WARN_MS) {
        /* burrow warning: the mole blinks dim for the last stretch of its
           window, so the timeout never comes out of nowhere */
        gfx::fillRect(moleRight ? HOLE_RX : HOLE_LX, HOLE_Y, 2, 2,
                      ((phaseAcc / BURROW_BLINK_MS) & 1) ? BR_DIM : BR_MAX);
      }
      break;
    }
  }

  return GAME_ON;
}

}  // namespace whack
