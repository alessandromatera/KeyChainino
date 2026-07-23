#include "power.h"

#include <avr/power.h>
#include <avr/sleep.h>

#include "display.h"

namespace power {

void sleepUntilWake() {
  display::park();  // scan off, matrix Hi-Z: no LED can leak current

  /* If we got here from an A+B hold, wait for the chord to be released:
     A+B is also the wake gesture, so going down with the buttons still
     pressed would wake (= restart) immediately. Screen is already dark
     (parked), so to the player the hold simply powers the board off. */
  while (!digitalRead(BUTTON_A) || !digitalRead(BUTTON_B)) {}
  delay(30);  // release debounce: a bounce here would re-wake instantly

  power_timer0_disable();  // millis()
  power_timer2_disable();  // display scan clock

  /* INT0/INT1 wake the MCU; their ISRs (in the .ino) are empty. */
  bitSet(EIMSK, INT0);
  bitSet(EIMSK, INT1);

  /* Buttons idle high (pull-ups): sleep until both read low = pressed. */
  while (digitalRead(BUTTON_B) || digitalRead(BUTTON_A)) {
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    cli();  // timed sequence below must not be interrupted
    sleep_enable();
    MCUCR = bit(BODS) | bit(BODSE);  // brown-out detector off while asleep
    MCUCR = bit(BODS);
    sei();
    sleep_cpu();
    sleep_disable();
  }

  bitClear(EIMSK, INT0);
  bitClear(EIMSK, INT1);

  power_timer0_enable();
  power_timer2_enable();
  display::resume();
}

}  // namespace power
