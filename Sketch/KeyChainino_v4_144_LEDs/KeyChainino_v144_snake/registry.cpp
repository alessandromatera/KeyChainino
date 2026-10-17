/*
 * registry.cpp — the installed game: just Snake. One row, one EEPROM
 * high-score slot. The table lives in PROGMEM like everything constant.
 */
#include "engine.h"
#include "games.h"

static const char NAME_SNAKE[] PROGMEM = "SNAKE";
static const char LBL_SCORE[] PROGMEM = "SCORE ";

const GameDef GAME_TABLE[] PROGMEM = {
  { snake::reset, snake::frame, NAME_SNAKE, LBL_SCORE, nullptr, 0 },
};

namespace engine {

uint8_t numGames() {
  return sizeof(GAME_TABLE) / sizeof(GAME_TABLE[0]);
}

}  // namespace engine
