/*
 * engine.h — the game framework, rethought.
 *
 * A game is now just two functions:
 *
 *   void reset()      — set up a fresh round (screen already cleared)
 *   uint8_t frame()   — advance ONE fixed 20ms step and draw; return
 *                       GAME_ON, GAME_LOSE or GAME_WIN
 *
 * The engine owns everything around the round: the intro face, the fixed
 * 50Hz timestep, flushing the display after every frame, the universal
 * A+B-hold exit, the win/lose faces, the score scroll (skippable with any
 * button) and the EEPROM high-score handling. Games keep their score in
 * engine::score and never touch any of that machinery — the old
 * init/tick/isDone/stop contract, the per-game `done`/`over` flags and
 * all the millis() bookkeeping are gone.
 *
 * Pacing idiom for games (all speeds stay in milliseconds, like the
 * originals): keep a ms accumulator, add FRAME_MS each frame, act when it
 * passes the current period. Short blocking effect animations (flashes)
 * may still use delay(); the engine resynchronizes afterwards.
 */
#ifndef RETHINK_ENGINE_H
#define RETHINK_ENGINE_H

#include "board.h"

/* frame() results */
#define GAME_ON 0
#define GAME_LOSE 1
#define GAME_WIN 2

/* GameDef flags */
#define GF_SCORELESS 0x01  // toys (dice, life): skip the score/record flow

#define FRAME_MS 20  // fixed timestep: 50 frames per second

struct GameDef {
  void (*reset)();
  uint8_t (*frame)();
  const char* name;        // PROGMEM, scrolled in the menu
  const char* scoreLabel;  // PROGMEM, e.g. "SCORE " (ignored if scoreless)
  const uint16_t* icon;    // PROGMEM packed 12-row bitmap
  uint8_t flags;
};

/* The table itself lives fully in PROGMEM (registry.cpp). */
extern const GameDef GAME_TABLE[] PROGMEM;

namespace engine {

extern uint16_t score;  // written by the running game

uint8_t numGames();
void gameDef(uint8_t idx, GameDef* out);  // copy one entry out of PROGMEM

/* Run one round of game idx; returns when it ends or the player exits. */
void runGame(uint8_t idx);

/* Shared helpers (games and menu may use them too) */
void showFaceP(const uint16_t* iconP, uint16_t holdMs);
void scrollP(const char* msgP);                    // blocking, any button skips
void scrollNumP(const char* labelP, uint16_t n);   // "<label><n>", same

}  // namespace engine

#endif
