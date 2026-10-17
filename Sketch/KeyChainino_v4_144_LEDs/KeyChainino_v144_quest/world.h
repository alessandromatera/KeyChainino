/*
 * world.h — the overworld: a 4x4 grid of hand-authored 12x12 rooms,
 * stored as readable ASCII maps in PROGMEM (144 bytes per room; the
 * whole world costs ~2.3KB of flash and zero RAM).
 *
 * Map legend:
 *   '#' wall            '.' floor
 *   'D' locked door     (only ever on a border, at position 5)
 *   'K' key   'H' heart 'S' sword   'T' treasure (the goal)
 *   'E' enemy spawn     'B' boss (top-left of its 2x2 body)
 *   '@' the player's home cell (exactly one in the world)
 *
 * Rooms connect through 1-cell gaps in their border walls, always at
 * row/column 5, so neighbouring edges line up by construction. Walking
 * off an edge flips to the next room, classic flip-screen style.
 *
 * Items get a stable id from their scan order (room-major, then
 * row-major) — that id indexes the save file's "taken" bitmask, so a
 * picked-up key stays picked up across power cycles. Doors are
 * identified by the edge they sit on (see DOOR_EDGES in world.cpp).
 */
#ifndef QUEST_WORLD_H
#define QUEST_WORLD_H

#include "board.h"

#define WORLD_W 4
#define WORLD_H 4
#define NUM_ROOMS (WORLD_W * WORLD_H)

namespace world {

char cell(uint8_t room, uint8_t x, uint8_t y);  // raw map char

/* Scan-order id of the item ('K','H','S','T') at this cell. */
uint8_t itemIdAt(uint8_t room, uint8_t x, uint8_t y);

/* Id of the first item of this type in the world (the sword has one). */
uint8_t itemIdOfType(char type);

/* Which door bit the 'D' at this border cell belongs to; -1 if none. */
int8_t doorIndexAt(uint8_t room, uint8_t x, uint8_t y);

/* The '@' cell. */
void findStart(uint8_t* room, int8_t* x, int8_t* y);

}  // namespace world

#endif
