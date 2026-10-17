/*
 * icons.h — the KeyChainino faces, as packed bitmaps: 12 words per image
 * (bit x of word y = pixel x,y). Draw with gfx::drawIconP().
 */
#ifndef RETHINK_ICONS_H
#define RETHINK_ICONS_H

#include "board.h"

extern const uint16_t KeyChaininoFace[12] PROGMEM;
extern const uint16_t KeyChaininoSadFace[12] PROGMEM;

#endif
