/*
 * registry.cpp — the installed games. Row order = menu order = EEPROM
 * high-score slot, so append new games at the END (or bump the EEPROM
 * magic in save.cpp when reordering).
 *
 * Unlike the classic firmware the whole table lives in PROGMEM: adding a
 * game costs zero RAM.
 */
#include "engine.h"
#include "games.h"
#include "icons.h"

/* Display names (menu scroll + game intro). */
static const char NAME_BRICKS[] PROGMEM = "BRICKS";
static const char NAME_SNAKE[] PROGMEM = "SNAKE";
static const char NAME_BLOCKS[] PROGMEM = "BLOCKS";
static const char NAME_INVADERS[] PROGMEM = "INVADERS";
static const char NAME_TENNIS[] PROGMEM = "TENNIS";
static const char NAME_TOWER[] PROGMEM = "TOWER";
static const char NAME_FROG[] PROGMEM = "FROG";
static const char NAME_FLAP[] PROGMEM = "FLAP";
static const char NAME_RACER[] PROGMEM = "RACER";
static const char NAME_ECHO[] PROGMEM = "ECHO";
static const char NAME_JUMP[] PROGMEM = "JUMP";
static const char NAME_CATCH[] PROGMEM = "CATCH";
static const char NAME_WHACK[] PROGMEM = "WHACK";
static const char NAME_DICE[] PROGMEM = "DICE";
static const char NAME_LIFE[] PROGMEM = "LIFE";
static const char NAME_MUNCHER[] PROGMEM = "MUNCHER";
static const char NAME_STEROIDS[] PROGMEM = "STEROIDS";

static const char LBL_SCORE[] PROGMEM = "SCORE ";
static const char LBL_LINES[] PROGMEM = "LINES ";
static const char LBL_ROWS[] PROGMEM = "ROWS ";

const GameDef GAME_TABLE[] PROGMEM = {
  { bricks::reset,   bricks::frame,   NAME_BRICKS,   LBL_SCORE, ICON_BRICKS,   0 },
  { snake::reset,    snake::frame,    NAME_SNAKE,    LBL_SCORE, ICON_SNAKE,    0 },
  { blocks::reset,   blocks::frame,   NAME_BLOCKS,   LBL_LINES, ICON_BLOCKS,   0 },
  { invaders::reset, invaders::frame, NAME_INVADERS, LBL_SCORE, ICON_INVADERS, 0 },
  { tennis::reset,   tennis::frame,   NAME_TENNIS,   LBL_SCORE, ICON_TENNIS,   0 },
  { tower::reset,    tower::frame,    NAME_TOWER,    LBL_ROWS,  ICON_TOWER,    0 },
  { frog::reset,     frog::frame,     NAME_FROG,     LBL_SCORE, ICON_FROG,     0 },
  { flap::reset,     flap::frame,     NAME_FLAP,     LBL_SCORE, ICON_FLAP,     0 },
  { racer::reset,    racer::frame,    NAME_RACER,    LBL_SCORE, ICON_RACER,    0 },
  { echo::reset,     echo::frame,     NAME_ECHO,     LBL_SCORE, ICON_ECHO,     0 },
  { jump::reset,     jump::frame,     NAME_JUMP,     LBL_SCORE, ICON_JUMP,     0 },
  { catcher::reset,  catcher::frame,  NAME_CATCH,    LBL_SCORE, ICON_CATCH,    0 },
  { whack::reset,    whack::frame,    NAME_WHACK,    LBL_SCORE, ICON_WHACK,    0 },
  { dice::reset,     dice::frame,     NAME_DICE,     LBL_SCORE, ICON_DICE,     GF_SCORELESS },
  { life::reset,     life::frame,     NAME_LIFE,     LBL_SCORE, ICON_LIFE,     GF_SCORELESS },
  /* appended at the end on purpose: existing EEPROM/menu indices 0..14
     must not shift (see the file header and save.cpp's EE_MAGIC bump) */
  { muncher::reset,  muncher::frame,  NAME_MUNCHER,  LBL_SCORE, ICON_MUNCHER,  0 },
  { steroids::reset, steroids::frame, NAME_STEROIDS, LBL_SCORE, ICON_STEROIDS, 0 },
};

namespace engine {

uint8_t numGames() {
  return sizeof(GAME_TABLE) / sizeof(GAME_TABLE[0]);
}

}  // namespace engine
