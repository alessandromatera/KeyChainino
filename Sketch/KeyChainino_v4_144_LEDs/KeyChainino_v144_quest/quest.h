/*
 * quest.h — the adventure's top loop. boot() restores the quest from
 * EEPROM (or starts a fresh one, title and all); tick() is called forever
 * from loop() and owns the fixed 50Hz timestep, the save-and-sleep exit
 * chord, the idle sleep and the death/victory ceremonies.
 */
#ifndef QUEST_QUEST_H
#define QUEST_QUEST_H

#include "board.h"

namespace quest {

void boot();
void tick();

}  // namespace quest

#endif
