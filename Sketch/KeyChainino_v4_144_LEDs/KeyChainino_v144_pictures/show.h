/*
 * show.h — everything that moves on the matrix:
 *
 *   slide()    the swift push from one picture to the next (ease-out,
 *              a one-pixel overshoot, a dim motion smear), then the new
 *              picture plays its little show
 *   wake()     the picture rises into view, then plays its little show
 *   bedtime()  the picture sinks away before the KeyChainino sleeps
 *   animate()  A+B: a built-in plays its own hand-made animation; a
 *              child's drawing gets the next generic effect (hop, jelly,
 *              sparkle, wave, flip, beat, burst, spin)
 *
 * Slides and wake/bedtime block for a fraction of a second but keep
 * polling the buttons; animations run from tick(), so any press cuts
 * them short straight away.
 */
#ifndef PICTURES_SHOW_H
#define PICTURES_SHOW_H

#include "board.h"

namespace show {

void wake();
void bedtime();
void slide(int8_t dir);  // to the album's current entry; +1 = comes from the right
void animate();
void tick();             // advance a running animation; call every loop

}  // namespace show

#endif
