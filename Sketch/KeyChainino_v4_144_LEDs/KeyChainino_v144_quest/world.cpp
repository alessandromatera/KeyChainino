/*
 * world.cpp — the sixteen rooms.
 *
 * Overworld layout (room indexes):
 *
 *      0   1   2   3      row 0 (north)
 *      4   5   6   7
 *      8   9  10  11
 *     12  13  14  15      row 3 (south)
 *
 * You start at home (12, bottom-left). The sword waits in a cave chamber
 * one room north (8). Two keys — one in the walled grove of 9, one in
 * the far grove of 2 — open the two locked doors: 11->7 into the dungeon
 * antechamber, then 7->3 into the boss room, where a 2x2 boss patrols
 * between you and the treasure.
 *
 * Authoring rules (checked by tools/check_world.py):
 *   - every room is 12 rows of 12 chars, corners always '#';
 *   - border openings ('.'/'D') only at row/col 5, and both sides of a
 *     shared edge must agree;
 *   - wall clumps are >=2 px runs (a lone dim pixel is the charlieplex
 *     worst case, see the display README).
 */
#include "world.h"

/* clang-format off */
static const char R00[] PROGMEM =
  "############"
  "#..........#"
  "#..##......#"
  "#..##..E...#"
  "#..........#"
  "#..........."
  "#......##..#"
  "#......##..#"
  "#..........#"
  "#..........#"
  "#..........#"
  "#####.######";

static const char R01[] PROGMEM =
  "############"
  "#..........#"
  "#...####...#"
  "#..........#"
  "#..........#"
  "............"
  "#..E.......#"
  "#.....###..#"
  "#..........#"
  "#..........#"
  "#..........#"
  "#####.######";

static const char R02[] PROGMEM =   /* the north grove: key 2 */
  "############"
  "#..........#"
  "#..######..#"
  "#..#....#..#"
  "#..#.K..#..#"
  "...#....#..#"
  "#..#.E..#..#"
  "#..##.###..#"
  "#..........#"
  "#....E.....#"
  "#..........#"
  "#####.######";

static const char R03[] PROGMEM =   /* boss room: the treasure */
  "############"
  "#..........#"
  "#....T.....#"
  "#..........#"
  "#..........#"
  "#..........#"
  "#....B.....#"
  "#..........#"
  "#..##..##..#"
  "#..........#"
  "#..........#"
  "#####D######";

static const char R04[] PROGMEM =
  "#####.######"
  "#..........#"
  "#.##.......#"
  "#.##.......#"
  "#..........#"
  "#..........."
  "#......E...#"
  "#..........#"
  "#......##..#"
  "#......##..#"
  "#..........#"
  "#####.######";

static const char R05[] PROGMEM =   /* the crossroads shrine: a heart */
  "#####.######"
  "#..........#"
  "#..........#"
  "#...#..#...#"
  "#...#..#...#"
  "......H....."
  "#...#..#...#"
  "#...#..#...#"
  "#..........#"
  "#..........#"
  "#..........#"
  "#####.######";

static const char R06[] PROGMEM =   /* guarded dead end: a heart */
  "#####.######"
  "#..........#"
  "#..####...H#"
  "#..........#"
  "#..E....E..#"
  "...........#"
  "#..........#"
  "#..####....#"
  "#..........#"
  "#..........#"
  "#..........#"
  "############";

static const char R07[] PROGMEM =   /* dungeon antechamber: 3 guards */
  "#####D######"
  "#..........#"
  "#.##....##.#"
  "#..........#"
  "#....E.....#"
  "#..........#"
  "#..E....E..#"
  "#..........#"
  "#.##....##.#"
  "#..........#"
  "#..........#"
  "#####D######";

static const char R08[] PROGMEM =   /* the sword cave */
  "#####.######"
  "#..........#"
  "#..######..#"
  "#..#....#..#"
  "#..#.S..#..#"
  "#..#....#..."
  "#..##.###..#"
  "#..........#"
  "#..........#"
  "#..........#"
  "#..........#"
  "#####.######";

static const char R09[] PROGMEM =   /* the walled grove: key 1 */
  "#####.######"
  "#..........#"
  "#..........#"
  "#...####...#"
  "#...#..#...#"
  "....#K.#...."
  "#...#..#...#"
  "#...##.#...#"
  "#....E.....#"
  "#.......E..#"
  "#..........#"
  "#####.######";

static const char R10[] PROGMEM =
  "############"
  "#..........#"
  "#.##...##..#"
  "#..........#"
  "#..........#"
  "............"
  "#..........#"
  "#....E.....#"
  "#..##...##.#"
  "#..........#"
  "#..........#"
  "#####.######";

