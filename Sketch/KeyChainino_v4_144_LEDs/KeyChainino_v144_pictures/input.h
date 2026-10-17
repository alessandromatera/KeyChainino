/*
 * input.h — the two buttons, tuned for small hands:
 *
 *   tap A        -> previous picture (auto-repeats, slowly, while held)
 *   tap B        -> next picture
 *   A+B together -> fires the moment both are down, however long they
 *                   are then held (no "tap vs hold" for a 3-year-old)
 *
 * Presses are debounced (two equal 5ms samples). A button held for ages
 * stops auto-repeating after a while, so a keychain squeezed in a pocket
 * does not count as play and the idle timer can still put it to sleep.
 *
 * poll() once per loop; the event getters consume their event.
 */
#ifndef PICTURES_INPUT_H
#define PICTURES_INPUT_H

#include "board.h"

namespace input {

void poll();
void flush();  // swallow whatever is pressed right now (after waking)

bool tapA();   // consume: A pressed (and on each auto-repeat)
bool tapB();   // consume: B pressed
bool chord();  // consume: A+B pressed together

/* true when an event was produced since the last call: the idle timer's
   notion of "the child is playing". Holding a button is not activity. */
bool activity();

}  // namespace input

#endif
