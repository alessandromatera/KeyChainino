#include "save.h"

#include <EEPROM.h>

namespace save {

#define EE_MAGIC_ADDR 0
#define EE_MAGIC 0x5A  // standalone-snake layout (1 game). Distinct from
                       // the multigame firmware's magic (0x53) so that
                       // flashing one firmware over the other re-inits
                       // the EEPROM area instead of reading the other
                       // layout's slot 0 as Snake's high score.
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
