/*
 * menu.h — game picker. A/B browse, chord-tap launches. The icon is
 * bright; the bottom row is a full-brightness fill bar with one LED per
 * game (wrapping past the 12th), so it advances exactly one step per
 * press. No dim pixels here: an isolated dim dot competes with
 * charlieplex ghost light and reads as a phantom LED. Settle on a game
 * for half a second and its name scrolls once. 25s of no input powers
 * the board down; A+B wakes it.
 */
#ifndef RETHINK_MENU_H
#define RETHINK_MENU_H

#include "board.h"

namespace menu {

void begin();   // load the remembered selection (call once at boot)
void enter();   // (re)take the screen: redraw, reset timers
int8_t tick();  // call per loop; returns a game index to launch, or -1

}  // namespace menu

#endif
