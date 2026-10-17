#include "save.h"

#include <EEPROM.h>

namespace save {

/* 0x5A ('Z') also acts as the layout version: bump it if State ever
   changes shape, so stale bytes from another firmware (the multigame
   sketch keeps its high scores in the same low addresses) can never be
   misread as a quest in progress. */
#define EE_MAGIC 0x5A
#define EE_BASE 0  // magic, then the fields in struct order

bool load(State* out) {
  if (EEPROM.read(EE_BASE) != EE_MAGIC) return false;
  out->room = EEPROM.read(EE_BASE + 1);
  out->x = (int8_t)EEPROM.read(EE_BASE + 2);
  out->y = (int8_t)EEPROM.read(EE_BASE + 3);
  out->facing = EEPROM.read(EE_BASE + 4);
  out->hearts = EEPROM.read(EE_BASE + 5);
  out->keys = EEPROM.read(EE_BASE + 6);
  out->taken = (uint16_t)EEPROM.read(EE_BASE + 7) |
               ((uint16_t)EEPROM.read(EE_BASE + 8) << 8);
  out->doors = EEPROM.read(EE_BASE + 9);
  /* Sanity: a corrupt save must fall back to a fresh quest, never spawn
     the player inside a wall or with zero hearts. */
  if (out->room >= 16 || (uint8_t)out->x >= MATRIX_W ||
      (uint8_t)out->y >= MATRIX_H || out->facing > 3 ||
      out->hearts == 0 || out->hearts > 3 || out->keys > 4) {
    return false;
  }
  return true;
}

void store(const State* s) {
  EEPROM.update(EE_BASE + 1, s->room);
  EEPROM.update(EE_BASE + 2, (uint8_t)s->x);
  EEPROM.update(EE_BASE + 3, (uint8_t)s->y);
  EEPROM.update(EE_BASE + 4, s->facing);
  EEPROM.update(EE_BASE + 5, s->hearts);
  EEPROM.update(EE_BASE + 6, s->keys);
  EEPROM.update(EE_BASE + 7, s->taken & 0xFF);
  EEPROM.update(EE_BASE + 8, s->taken >> 8);
  EEPROM.update(EE_BASE + 9, s->doors);
  EEPROM.update(EE_BASE, EE_MAGIC);  // magic last: a torn write stays invalid
}

void wipe() { EEPROM.update(EE_BASE, 0xFF); }

}  // namespace save
