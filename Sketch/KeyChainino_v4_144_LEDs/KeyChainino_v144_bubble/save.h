/*
 * save.h — the few bytes we keep in EEPROM: last selected game and one
 * 16-bit high score per game. A magic byte re-initializes the area the
 * first time this firmware runs (bump it if the layout ever changes).
 */
#ifndef RETHINK_SAVE_H
#define RETHINK_SAVE_H

#include "board.h"

namespace save {

void begin(uint8_t numGames);

uint8_t lastGame();
void setLastGame(uint8_t idx);

uint16_t hiScore(uint8_t gameIdx);
bool submitScore(uint8_t gameIdx, uint16_t score);  // true = new record

}  // namespace save

#endif
