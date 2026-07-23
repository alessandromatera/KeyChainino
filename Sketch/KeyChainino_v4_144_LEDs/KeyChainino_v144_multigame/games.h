/*
 * games.h — entry points of the 17 game modules. Each module implements
 * exactly reset() + frame() (see engine.h for the contract) in its own
 * game_<name>.cpp file, keeps all state static, and registers itself in
 * registry.cpp.
 */
#ifndef RETHINK_GAMES_H
#define RETHINK_GAMES_H

#include "engine.h"

#define GAME_MODULE(ns) \
  namespace ns {        \
  void reset();         \
  uint8_t frame();      \
  }

GAME_MODULE(bricks)
GAME_MODULE(snake)
GAME_MODULE(blocks)
GAME_MODULE(invaders)
GAME_MODULE(tennis)
GAME_MODULE(tower)
GAME_MODULE(frog)
GAME_MODULE(flap)
GAME_MODULE(racer)
GAME_MODULE(echo)
GAME_MODULE(jump)
GAME_MODULE(catcher)
GAME_MODULE(whack)
GAME_MODULE(dice)
GAME_MODULE(life)
GAME_MODULE(muncher)
GAME_MODULE(steroids)

#undef GAME_MODULE

#endif
