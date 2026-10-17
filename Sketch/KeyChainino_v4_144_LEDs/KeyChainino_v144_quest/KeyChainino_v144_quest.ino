/*************************************************************************
  QUEST — AN ADVENTURE FOR KEYCHAININO 144
  https://www.keychainino.com/product/keychainino-144

  One game, one world: a 4x4 grid of rooms to explore on the 12x12
  matrix. Find the sword in its cave, hunt down two keys, unlock the
  dungeon, slay the 2x2 boss and claim the treasure.

  Controls:
    tap A          — turn 90° clockwise (a dim pixel shows your facing)
    tap / hold B   — step / walk forward
    tap A+B        — sword strike (once you've found it)
    hold A+B 1.2s  — save and sleep; KEEP holding ~3s more (a bar fills)
                     to wipe the save and start a fresh quest
    press A+B      — wake from sleep, back exactly where you left off

  The quest saves itself to EEPROM on every room change and event —
  pocket it mid-dungeon, wake it next week, keep playing. Death keeps
  your items and doors; you respawn at home with full hearts.

  Runs on the rethought multigame machine: greyscale charlieplex driver
  (compiled scanlines + BCM in the Timer2 ISR), zero-heap PROGMEM text
  scroller, everything constant in flash. Build for MiniCore:avr:328
  with clock=Internal 8MHz (board.h refuses anything else).

  created by Alessandro Matera
* ************************************************************************/

#include "board.h"
#include "display.h"
#include "input.h"
#include "power.h"
#include "quest.h"

/* Empty button ISRs: they exist only so INT0/INT1 can wake the MCU from
   power-down. All gameplay input is polled. */
ISR(INT1_vect) {}  // BUTTON A
ISR(INT0_vect) {}  // BUTTON B

void setup() {
  randomSeed(analogRead(A0));  // floating matrix pin, read before the ADC goes down

  display::begin();  // starts the scan, powers off unused peripherals

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);

  power::sleepUntilWake();  // sleep until the A+B power-on gesture

  quest::boot();  // restore the saved quest, or roll the title
}

void loop() {
  quest::tick();
}
