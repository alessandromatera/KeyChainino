/*
 * input.h — the two buttons, debounced, reduced to the gestures the
 * counter understands:
 *
 *   press A (left) alone        -> "left":  show the count
 *   press B (right) alone       -> "right": count one
 *   hold A and B for LONG_CHORD_MS -> "long chord": reset the count
 *
 * A short grace window after the first button goes down waits for the
 * second, so a slightly staggered two-thumb press reads as a chord and
 * not as a single press. Holding one button does nothing more: one
 * physical press is exactly one event, never an auto-repeat. Letting go
 * of A+B before LONG_CHORD_MS does nothing at all.
 *
 * Levels are debounced before the gesture logic sees them: a contact must
 * hold steady for a few ms to be believed, so switch chatter can neither
 * double-count a press nor fake one on release.
 *
 * poll() as often as possible; the event getters consume their event.
 */
#ifndef RETHINK_INPUT_H
#define RETHINK_INPUT_H

#include "board.h"

#define LONG_CHORD_MS 2000  // hold A+B this long to reset

namespace input {

void reset();  // forget everything (call right after waking up)
void poll();

bool left();       // consume: A was pressed on its own
bool right();      // consume: B was pressed on its own
bool longChord();  // consume: A+B were held for LONG_CHORD_MS

bool chording();     // A+B are down and the hold is still running
uint16_t chordMs();  // how long they have been down (while chording())

}  // namespace input

#endif
