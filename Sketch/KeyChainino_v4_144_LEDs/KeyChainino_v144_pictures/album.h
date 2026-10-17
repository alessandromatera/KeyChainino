/*
 * album.h — the list of pictures the buttons scroll through:
 *
 *   [ child's drawings, newest first ] [ chosen built-ins, library order ]
 *
 * Every built-in lives in flash, but the KeyChainino only shows the ones
 * chosen on the web page (upload/keychainino-pictures.html), which writes
 * the drawings and the choice into the EEPROM over ISP. Layout, shared
 * with the page:
 *
 *   0x000  'K' 'C' 'P'   magic
 *   0x003  2             layout version
 *   0x004  count         number of drawings, 0..ALBUM_SLOTS
 *   0x005  selected      list index shown on wake (the page sets it to the
 *                        picture it just added, so that one shows first)
 *   0x006  reserved (0xFF) up to 0x00F
 *   0x010  slot 0 ...    24 bytes each: 12 rows, uint16 little endian,
 *                        bit x of row y = pixel (x, y). Oldest first.
 *   0x3D0  reserved up to 0x3DF
 *   0x3E0  choice        32 bytes: built-in with uid u is shown when bit
 *                        (u & 7) of byte u/8 is set (uids: tools/uids.json)
 *
 * Version 1 (no choice: every built-in was shown) is upgraded in place on
 * boot: drawings kept, choice = DEFAULT_CHOICE. Bump ALBUM_VERSION here and
 * in the page if the layout ever changes again.
 */
#ifndef PICTURES_ALBUM_H
#define PICTURES_ALBUM_H

#include "board.h"

#define ALBUM_MAGIC0 'K'
#define ALBUM_MAGIC1 'C'
#define ALBUM_MAGIC2 'P'
#define ALBUM_VERSION 2
#define ALBUM_SLOTS 40
#define ALBUM_SLOT_BASE 0x10
#define ALBUM_SLOT_SIZE 24
#define ALBUM_CHOICE_BASE 0x3E0
#define ALBUM_CHOICE_SIZE 32

namespace album {

void begin();  // validate, upgrade or format the EEPROM album; restore selection

uint8_t size();      // drawings + chosen built-ins (never 0)
uint8_t index();     // current list position
void step(int8_t d); // move by d, wrapping around

bool isDrawing();    // current entry is one of the child's drawings
uint8_t builtin();   // current built-in, as an index into PICS (valid when !isDrawing())

void load(uint16_t rows[12]);  // the current entry's still picture

void saveSelection();  // remember the current entry (EEPROM, on sleep)

}  // namespace album

#endif
