#include "input.h"

namespace input {

/* Timing (ms) */
#define DEBOUNCE_MS 15     // a level must hold this long to be believed
#define CHORD_GRACE_MS 50  // window to gather A+B into one chord

/* Button masks */
#define MASK_A 1
#define MASK_B 2
#define MASK_AB (MASK_A | MASK_B)

enum State : uint8_t {
  IDLE,
  PEND,   // one button down, might still become a chord
  HELD,   // its press is out; nothing more until it is released
  CHORD,  // both down: timing the hold
  DONE    // chord fired or let go early: nothing more until both are up
};

static State st;
static uint32_t stSince;
static uint8_t pendMask;     // the button that went down alone (PEND)
static uint8_t raw, stable;  // last sampled levels / debounced levels
static uint32_t rawSince;    // when raw last changed
static bool evLeft, evRight, evLong;

static uint8_t readButtons() {
  uint8_t m = 0;
  if (!digitalRead(BUTTON_A)) m |= MASK_A;
  if (!digitalRead(BUTTON_B)) m |= MASK_B;
  return m;
}

/* A lone press is confirmed: tell which button it was. */
static void single(uint8_t m) {
  if (m == MASK_A) evLeft = true;
  else evRight = true;
}

void reset() {
  st = IDLE;
  raw = stable = 0;
  rawSince = millis();
  evLeft = evRight = evLong = false;
}

void poll() {
  uint32_t now = millis();

  /* Debounce: a change in the raw levels is believed only once it has
     held steady for DEBOUNCE_MS (contact bounce is a few ms of chatter). */
  uint8_t m = readButtons();
  if (m != raw) {
    raw = m;
    rawSince = now;
  } else if (now - rawSince >= DEBOUNCE_MS) {
    stable = raw;
  }

  switch (st) {
    case IDLE:
      if (stable == MASK_AB) { st = CHORD; stSince = now; }
      else if (stable) { st = PEND; stSince = now; pendMask = stable; }
      break;

    case PEND:
      if (stable == MASK_AB) { st = CHORD; stSince = now; }
      else if (!stable) { single(pendMask); st = IDLE; }  // quick tap
      else if (now - stSince >= CHORD_GRACE_MS) { single(pendMask); st = HELD; }
      break;

    case HELD:
      if (stable == MASK_AB) { st = CHORD; stSince = now; }
      else if (!stable) st = IDLE;
      break;

    case CHORD:
      if (stable != MASK_AB) st = stable ? DONE : IDLE;  // let go early
      else if (now - stSince >= LONG_CHORD_MS) { evLong = true; st = DONE; }
      break;

    case DONE:
      if (!stable) st = IDLE;
      break;
  }
}

bool left() { bool e = evLeft; evLeft = false; return e; }
bool right() { bool e = evRight; evRight = false; return e; }
bool longChord() { bool e = evLong; evLong = false; return e; }

bool chording() { return st == CHORD; }
uint16_t chordMs() { return millis() - stSince; }

}  // namespace input
