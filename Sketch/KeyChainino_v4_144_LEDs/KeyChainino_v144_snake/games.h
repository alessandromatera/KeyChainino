/*
 * games.h — entry point of the one installed game module. Snake
 * implements exactly reset() + frame() (see engine.h for the contract)
 * in game_snake.cpp and registers itself in registry.cpp.
 */
#ifndef RETHINK_GAMES_H
#define RETHINK_GAMES_H

#include "engine.h"

namespace snake {
void reset();
uint8_t frame();
}

#endif
