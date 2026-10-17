/*************************************************************************
  BUBBLE — STANDALONE FIRMWARE FOR KEYCHAININO 144
  https://www.keychainino.com/product/keychainino-144

  The rethought multigame engine with exactly one game installed and no
  menu: power on and you are playing.

    BUBBLE — a bubble shooter. Tap A / tap B: rotate
    the aim, tap A+B together: fire. The bubble bounces off the walls and
    sticks to the cluster; 3 or more of the same shade pop, whatever is
    left hanging drops. Every 6 landed shots the ceiling pushes a new row
    down. Clear the board to win; if the cluster reaches the bottom row
    you lose.

  The machine underneath is the rethought edition's: 4-level greyscale
  charlieplex driver (compiled scanlines + binary code modulation, the
  Timer2 ISR only writes port registers), zero heap, fixed 50Hz timestep,
  score scroll streamed from flash, high score in EEPROM.

  Flow: A+B together while asleep powers on. Each round ends with the
  face, the score scroll and the record scroll (RECORD! when this round
  just set it, "RECORD n" otherwise — the best lives in EEPROM, so it
  survives battery swaps; any button skips the scrolls). Then the board
  sleeps (~1uA). Holding A+B ~1.2s powers off mid-round (screen blanks;
  sleep begins when you release), and 30s without a button press powers
  off too. Waking with A+B always starts a fresh round.

  created by Alessandro Matera
* ************************************************************************/

#include "board.h"
#include "display.h"
#include "engine.h"
#include "input.h"
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

  power::sleepUntilWake();  // sleep until the A+B power-on gesture
}

void loop() {
  engine::runGame(0);  // intro face, the round, the outro — blocks throughout
  power::sleepUntilWake();
}
