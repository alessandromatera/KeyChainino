#include "save.h"

#include <EEPROM.h>

namespace save {

#define EE_MAGIC_ADDR 0
#define EE_MAGIC 0x53  // bumped from 0x52: 17 games (was 15) need 4 more
                       // hiscore bytes that would otherwise hold stale
                       // data from the old layout -- forces a one-time
                       // re-init of the whole EEPROM area on first boot
                       // of this firmware (wipes old high scores).
#define EE_LAST_GAME 1
#define EE_HISCORE_BASE 2  // uint16 per game, little endian

void begin(uint8_t numGames) {
  if (EEPROM.read(EE_MAGIC_ADDR) != EE_MAGIC) {
    EEPROM.update(EE_MAGIC_ADDR, EE_MAGIC);
    EEPROM.update(EE_LAST_GAME, 0);
    for (uint8_t i = 0; i < 2 * numGames; i++) {
      EEPROM.update(EE_HISCORE_BASE + i, 0);
    }
  }
}

uint8_t lastGame() { return EEPROM.read(EE_LAST_GAME); }

void setLastGame(uint8_t idx) { EEPROM.update(EE_LAST_GAME, idx); }

uint16_t hiScore(uint8_t gameIdx) {
  int addr = EE_HISCORE_BASE + 2 * gameIdx;
  return (uint16_t)EEPROM.read(addr) | ((uint16_t)EEPROM.read(addr + 1) << 8);
}

bool submitScore(uint8_t gameIdx, uint16_t score) {
  if (score <= hiScore(gameIdx)) return false;
  int addr = EE_HISCORE_BASE + 2 * gameIdx;
  EEPROM.update(addr, score & 0xFF);
  EEPROM.update(addr + 1, score >> 8);
  return true;
}

}  // namespace save
