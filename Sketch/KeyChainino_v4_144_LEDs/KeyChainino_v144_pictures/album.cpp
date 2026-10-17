#include "album.h"

#include <EEPROM.h>

#include "pictures_data.h"

namespace album {

#define EE_VERSION 3
#define EE_COUNT 4
#define EE_SELECTED 5

static uint8_t drawings;              // how many child drawings are in EEPROM
static uint8_t chosen[NUM_BUILTIN];   // PICS indices shown, library order
static uint8_t nChosen;
static uint8_t cur;                   // list index

static void writeDefaultChoice() {
  for (uint8_t i = 0; i < ALBUM_CHOICE_SIZE; i++) {
    EEPROM.update(ALBUM_CHOICE_BASE + i, pgm_read_byte(&DEFAULT_CHOICE[i]));
  }
}

void begin() {
  uint8_t version = EEPROM.read(EE_VERSION);
  bool ok = EEPROM.read(0) == ALBUM_MAGIC0 && EEPROM.read(1) == ALBUM_MAGIC1 &&
            EEPROM.read(2) == ALBUM_MAGIC2 &&
            (version == 1 || version == ALBUM_VERSION) &&
            EEPROM.read(EE_COUNT) <= ALBUM_SLOTS;
  if (!ok) {
    /* First boot of this firmware (or leftovers from another one, e.g. the
       multigame's high scores): no drawings, the default built-ins. */
    EEPROM.update(0, ALBUM_MAGIC0);
    EEPROM.update(1, ALBUM_MAGIC1);
    EEPROM.update(2, ALBUM_MAGIC2);
    EEPROM.update(EE_VERSION, ALBUM_VERSION);
    EEPROM.update(EE_COUNT, 0);
    EEPROM.update(EE_SELECTED, 0);
    writeDefaultChoice();
  } else if (version == 1) {
    /* An album from before built-ins could be chosen: keep the drawings,
       start with the default choice. */
    writeDefaultChoice();
    EEPROM.update(EE_SELECTED, 0);
    EEPROM.update(EE_VERSION, ALBUM_VERSION);
  }

  drawings = EEPROM.read(EE_COUNT);
  nChosen = 0;
  for (uint8_t i = 0; i < NUM_BUILTIN; i++) {
    uint8_t uid = pgm_read_byte(&PICS[i].uid);
    if (EEPROM.read(ALBUM_CHOICE_BASE + (uid >> 3)) & (1 << (uid & 7))) chosen[nChosen++] = i;
  }
  if (!drawings && !nChosen) chosen[nChosen++] = 0;  // never an empty book: the first picture

  cur = EEPROM.read(EE_SELECTED);
  if (cur >= size()) cur = 0;
}

uint8_t size() { return drawings + nChosen; }
uint8_t index() { return cur; }

void step(int8_t d) {
  int16_t n = size();
  cur = (uint8_t)(((int16_t)cur + d % n + n) % n);
}

bool isDrawing() { return cur < drawings; }
uint8_t builtin() { return chosen[cur - drawings]; }

void load(uint16_t rows[12]) {
  if (isDrawing()) {
    uint8_t slot = drawings - 1 - cur;  // newest first
    int addr = ALBUM_SLOT_BASE + slot * ALBUM_SLOT_SIZE;
    for (uint8_t y = 0; y < 12; y++, addr += 2) {
      rows[y] = (EEPROM.read(addr) | ((uint16_t)EEPROM.read(addr + 1) << 8)) & 0x0FFF;
    }
  } else {
    uint8_t f = pgm_read_byte(&PICS[builtin()].frame);
    for (uint8_t y = 0; y < 12; y++) rows[y] = pgm_read_word(&FRAMES[f][y]);
  }
}

void saveSelection() { EEPROM.update(EE_SELECTED, cur); }

}  // namespace album
