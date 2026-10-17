/*************************************************************************
  CONTA PUNTI — STITCH COUNTER FIRMWARE FOR KEYCHAININO 144
  https://www.keychainino.com/product/keychainino-144

  A stitch/row counter for knitting and sewing. The board sleeps between
  stitches; any button wakes it.

    press B (right)       -> count +1: the old number rolls up and out,
                             the new one rolls in, like an odometer
    press A (left)        -> just show the number, no roll
    hold A and B for 2s   -> reset the count to zero. The count dims
                             and a ring fills clockwise around the edge
                             while they are held; when it closes, the
                             count rolls back down to 0. Let go before
                             that and nothing changes (the count is
                             shown).

  One or two digits are shown static and centered; from 100 the number
  scrolls through once (a +1 first rolls its last two digits, then
  scrolls the whole number). A static number stays on for SHOW_MS after the
  last press, then the board powers down (~1uA). Presses while the number
  is still on screen count too, so quick successive stitches are never
  lost. Holding a button counts once: no auto-repeat.

  Every change of the count is stored in EEPROM (wear-levelled, see
  save.h), so it survives a flat or swapped battery: the board picks up
  where it left off.

  The machine underneath is the rethought edition's: 4-level greyscale
  charlieplex driver (compiled scanlines + binary code modulation, the
  Timer2 ISR only writes port registers), zero heap, digits streamed from
  flash.

  created by Alessandro Matera
* ************************************************************************/

#include "board.h"
#include "display.h"
#include "input.h"
#include "power.h"
#include "save.h"
#include "text.h"

#define SHOW_MS 1000      // a static number stays on this long after the last press
#define COUNT_MAX 9999    // then the count wraps to 0
#define STATIC_BELOW 100  // 1-2 digits fit the matrix; 3+ scroll

/* The reset ring runs around the edge of the matrix. One or two digits
   (columns 1..10, rows 2..8) never touch it. */
#define RING_LEN (2 * (MATRIX_W + MATRIX_H) - 4)  // 44 pixels

/* Empty button ISRs: they exist only so INT0/INT1 can wake the MCU from
   power-down. All input is polled. */
ISR(INT1_vect) {}  // BUTTON A
ISR(INT0_vect) {}  // BUTTON B

static uint16_t count;  // mirrored in EEPROM by save::store()
static char digits[6];  // utoa buffer (any uint16); the scroller streams from it
static char before[6];  // the count before a +1: where the roll starts

/* Put the count on screen. Returns when the display may go dark: after
   SHOW_MS for a static number; right away for a scroll, which runs its
   course on its own and leaves the screen blank. */
static uint32_t show() {
  utoa(count, digits, 10);
  if (count < STATIC_BELOW) {
    text::draw(digits);
    return millis() + SHOW_MS;
  }
  display::clear();
  text::start(digits);
  return millis();
}

/* What a roll can show: the last two digits (3+ don't fit the screen). */
static const char* lastTwo(const char* s) {
  size_t n = strlen(s);
  return n > 2 ? s + n - 2 : s;
}

/* Change the count to v with an odometer roll: up for a +1, down for a
   reset. Stored first, before lighting up: the battery is least loaded. */
static void rollCount(uint16_t v, bool down, uint8_t fromLevel) {
  utoa(count, before, 10);
  count = v;
  save::store(count);
  utoa(count, digits, 10);
  text::roll(lastTwo(before), lastTwo(digits), down, fromLevel);
}

/* Pixel i of the edge ring, counted clockwise from the top center. */
static uint8_t* ringPixel(uint8_t i) {
  uint8_t p = (i + MATRIX_W / 2) % RING_LEN;  // from the top-left corner
  uint8_t x, y;
  if (p < MATRIX_W) {                               // top, rightwards
    x = p;
    y = 0;
  } else if (p < MATRIX_W + MATRIX_H - 1) {         // right side, down
    x = MATRIX_W - 1;
    y = p - (MATRIX_W - 1);
  } else if (p < 2 * MATRIX_W + MATRIX_H - 2) {     // bottom, leftwards
    x = 2 * MATRIX_W + MATRIX_H - 3 - p;
    y = MATRIX_H - 1;
  } else {                                          // left side, up
    x = 0;
    y = RING_LEN - p;
  }
  return &display::fb[x * MATRIX_H + y];
}

/* Reset in progress: the count dimmed (it is about to go) inside a dim
   ring whose first `lit` pixels are bright. The reset fires when the
   ring closes. */
static void drawRing(uint8_t lit) {
  text::stop();  // the ring replaces whatever was animating
  utoa(count, digits, 10);
  display::clear();
  text::paint(lastTwo(digits), BR_DIM);
  for (uint8_t i = 0; i < RING_LEN; i++) {
    *ringPixel(i) = i < lit ? BR_MAX : BR_DIM;
  }
  display::flush();
}

void setup() {
  display::begin();  // starts the scan, powers off unused peripherals

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);

  count = save::load();
  if (count > COUNT_MAX) count = 0;
}

void loop() {
  power::sleepUntilWake();  // a button is down when this returns
  input::reset();

  uint32_t offAt = millis() + SHOW_MS;  // guard: a wake that never resolves
                                        // into a gesture still goes back
                                        // to sleep
  uint32_t lastStep = millis();
  uint8_t ring = 0;     // reset ring on screen: its bright pixels (0 = none)
  bool rolled = false;  // a +1 roll is running; show() the count after it
  for (;;) {
    input::poll();
    if (input::right()) {  // the roll tells a count from a mere look (left)
      rollCount(count < COUNT_MAX ? count + 1 : 0, false, BR_MAX);
      rolled = true;
      lastStep = millis();  // the old number gets a full frame in place
    } else if (input::left()) {
      rolled = false;
      offAt = show();
    } else if (input::longChord()) {  // the ring closed: rewind to 0
      rollCount(0, true, BR_DIM);
      rolled = true;
      lastStep = millis();
      ring = 0;
    }

    /* A+B held: fill the ring, and stay up until the hold resolves. */
    if (input::chording()) {
      uint8_t lit = 1 + (uint32_t)input::chordMs() * RING_LEN / LONG_CHORD_MS;
      if (lit > RING_LEN) lit = RING_LEN;
      if (lit != ring) drawRing(ring = lit);
      continue;
    }
    if (ring) {  // let go before the reset: nothing changed, show the count
      ring = 0;
      rolled = false;
      offAt = show();
    }

    uint32_t now = millis();
    if (text::active()) {  // animating: pace it, and stay up until it ends
      if (now - lastStep >= text::stepMs()) {
        lastStep = now;
        text::step();
      }
    } else if (rolled) {  // the roll has landed: hold it, or scroll 3+ digits
      rolled = false;
      offAt = show();
    } else if ((int32_t)(now - offAt) >= 0) {
      break;
    }
  }

  /* Blank before sleeping: the scan resumes on wake with whatever table
     was last flushed, and the old number must not flash before the new. */
  display::clear();
  display::flush();
}
