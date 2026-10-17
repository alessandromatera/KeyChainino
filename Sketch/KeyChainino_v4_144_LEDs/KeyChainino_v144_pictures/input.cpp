#include "input.h"

namespace input {

/* Timing (ms) */
#define SAMPLE_MS 5        // debounce: a level counts once seen twice in a row
#define CHORD_GRACE 150    // a lone press waits this long for its partner
#define REPEAT_DELAY 650   // hold before auto-repeat starts
#define REPEAT_EVERY 450   // auto-repeat period: each picture stays readable
#define REPEAT_MAX 24      // then stop repeating (~11s): pocket protection

enum State : uint8_t {
  IDLE,
  PEND_A,       // A down, might still become a chord
  PEND_B,
  HELD_A,       // confirmed single press, auto-repeating
  HELD_B,
  WAIT_RELEASE  // chord fired: swallow everything until both are up
};

static State st = WAIT_RELEASE;  // boot conservative: eat a held chord
static uint32_t stSince, repeatAt, lastSample;
static uint8_t repeats;
static bool rawA, rawB, a, b;  // last raw sample, debounced levels
static bool evA, evB, evChord, evAny;

void flush() {
  /* Load the debouncer with what is pressed NOW, or the wake chord still
     under the child's fingers would look like a fresh press 5ms later. */
  rawA = a = !digitalRead(BUTTON_A);
  rawB = b = !digitalRead(BUTTON_B);
  st = WAIT_RELEASE;
  evA = evB = evChord = evAny = false;
}

static void sample() {
  bool ra = !digitalRead(BUTTON_A);
  bool rb = !digitalRead(BUTTON_B);
  if (ra == rawA) a = ra;
  if (rb == rawB) b = rb;
  rawA = ra;
  rawB = rb;
}

void poll() {
  uint32_t now = millis();
  if (now - lastSample < SAMPLE_MS) return;
  lastSample = now;
  sample();

  switch (st) {
    case IDLE:
      if (a && b) { evChord = evAny = true; st = WAIT_RELEASE; }
      else if (a) { st = PEND_A; stSince = now; }
      else if (b) { st = PEND_B; stSince = now; }
      break;

    case PEND_A:
    case PEND_B: {
      bool mine = (st == PEND_A) ? a : b;
      bool other = (st == PEND_A) ? b : a;
      if (other) { evChord = evAny = true; st = WAIT_RELEASE; }
      else if (!mine || now - stSince >= CHORD_GRACE) {
        if (st == PEND_A) evA = true; else evB = true;
        evAny = true;
        if (!mine) st = IDLE;  // quick tap: done
        else {
          st = (st == PEND_A) ? HELD_A : HELD_B;
          repeatAt = now + REPEAT_DELAY;
          repeats = 0;
        }
      }
      break;
    }

    case HELD_A:
    case HELD_B: {
      bool mine = (st == HELD_A) ? a : b;
      bool other = (st == HELD_A) ? b : a;
      if (other) { evChord = evAny = true; st = WAIT_RELEASE; }
      else if (!mine) st = IDLE;
      else if (repeats < REPEAT_MAX && (int32_t)(now - repeatAt) >= 0) {
        if (st == HELD_A) evA = true; else evB = true;
        evAny = true;
        repeats++;
        repeatAt = now + REPEAT_EVERY;
      }
      break;
    }

    case WAIT_RELEASE:
      if (!a && !b) st = IDLE;
      break;
  }
}

bool tapA() { bool e = evA; evA = false; return e; }
bool tapB() { bool e = evB; evB = false; return e; }
bool chord() { bool e = evChord; evChord = false; return e; }
bool activity() { bool e = evAny; evAny = false; return e; }

}  // namespace input
