/*
 * display.h — charlieplex driver for the 12x12 matrix, rethought.
 *
 * What changed versus the classic KeyChainino library:
 *
 *  - 4-level greyscale. Every pixel is 0..3 (BR_OFF..BR_MAX), rendered by
 *    binary code modulation: each scanline is shown twice per frame, once
 *    for a 1-weight slot (bit 0 of the pixel) and once for a 2-weight slot
 *    (bit 1). Total on-time is proportional to the pixel value.
 *
 *  - The ISR does no work. display::flush() "compiles" the framebuffer
 *    into ready-made DDR/PORT register images (13 scanlines x 2 planes x
 *    3 ports); the Timer2 ISR just blanks the matrix and writes 6 bytes to
 *    the port registers. No pinMode/digitalWrite, no per-LED loop.
 *
 *  - No heap and no 169-byte Chaplex matrix. RAM: 144-byte framebuffer +
 *    156-byte compiled scan table.
 *
 * Timer2 runs in CTC mode and is owned by this driver. All timing derives
 * from F_CPU, so the refresh is ~100 Hz at both 8 and 16 MHz.
 *
 * The framebuffer is column-major like the classic library: fb[x*12 + y].
 * Use gfx.h for drawing; games never touch fb directly.
 */
#ifndef RETHINK_DISPLAY_H
#define RETHINK_DISPLAY_H

#include "board.h"

namespace display {

extern uint8_t fb[MATRIX_W * MATRIX_H];  // pixel values 0..3

void begin();   // configure Timer2 + power off unused peripherals
void flush();   // compile fb into the scan table (makes drawing visible)

/* For sleep: park the matrix (scan off, all LED pins Hi-Z) and resume. */
void park();
void resume();

}  // namespace display

#endif
