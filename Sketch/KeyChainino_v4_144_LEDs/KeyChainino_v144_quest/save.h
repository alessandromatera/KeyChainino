/*
 * save.h — the quest lives in EEPROM, so the keychain form factor works
 * FOR the game: power off (or the 30s idle sleep) mid-adventure and the
 * world is exactly where you left it at the next wake. Ten bytes: where
 * you are, what you carry, which doors you already opened and which items
 * the world no longer holds. A magic byte validates the save; wipe() is
 * only called on victory and on the deliberate long-hold quest reset.
 */
#ifndef QUEST_SAVE_H
#define QUEST_SAVE_H

#include "board.h"

namespace save {

struct State {
  uint8_t room;     // 0..15
  int8_t x, y;      // player cell
  uint8_t facing;   // 0 N, 1 E, 2 S, 3 W
  uint8_t hearts;   // 1..3
  uint8_t keys;
  uint16_t taken;   // bit per world item (scan-order id, see world.h)
  uint8_t doors;    // bit per door (world.cpp's DOOR_EDGES order)
};

bool load(State* out);          // false = no (or corrupt) save
void store(const State* s);     // EEPROM.update: cheap, wear-friendly
void wipe();                    // invalidate: next boot starts fresh

}  // namespace save

#endif
