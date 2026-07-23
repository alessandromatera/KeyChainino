/*
 * power.h — power-down sleep, woken by the classic A+B gesture.
 * ~1uA in sleep: BOD off, all timers gated, every LED line Hi-Z.
 */
#ifndef RETHINK_POWER_H
#define RETHINK_POWER_H

#include "board.h"

namespace power {

/* Blank the matrix and power-down until BOTH buttons are pressed.
   Returns with the display scanning again. */
void sleepUntilWake();

}  // namespace power

#endif
