/*
 * board.h — KeyChainino 144 hardware facts, shared by every module.
 *
 * The board is an ATmega328 with a 12x12 charlieplexed LED matrix on 13
 * GPIOs and two buttons on the external interrupt pins (used both for play
 * and to wake the MCU from power-down).
 */
#ifndef RETHINK_BOARD_H
#define RETHINK_BOARD_H

#include <Arduino.h>

/* The display scan is cycle-hardcoded (see display.cpp) so it stays right
   at ~100 Hz no matter what clock the build claims. Everything else that
   paces itself off millis()/delay() (menu name-scroll settle, chord/tap
   timing, idle sleep, game frame pacing...) is NOT clock-independent: the
   Arduino core derives its millis() tick length from the compiled F_CPU,
   so building for the wrong clock doesn't just skew the wall-clock speed
   of a game, it silently changes *when* the menu's input::flush() (called
   from enter(), to eat buttons still held from the wake gesture) resolves
   relative to a real button press, which can eat a legitimate tap. Building
   with the bare "MiniCore:avr:328" FQBN (no explicit clock=) picks
   MiniCore's default of external 16 MHz; the board's actual clock is the
   internal 8 MHz RC oscillator this firmware assumes everywhere. Select
   Tools > Clock > Internal 8 MHz in the IDE, or pass
   MiniCore:avr:328:clock=8MHz_internal to arduino-cli. */
#if F_CPU != 8000000UL
#error "Wrong clock: select Internal 8 MHz (MiniCore:avr:328:clock=8MHz_internal). KeyChainino 144 runs its internal RC oscillator at 8 MHz; building for any other clock silently halves millis()/delay() versus real elapsed time."
#endif

#define MATRIX_W 12
#define MATRIX_H 12

#define BUTTON_A 3  // INT1
#define BUTTON_B 2  // INT0

/* PORTD bits that must always stay high: the button pull-ups (PD2/PD3).
   The display ISR rewrites PORTD wholesale, so it ORs this in every slot. */
#define PORTD_BUTTON_PULLUPS (_BV(PD2) | _BV(PD3))

/* Brightness levels understood by the display driver. The two BCM planes
   are weighted 1:7 (see display.cpp for why charlieplexing forces such a
   wide split), so in practice there are three honest levels:
     BR_OFF  —  off
     BR_DIM  —  ~1/8 duty: scenery, backgrounds, guides
     BR_MAX  —  full: the player and the action
   BR_MID (~7/8) is barely below full; use it only where "bright" is all
   that matters. */
#define BR_OFF 0
#define BR_DIM 1
#define BR_MID 2
#define BR_MAX 3

#endif
