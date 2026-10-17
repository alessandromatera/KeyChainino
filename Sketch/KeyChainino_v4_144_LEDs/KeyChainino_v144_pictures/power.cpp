#include "power.h"

#include <avr/eeprom.h>
#include <avr/power.h>
#include <avr/sleep.h>

#include "display.h"

/* The wake source is a pin CHANGE on either button (PCINT18 = PD2 = B,
   PCINT19 = PD3 = A), not the level-triggered INT0/INT1 earlier versions
   used. A level interrupt re-fires for as long as its pin is low, so a
   button squeezed in a pocket kept the MCU spinning at full current; a
   change interrupt fires once per edge and lets the part sleep again.
   (Sketches may still define empty INT0/INT1 ISRs: harmless, never armed.) */
ISR(PCINT2_vect) {}

namespace power {

static inline bool held(uint8_t pin) { return !digitalRead(pin); }

/* Both buttons down and staying down for ~20ms: a real chord, not a
   contact bouncing while the other finger lifts. Timers are off, so
   this paces itself with busy-wait delays. */
static bool chordConfirmed() {
  for (uint8_t i = 0; i < 10; i++) {
    if (!held(BUTTON_A) || !held(BUTTON_B)) return false;
    delayMicroseconds(2000);
  }
  return true;
}

void sleepUntilWake() {
  /* An EEPROM write takes ~3.4ms and EEPROM.update() returns as soon as
     it starts (e.g. a sketch saving its state right before bedtime). The
     datasheet doesn't promise a write completes in power-down, where the
     clocks stop: let it finish first. */
  eeprom_busy_wait();

  display::park();  // scan off, matrix Hi-Z: no LED can leak current

  /* Power-down stops these clocks anyway; gating them as well keeps
     millis() and the scan frozen during the brief awake checks between
     edges below (and in the simulator, which doesn't stop clocks on
     SLEEP). */
  power_timer0_disable();  // millis()
  power_timer2_disable();  // display scan clock

  PCMSK2 = _BV(PCINT18) | _BV(PCINT19);
  PCIFR = _BV(PCIF2);  // drop any edge left over from being awake
  PCICR = _BV(PCIE2);

  /* Arm only once both buttons have been seen released: A+B is also the
     hold-to-exit chord, so going down with it still pressed must not wake
     (= restart) straight away. Until then every edge just wakes us for a
     few microseconds to look, instead of spinning in a busy loop.

     The buttons are read with interrupts OFF: an edge landing after the
     read then stays pending in PCIF2 and the SLEEP below returns at once.
     (Read with interrupts on, that edge would run the empty ISR, clear
     the flag, and leave us asleep on a stale reading.) */
  bool armed = false;
  for (;;) {
    delayMicroseconds(2000);  // let the contacts settle
    cli();
    bool a = held(BUTTON_A), b = held(BUTTON_B);
    if (!a && !b) armed = true;
    else if (armed && a && b && chordConfirmed()) break;  // interrupts still off

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sei();       // the instruction after SEI always runs before a pending
    sleep_cpu(); // interrupt: an edge since the read wakes us straight away
    sleep_disable();
  }

  PCICR = 0;
  PCMSK2 = 0;
  PCIFR = _BV(PCIF2);
  sei();

  power_timer0_enable();
  power_timer2_enable();
  display::resume();
}

}  // namespace power
