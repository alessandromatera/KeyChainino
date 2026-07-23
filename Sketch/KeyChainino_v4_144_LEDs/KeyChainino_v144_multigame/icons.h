/*
 * icons.h — menu icons and the KeyChainino faces, rethought as packed
 * bitmaps: 12 words per image (bit x of word y = pixel x,y) instead of
 * 144 bytes — same pictures, 1/6th of the flash.
 *
 * Menu icons keep row 11 empty: the menu draws its position bar there.
 * Draw with gfx::drawIconP().
 */
#ifndef RETHINK_ICONS_H
#define RETHINK_ICONS_H

#include "board.h"

extern const uint16_t ICON_BRICKS[12] PROGMEM;
extern const uint16_t ICON_SNAKE[12] PROGMEM;
extern const uint16_t ICON_BLOCKS[12] PROGMEM;
extern const uint16_t ICON_INVADERS[12] PROGMEM;
extern const uint16_t ICON_TENNIS[12] PROGMEM;
extern const uint16_t ICON_TOWER[12] PROGMEM;
extern const uint16_t ICON_FROG[12] PROGMEM;
extern const uint16_t ICON_FLAP[12] PROGMEM;
extern const uint16_t ICON_RACER[12] PROGMEM;
extern const uint16_t ICON_ECHO[12] PROGMEM;
extern const uint16_t ICON_JUMP[12] PROGMEM;
extern const uint16_t ICON_CATCH[12] PROGMEM;
extern const uint16_t ICON_WHACK[12] PROGMEM;
extern const uint16_t ICON_DICE[12] PROGMEM;
extern const uint16_t ICON_LIFE[12] PROGMEM;
extern const uint16_t ICON_MUNCHER[12] PROGMEM;
extern const uint16_t ICON_STEROIDS[12] PROGMEM;

extern const uint16_t KeyChaininoFace[12] PROGMEM;
extern const uint16_t KeyChaininoSadFace[12] PROGMEM;

#endif