static const char R11[] PROGMEM =   /* before the dungeon door */
  "#####D######"
  "#..........#"
  "#..........#"
  "#...####...#"
  "#..........#"
  "...........#"
  "#..E....E..#"
  "#..........#"
  "#...####...#"
  "#..........#"
  "#..........#"
  "#####.######";

static const char R12[] PROGMEM =   /* home: safe */
  "#####.######"
  "#..........#"
  "#..........#"
  "#...##.....#"
  "#..........#"
  "#..........."
  "#.....@....#"
  "#..........#"
  "#.....##...#"
  "#..........#"
  "#..........#"
  "############";

static const char R13[] PROGMEM =
  "#####.######"
  "#..........#"
  "#..##......#"
  "#..........#"
  "#.......E..#"
  "............"
  "#..........#"
  "#......##..#"
  "#..........#"
  "#..........#"
  "#..........#"
  "############";

static const char R14[] PROGMEM =   /* the south shrine: a heart */
  "#####.######"
  "#..........#"
  "#..........#"
  "#..##..##..#"
  "#..#....#..#"
  ".....H......"
  "#..#....#..#"
  "#..##..##..#"
  "#.....E....#"
  "#..........#"
  "#..........#"
  "############";

static const char R15[] PROGMEM =
  "#####.######"
  "#..........#"
  "#.......##.#"
  "#..........#"
  "#..E.......#"
  "...........#"
  "#..........#"
  "#..##......#"
  "#..........#"
  "#......##..#"
  "#..........#"
  "############";
/* clang-format on */

static const char* const ROOMS[NUM_ROOMS] PROGMEM = {
  R00, R01, R02, R03, R04, R05, R06, R07,
  R08, R09, R10, R11, R12, R13, R14, R15,
};

/* Doors, identified by the edge they sit on. Both doors are vertical, so
   an edge is named by the room on its NORTH side: a 'D' in room r's row 0
   sits on edge (r-4), one in row 11 on edge (r). Bit i of the save's door
   mask = DOOR_EDGES[i]. */
static const uint8_t DOOR_EDGES[] PROGMEM = {
  3,  /* boss door: between room 3 (south edge) and room 7  */
  7,  /* dungeon door: between room 7 (south edge) and room 11 */
};
#define NUM_DOORS ((int8_t)sizeof(DOOR_EDGES))

namespace world {

char cell(uint8_t room, uint8_t x, uint8_t y) {
  const char* map = (const char*)pgm_read_ptr(&ROOMS[room]);
  return (char)pgm_read_byte(map + (uint8_t)(y * MATRIX_W) + x);
}

static bool isItem(char c) {
  return c == 'K' || c == 'H' || c == 'S' || c == 'T';
}

uint8_t itemIdAt(uint8_t room, uint8_t x, uint8_t y) {
  uint8_t id = 0;
  for (uint8_t r = 0; r <= room; r++) {
    for (uint8_t yy = 0; yy < MATRIX_H; yy++) {
      for (uint8_t xx = 0; xx < MATRIX_W; xx++) {
        if (r == room && yy == y && xx == x) return id;
        if (isItem(cell(r, xx, yy))) id++;
      }
    }
  }
  return id;  // unreachable for a valid (room,x,y)
}

uint8_t itemIdOfType(char type) {
  uint8_t id = 0;
  for (uint8_t r = 0; r < NUM_ROOMS; r++) {
    for (uint8_t yy = 0; yy < MATRIX_H; yy++) {
      for (uint8_t xx = 0; xx < MATRIX_W; xx++) {
        char c = cell(r, xx, yy);
        if (c == type) return id;
        if (isItem(c)) id++;
      }
    }
  }
  return 0xFF;  // not in the world
}

int8_t doorIndexAt(uint8_t room, uint8_t x, uint8_t y) {
  (void)x;
  uint8_t edge;
  if (y == 0) edge = room - WORLD_W;       // door on my north edge
  else if (y == MATRIX_H - 1) edge = room; // door on my south edge
  else return -1;                          // no horizontal doors exist
  for (int8_t i = 0; i < NUM_DOORS; i++) {
    if (pgm_read_byte(&DOOR_EDGES[i]) == edge) return i;
  }
  return -1;
}

void findStart(uint8_t* room, int8_t* x, int8_t* y) {
  for (uint8_t r = 0; r < NUM_ROOMS; r++) {
    for (uint8_t yy = 0; yy < MATRIX_H; yy++) {
      for (uint8_t xx = 0; xx < MATRIX_W; xx++) {
        if (cell(r, xx, yy) == '@') {
          *room = r;
          *x = xx;
          *y = yy;
          return;
        }
      }
    }
  }
}

}  // namespace world
