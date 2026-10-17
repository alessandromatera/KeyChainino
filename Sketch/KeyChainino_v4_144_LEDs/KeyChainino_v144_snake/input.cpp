#include "input.h"

namespace input {

/* Timing (ms) */
#define CHORD_GRACE 60     // window to gather A+B into one chord
#define CHORD_TAP_MAX 350  // chord released within this = chord tap
#define CHORD_HOLD 1200    // chord held this long = exit request
#define REPEAT_DELAY 300   // before auto-repeat starts
#define REPEAT_EVERY 150   // auto-repeat period

enum State : uint8_t {
  IDLE,
  PEND_A,       // A down, might become a chord
  PEND_B,
  HELD_A,       // confirmed single press, auto-repeating
  HELD_B,
  CHORD,        // both down: tap or hold pending
  WAIT_RELEASE  // swallow everything until both buttons are up
};

static State st = WAIT_RELEASE;  // boot conservative: eat the power-on chord
static uint32_t stSince;
static uint32_t repeatAt;
static bool evA, evB, evChord, evExit, exitFired;

void flush() {
  st = WAIT_RELEASE;
  evA = evB = evChord = evExit = exitFired = false;
}

bool downA() { return !digitalRead(BUTTON_A); }
bool downB() { return !digitalRead(BUTTON_B); }

void poll() {
  bool a = downA();
  bool b = downB();
  uint32_t now = millis();

  switch (st) {
    case IDLE:
      if (a && b) { st = CHORD; stSince = now; exitFired = false; }
      else if (a) { st = PEND_A; stSince = now; }
      else if (b) { st = PEND_B; stSince = now; }
      break;

    case PEND_A:
      if (b) { st = CHORD; stSince = now; exitFired = false; }
      else if (!a) { evA = true; st = IDLE; }  // quick tap
      else if (now - stSince >= CHORD_GRACE) {
        evA = true; st = HELD_A; repeatAt = now + REPEAT_DELAY;
      }
      break;

    case PEND_B:
      if (a) { st = CHORD; stSince = now; exitFired = false; }
      else if (!b) { evB = true; st = IDLE; }
      else if (now - stSince >= CHORD_GRACE) {
        evB = true; st = HELD_B; repeatAt = now + REPEAT_DELAY;
      }
      break;

    case HELD_A:
      if (b) { st = CHORD; stSince = now; exitFired = false; }
      else if (!a) st = IDLE;
      else if ((int32_t)(now - repeatAt) >= 0) {
        evA = true; repeatAt = now + REPEAT_EVERY;
      }
      break;

    case HELD_B:
      if (a) { st = CHORD; stSince = now; exitFired = false; }
      else if (!b) st = IDLE;
      else if ((int32_t)(now - repeatAt) >= 0) {
        evB = true; repeatAt = now + REPEAT_EVERY;
      }
      break;

    case CHORD:
      if (!a && !b) {
        if (!exitFired && now - stSince <= CHORD_TAP_MAX) evChord = true;
        st = IDLE;
        exitFired = false;
      } else if (!a || !b) {  // one finger lifted
        if (!exitFired && now - stSince <= CHORD_TAP_MAX) evChord = true;
        st = WAIT_RELEASE;
      } else if (!exitFired && now - stSince >= CHORD_HOLD) {
        evExit = true;
        exitFired = true;
        st = WAIT_RELEASE;
      }
      break;

    case WAIT_RELEASE:
      if (!a && !b) st = IDLE;
      break;
  }
}

bool tapA() { bool e = evA; evA = false; return e; }
bool tapB() { bool e = evB; evB = false; return e; }
bool chordTap() { bool e = evChord; evChord = false; return e; }
bool exitHold() { bool e = evExit; evExit = false; return e; }

bool idle() { return st == IDLE && !evA && !evB && !evChord && !evExit; }

}  // namespace input
