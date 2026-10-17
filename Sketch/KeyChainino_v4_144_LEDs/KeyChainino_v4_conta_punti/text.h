/*
 * text.h — digits on the matrix, rethought: zero heap, zero copies.
 *
 * The classic library malloc()ed a rendered buffer of the whole string
 * (freed from inside the Timer2 ISR!). This one keeps a pointer into the
 * caller's digit string and streams glyph columns straight out of PROGMEM,
 * so there is no allocation anywhere in the firmware. Digits are drawn
 * 7 rows tall, vertically centered (rows 2..8), at full brightness.
 *
 * Three ways to show a number (a RAM string of digits, e.g. from utoa):
 *
 *   draw(s)   — static, horizontally centered, shown at once. For what
 *               fits the 12 columns: one or two digits.
 *
 *   start(s)  — scroll it through, for the numbers that don't fit. Each
 *               step() shifts the framebuffer one column left and feeds
 *               the next column in, until the number has fully left the
 *               screen.
 *
 *   roll(from, to) — the odometer: `from` slides up and out while `to`
 *               rolls in from below, one row per step(); with down, the
 *               other way round (counting back). Both are drawn like
 *               draw(), so one or two digits each; the last step leaves
 *               `to` exactly as draw(to) would.
 *
 * paint(s, level) is draw() for composing: it only adds the centered
 * digits to the framebuffer, at any brightness, and leaves clearing and
 * flushing to the caller.
 *
 * The animated ones need step() every stepMs() (the caller paces it);
 * step() returns false once the animation is over. Their strings must
 * stay valid meanwhile.
 *
 * draw(), start() and roll() flush the display themselves. draw()
 * cancels a running animation; stop() cancels it too, leaving the screen
 * to the caller.
 */
#ifndef RETHINK_TEXT_H
#define RETHINK_TEXT_H

#include "board.h"

#define TEXT_STEP_MS 50  // scroll: one column per step
#define TEXT_ROLL_MS 30  // roll: one row per step

namespace text {

void draw(const char* s);                  // static, centered
void start(const char* s);                 // begin scrolling
void roll(const char* from, const char* to,  // begin rolling from -> to;
          bool down = false,                 // `from` drawn at fromLevel
          uint8_t fromLevel = BR_MAX);
void paint(const char* s, uint8_t level);    // add centered, no flush
void stop();       // cancel an animation (the screen is left as is)
bool active();
uint8_t stepMs();  // how often the running animation wants step()
bool step();       // advance it; false = finished (and auto-stopped)

}  // namespace text

#endif
