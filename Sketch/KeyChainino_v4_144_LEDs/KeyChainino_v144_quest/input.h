/*
 * input.h — the 2-button grammar, same language the multigame firmware
 * taught its players, reimplemented for the rethought loop:
 *
 *   tap A / tap B      -> single-step events (auto-repeat while held)
 *   tap A+B together   -> "chord tap" (confirm / rotate / fire)
 *   hold A+B >= 1.2s   -> "chord hold" (universal back-to-menu)
 *
 * A short grace window turns near-simultaneous presses into a chord
 * instead of a stray single press. Games needing continuous movement
 * read the levels (downA/downB) and pace themselves.
 *
 * poll() once per loop; the event getters consume their event.
 */
#ifndef RETHINK_INPUT_H
#define RETHINK_INPUT_H

#include "board.h"

namespace input {

void poll();
void flush();  // swallow whatever is pressed right now (context switches)

bool tapA();      // consume: A tapped (fires again on auto-repeat)
bool tapB();      // consume: B tapped
bool chordTap();  // consume: A+B tapped together
bool exitHold();  // consume: A+B held long enough to ask for the menu

bool downA();  // level: A currently pressed
bool downB();  // level: B currently pressed
bool idle();   // nothing pressed, nothing pending

/* level: both buttons down together (the exit-gesture window). Games with a
   free-running world freeze it while this is true, so the ~1.2s A+B hold
   that ends the round can't kill the player mid-gesture. */
bool chordHeld();

}  // namespace input

#endif
