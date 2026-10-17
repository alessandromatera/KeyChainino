/*
 * power.h — power-down sleep, woken by any button.
 * ~1uA in sleep: BOD off, all timers gated, every LED line Hi-Z.
 */
#ifndef RETHINK_POWER_H
#define RETHINK_POWER_H

#include "board.h"

namespace power {

/* Blank the matrix and power-down until EITHER button is pressed.
   Returns with the display scanning again and the button still down. */
void sleepUntilWake();

}  // namespace power

#endif
