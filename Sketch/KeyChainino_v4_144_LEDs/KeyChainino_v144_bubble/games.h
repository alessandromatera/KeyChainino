/*
 * games.h — entry point of the one installed game module. Bubble
 * implements exactly reset() + frame() (see engine.h for the contract)
 * in game_bubble.cpp and registers itself in registry.cpp.
 */
#ifndef RETHINK_GAMES_H
#define RETHINK_GAMES_H

#include "engine.h"

namespace bubble {
void reset();
uint8_t frame();
}

#endif
