/*************************************************************************
  MULTI-GAME FIRMWARE FOR KEYCHAININO 144 — RETHOUGHT EDITION
  https://www.keychainino.com/product/keychainino-144

  Same two-button language as always, now seventeen games — new machine
  underneath:

    * 4-level greyscale display. The charlieplex driver was rewritten
      around compiled scanlines + binary code modulation: the Timer2 ISR
      only writes port registers, and every pixel has a brightness 0..3.
      Games use it: walls and scenery are dim, you and the action are
      bright.
    * Zero heap. The old text scroller malloc()ed (and free()d inside an
      ISR); the new one streams glyphs from flash. Score scrolls can be
      skipped with any button.
    * A real game engine. Games are two functions (reset + one fixed
      50Hz frame step); the engine owns the intro/outro faces, score
      scrolls, EEPROM records, display flushing and the universal exit.
    * Everything constant lives in PROGMEM: game table, icons (packed
      12-word bitmaps, 6x smaller), names, fonts. Timer1 is not used by
      anything and stays powered off.

  V1.1 is a playability pass on the same machine:

    * The world freezes while the exit chord is held — the 1.2s A+B hold
      used to keep killing players mid-gesture.
    * Press-edge controls where latency mattered (JUMP, FLAP) and one
      action per physical press where auto-repeat used to cheat or
      double-fire (TOWER, MUNCHER, ECHO, WHACK).
    * Fairness: MUNCHER no longer spawns facing a wall, INVADERS shots
      can't tunnel through low formations, speed floors respect human
      reaction time, random-walk pipe gaps, speed-scaled cactus gaps.
    * Feedback: catch/kill/cut/death flashes where events were silent,
      visible lives/strikes/misses pips, danger telegraphs (BR_MID) in
      FROG, RACER, INVADERS and STEROIDS.

  The games:
    BRICKS   — A/B move the paddle
    SNAKE    — A/B turn left/right
    BLOCKS   — A/B shift, tap A+B together to rotate
    INVADERS — A/B move the ship, cannon auto-fires
    TENNIS   — A/B move the paddle, beat the AI up top
    TOWER    — any button locks the sliding row; reach the top
    FROG     — A hop forward, B hop back; dodge the traffic
    FLAP     — any button flaps; clear the pipe gaps
    RACER    — A/B steer through the scrolling tunnel
    ECHO     — watch the pads flash, repeat with A (left) / B (right)
    JUMP     — any button jumps the cacti
    CATCH    — A/B slide the basket under falling drops
    WHACK    — mole pops left or right: hit the matching button
    DICE     — any button rolls the die (exit with the hold chord)
    LIFE     — Conway's Life; any button reseeds (exit with the hold chord)
    MUNCHER  — A/B turn left/right; eat the dots, dodge the ghost
    STEROIDS — A/B strafe, ship auto-fires

  Everywhere: hold A+B ~1.2s to return to the menu.
  Power on as always: press A+B together while asleep.

  created by Alessandro Matera
* ************************************************************************/

#include "board.h"
#include "display.h"
#include "engine.h"
#include "icons.h"
#include "input.h"
#include "menu.h"
#include "power.h"
#include "save.h"

/* Empty button ISRs: they exist only so INT0/INT1 can wake the MCU from
   power-down. All gameplay input is polled. */
ISR(INT1_vect) {}  // BUTTON A
ISR(INT0_vect) {}  // BUTTON B

void setup() {
  randomSeed(analogRead(A0));  // floating matrix pin, read before the ADC goes down

  display::begin();  // starts the scan, powers off unused peripherals

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);

  save::begin(engine::numGames());
  menu::begin();

  power::sleepUntilWake();  // sleep until the A+B power-on gesture

  engine::showFaceP(KeyChaininoFace, 500);
  menu::enter();
}

void loop() {
  input::poll();
  int8_t sel = menu::tick();
  if (sel >= 0) {
    engine::runGame((uint8_t)sel);  // blocks for the whole round
    menu::enter();
  }
}
