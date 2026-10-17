/*
 * text.h — the scroller, rethought: zero heap, zero copies.
 *
 * The classic library malloc()ed a rendered buffer of the whole string
 * (freed from inside the Timer2 ISR!). This one keeps three pointers and
 * streams glyph columns straight out of PROGMEM as it scrolls, so any
 * length works, PROGMEM strings scroll without a RAM copy, and there is
 * no allocation anywhere in the firmware.
 *
 * A message is up to two segments so "SCORE " (flash) + "123" (RAM digit
 * buffer) needs no string assembly. Text is drawn 7 rows tall, vertically
 * centered (rows 2..8), at full brightness.
 *
 * Non-blocking: start*() then call step() every TEXT_STEP_MS (the caller
 * paces it — menu and engine both do). step() shifts the framebuffer one
 * column left and feeds the next column in; it returns false when the
 * message has fully left the screen.
 */
#ifndef RETHINK_TEXT_H
#define RETHINK_TEXT_H

#include "board.h"

#define TEXT_STEP_MS 50

namespace text {

void startP(const char* msgP);                     // one PROGMEM string
void startP2(const char* msgP, const char* ram);   // PROGMEM + RAM suffix
void stop();                                       // abort (leaves fb as-is)
bool active();
bool step();  // advance one column; false = finished (and auto-stopped)

}  // namespace text

#endif
