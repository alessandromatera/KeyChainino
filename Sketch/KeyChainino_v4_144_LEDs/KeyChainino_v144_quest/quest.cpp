/*
 * quest.cpp — a tiny adventure in 144 pixels.
 *
 * Controls (the multigame two-button language, bent to adventuring):
 *   tap A          — turn 90° clockwise (one turn per physical press)
 *   tap/hold B     — step / walk where you're facing (auto-repeat walks)
 *   tap A+B        — sword strike into the cell you face (once you own it)
 *   hold A+B 1.2s  — save and sleep; KEEP holding ~3s more to wipe the
 *                    save and start a fresh quest (a bar fills as you hold)
 *
 * Reading the screen (three honest brightness levels, see board.h):
 *   static dim        walls
 *   dim, winking off  a locked door
 *   solid bright      YOU (fast-blinking = just hurt, invulnerable)
 *   bright, dim dips  an enemy (the muncher-ghost language)
 *   2x2 bright block  the boss
 *   slow on/off flash an item: key, heart, the sword, the treasure
 *   single dim pixel ahead of you: where you're facing / where B walks
 *   row 0 pips (after any event): hearts on the left, keys on the right
 *
 * The world state — position, hearts, keys, opened doors, taken items —
 * is mirrored to EEPROM on every room change and event, so sleeping (or
 * a dead battery) never loses the quest. Death is forgiving: the world
 * keeps your progress, you wake at home with full hearts.
 */
#include "quest.h"

#include "display.h"
#include "gfx.h"
#include "icons.h"
#include "input.h"
#include "power.h"
#include "save.h"
#include "text.h"
#include "world.h"

namespace quest {

#define FRAME_MS 20
#define IDLE_SLEEP_MS 30000UL

#define MAX_HEARTS 3
#define BOSS_HP 3

/* Pacing. Walking cadence is set by input auto-repeat (150ms); the step
   cooldown only guards the burst of a tap + immediate repeat. */
#define STEP_COOL_MS 120
#define SWORD_COOL_MS 300
#define SWORD_FX_MS 100
#define DOOR_FX_MS 240
#define IFRAMES_MS 900
#define ENTER_GRACE_MS 600   // can't be hit right after a room change
#define OVERLAY_MS 1500
#define FOE_STEP_MS 540
#define FOE_STEP_DUNGEON_MS 420
#define FOE_GREEDY_PCT 60    // % of enemy steps that chase; rest wander
#define BOSS_STEP_MS 380
#define BOSS_HURT_MS 350
#define RESET_HOLD_MS 2800   // extra A+B hold beyond the exit point

/* Blink language (all 20ms multiples) */
#define FOE_ON_MS 260
#define FOE_DIP_MS 60
#define ITEM_ON_MS 360
#define ITEM_OFF_MS 300
#define DOOR_ON_MS 460
#define DOOR_OFF_MS 160
#define HURT_BLINK_MS 80

/* ---- persistent quest state (mirrored in EEPROM) ---- */
static uint8_t room;
static int8_t px, py;
static uint8_t facing;  // 0 N, 1 E, 2 S, 3 W
static uint8_t hearts;
static uint8_t keys;
static uint16_t taken;
static uint8_t doorsOpen;
static bool hasSword;   // derived: taken bit of the sword item
static uint8_t swordId;

/* ---- the loaded room ---- */
static uint16_t walls[MATRIX_H];  // bit x of row y

struct Item { int8_t x, y; uint8_t id; char type; };
static Item items[4];
static uint8_t nItems;

struct Door { int8_t x, y; uint8_t idx; };  // still-locked doors here
static Door doors[2];
static uint8_t nDoors;

struct Foe { int8_t x, y; bool alive; };
static Foe foes[4];
static uint8_t nFoes;

static bool bossHere, bossAlive;
static int8_t bx, by;  // top-left of the 2x2 body
static uint8_t bossHp;

/* ---- timers & effects ---- */
static uint16_t stepCool, swordCool, iFrames, overlayMs;
static uint16_t fxMs;              // bright flash cell (sword / door opening)
static int8_t fxX, fxY;
static uint16_t doorFxMs;          // locked-door refusal flash
static int8_t dfxX, dfxY;
static uint16_t bossHurtMs;
static uint16_t foeAcc, bossAcc;
static uint16_t foeBlink, itemBlink, doorBlink, hurtBlink;
static bool foeBright, itemOn, doorOn, playerOn;
static uint32_t lastFrame, lastPress;
static bool aArmed;  // one rotate per physical A press
static bool hurtVis; // iFrames came from damage: blink the player

/* ------------------------------------------------------------------ */

static inline void deltaFor(uint8_t dir, int8_t* dx, int8_t* dy) {
  switch (dir) {
    case 0: *dx = 0; *dy = -1; break;
    case 1: *dx = 1; *dy = 0; break;
    case 2: *dx = 0; *dy = 1; break;
    default: *dx = -1; *dy = 0; break;
  }
}

static inline bool inGrid(int8_t x, int8_t y) {
  return (uint8_t)x < MATRIX_W && (uint8_t)y < MATRIX_H;
}
static inline bool wallAt(int8_t x, int8_t y) {
  return (walls[(uint8_t)y] & ((uint16_t)1 << (uint8_t)x)) != 0;
}
static int8_t doorAt(int8_t x, int8_t y) {
  for (uint8_t i = 0; i < nDoors; i++) {
    if (doors[i].x == x && doors[i].y == y) return i;
  }
  return -1;
}
static int8_t foeAt(int8_t x, int8_t y) {
  for (uint8_t i = 0; i < nFoes; i++) {
    if (foes[i].alive && foes[i].x == x && foes[i].y == y) return i;
  }
  return -1;
}
static int8_t itemAt(int8_t x, int8_t y) {
  for (uint8_t i = 0; i < nItems; i++) {
    if (items[i].x == x && items[i].y == y) return i;
  }
  return -1;
}
static inline bool inBoss(int8_t x, int8_t y) {
  return bossHere && bossAlive && x >= bx && x <= bx + 1 && y >= by &&
         y <= by + 1;
}

static void storeState() {
  save::State s;
  s.room = room; s.x = px; s.y = py; s.facing = facing;
  s.hearts = hearts; s.keys = keys; s.taken = taken; s.doors = doorsOpen;
  save::store(&s);
}

static void loadRoom() {
  nItems = nDoors = nFoes = 0;
  bossHere = false;
  for (uint8_t y = 0; y < MATRIX_H; y++) {
    walls[y] = 0;
    for (uint8_t x = 0; x < MATRIX_W; x++) {
      char c = world::cell(room, x, y);
      switch (c) {
        case '#':
          walls[y] |= (uint16_t)1 << x;
          break;
        case 'D': {
          int8_t di = world::doorIndexAt(room, x, y);
          if (di >= 0 && !(doorsOpen & (1 << di))) {
            doors[nDoors].x = x; doors[nDoors].y = y;
            doors[nDoors].idx = di; nDoors++;
          }
          break;  // an open (or unknown) door is plain floor
        }
        case 'K': case 'H': case 'S': case 'T': {
          uint8_t id = world::itemIdAt(room, x, y);
          if (c != 'T' && (taken & ((uint16_t)1 << id))) break;
          items[nItems].x = x; items[nItems].y = y;
          items[nItems].id = id; items[nItems].type = c; nItems++;
          break;
        }
        case 'E':
          if (nFoes < (uint8_t)(sizeof(foes) / sizeof(foes[0]))) {
            foes[nFoes].x = x; foes[nFoes].y = y;
            foes[nFoes].alive = true; nFoes++;
          }
          break;
        case 'B':
          bossHere = true; bossAlive = true;
          bx = x; by = y; bossHp = BOSS_HP;
          break;
        default:
          break;  // '.', '@'
      }
    }
  }
  foeAcc = bossAcc = 0;
  iFrames = ENTER_GRACE_MS;  // silent grace: protected but not blinking
  hurtVis = false;
}

/* ------------------------------- drawing -------------------------- */

static void draw() {
  gfx::clear();
  for (uint8_t y = 0; y < MATRIX_H; y++) {
    uint16_t w = walls[y];
    if (!w) continue;
    for (uint8_t x = 0; x < MATRIX_W; x++) {
      if (w & ((uint16_t)1 << x)) gfx::px(x, y, BR_DIM);
    }
  }
  for (uint8_t i = 0; i < nDoors; i++) {
    gfx::px(doors[i].x, doors[i].y, doorOn ? BR_DIM : BR_OFF);
  }

  /* Facing marker: a dim pixel on the open cell you'd walk into (drawn
     under everything alive — if it's hidden by an enemy or item, that IS
     what you're facing). No marker means you face something solid. */
  {
    int8_t dx, dy;
    deltaFor(facing, &dx, &dy);
    int8_t mx = px + dx, my = py + dy;
    if (inGrid(mx, my) && !wallAt(mx, my) && doorAt(mx, my) < 0) {
      gfx::px(mx, my, BR_DIM);
    }
  }

  for (uint8_t i = 0; i < nItems; i++) {
    gfx::px(items[i].x, items[i].y, itemOn ? BR_MAX : BR_OFF);
  }
  for (uint8_t i = 0; i < nFoes; i++) {
    if (foes[i].alive) gfx::px(foes[i].x, foes[i].y, foeBright ? BR_MAX : BR_DIM);
  }
  if (bossHere && bossAlive) {
    uint8_t v = (bossHurtMs && ((bossHurtMs / HURT_BLINK_MS) & 1)) ? BR_DIM
                                                                   : BR_MAX;
    gfx::px(bx, by, v);     gfx::px(bx + 1, by, v);
    gfx::px(bx, by + 1, v); gfx::px(bx + 1, by + 1, v);
  }

  if (playerOn) gfx::px(px, py, BR_MAX);

  if (fxMs) gfx::px(fxX, fxY, BR_MAX);
  if (doorFxMs) gfx::px(dfxX, dfxY, BR_MAX);

  if (overlayMs) {  // hearts left, keys right, on the top border row
    for (uint8_t i = 0; i < MAX_HEARTS; i++) {
      gfx::px(i, 0, i < hearts ? BR_MAX : BR_OFF);
    }
    for (uint8_t i = 0; i < keys; i++) {
      gfx::px(MATRIX_W - 1 - i, 0, BR_MAX);
    }
  }
}

static void showFace(const uint16_t* iconP, uint16_t holdMs) {
  gfx::clear();
  gfx::drawIconP(iconP);
  display::flush();
  delay(holdMs);
  gfx::clear();
  display::flush();
}

/* Blocking PROGMEM scroll, any button skips (the engine's idiom). */
static void scrollMsg(const char* msgP) {
  gfx::clear();
  display::flush();
  text::startP(msgP);
  uint32_t last = millis();
  input::flush();
  while (text::active()) {
    input::poll();
    if (input::tapA() || input::tapB() || input::chordTap() ||
        input::exitHold()) {
      break;
    }
    if (millis() - last >= TEXT_STEP_MS) {
      last += TEXT_STEP_MS;
      text::step();
    }
  }
  text::stop();
  gfx::clear();
  display::flush();
}

/* --------------------------- round lifecycle ---------------------- */

static void newGame(bool withTitle) {
  world::findStart(&room, &px, &py);
  facing = 0;  // home faces the sword cave to the north
  hearts = MAX_HEARTS;
  keys = 0;
  taken = 0;
  doorsOpen = 0;
  hasSword = false;
  storeState();
  if (withTitle) {
    gfx::clear();
    gfx::drawIconP(ICON_TREASURE);
    display::flush();
    delay(900);
    scrollMsg(PSTR("QUEST"));
  }
  loadRoom();
  overlayMs = OVERLAY_MS;
  input::flush();
  aArmed = false;
}

static void winQuest() {
  for (int8_t r = MATRIX_H - 1; r >= 0; r--) {  // triumph sweep
    gfx::hline(0, MATRIX_W - 1, r, BR_MAX);
    display::flush();
    delay(25);
  }
  delay(250);
  showFace(KeyChaininoFace, 700);
  scrollMsg(PSTR("YOU WIN"));
  save::wipe();
  newGame(true);  // a fresh quest, title and all
}

static void die() {
  showFace(KeyChaininoSadFace, 800);
  scrollMsg(PSTR("GAME OVER"));
  /* Forgiving: the world keeps every key, door and item; you wake at
     home with full hearts. */
  hearts = MAX_HEARTS;
  world::findStart(&room, &px, &py);
  facing = 0;
  storeState();
  loadRoom();
  overlayMs = OVERLAY_MS;
}

/* Hurt the player, shoving them one cell along (kx,ky) if it's free.
   Returns true if this was the last heart (world already rebuilt). */
static bool hurtPlayer(int8_t kx, int8_t ky) {
  if (iFrames) return false;
  gfx::invert(); display::flush(); delay(70);
  gfx::invert(); display::flush(); delay(40);
  int8_t tx = px + kx, ty = py + ky;
  if (inGrid(tx, ty) && !wallAt(tx, ty) && doorAt(tx, ty) < 0 &&
      foeAt(tx, ty) < 0 && !inBoss(tx, ty)) {
    px = tx; py = ty;
  }
  hearts--;
  overlayMs = OVERLAY_MS;
  if (hearts == 0) {
    die();
    return true;
  }
  iFrames = IFRAMES_MS;
  hurtVis = true;
  return false;
}

/* ------------------------------ actions --------------------------- */

static void pickup(uint8_t i) {
  char type = items[i].type;
  if (type == 'T') {
    winQuest();
    return;
  }
  if (type == 'H' && hearts >= MAX_HEARTS) return;  // stays for later
  if (type == 'H') hearts++;
  if (type == 'K') keys++;
  if (type == 'S') hasSword = true;
  taken |= (uint16_t)1 << items[i].id;
  items[i] = items[--nItems];
  overlayMs = OVERLAY_MS;
  storeState();
  if (type == 'S') scrollMsg(PSTR("SWORD"));  // dangerous to go alone
}

static void tryDoor(uint8_t i) {
  if (keys == 0) {  // rattle: the door flashes, you stay
    dfxX = doors[i].x; dfxY = doors[i].y; doorFxMs = DOOR_FX_MS;
    return;
  }
  keys--;
  doorsOpen |= (uint8_t)1 << doors[i].idx;
  fxX = doors[i].x; fxY = doors[i].y; fxMs = SWORD_FX_MS;  // swing-open flash
  doors[i] = doors[--nDoors];
  overlayMs = OVERLAY_MS;
  storeState();
}

static void changeRoom(int8_t dx, int8_t dy) {
  room = (uint8_t)((int8_t)room + dx + WORLD_W * dy);
  if (dx == 1) px = 0;
  else if (dx == -1) px = MATRIX_W - 1;
  if (dy == 1) py = 0;
  else if (dy == -1) py = MATRIX_H - 1;
  storeState();
  gfx::clear();
  display::flush();
  delay(90);
  loadRoom();
}

static void attemptStep() {
  int8_t dx, dy;
  deltaFor(facing, &dx, &dy);
  int8_t nx = px + dx, ny = py + dy;
  if (!inGrid(nx, ny)) {  // only reachable through a border gap
    changeRoom(dx, dy);
    return;
  }
  int8_t di = doorAt(nx, ny);
  if (di >= 0) {
    tryDoor(di);
    return;
  }
  if (wallAt(nx, ny)) return;
  if (inBoss(nx, ny)) {
    hurtPlayer(-dx, -dy);
    return;
  }
  if (foeAt(nx, ny) >= 0) {  // walking into an enemy hurts YOU
    hurtPlayer(-dx, -dy);
    return;
  }
  px = nx; py = ny;
  int8_t ii = itemAt(px, py);
  if (ii >= 0) pickup((uint8_t)ii);
}

static void trySword() {
  if (!hasSword || swordCool) return;
  swordCool = SWORD_COOL_MS;
  int8_t dx, dy;
  deltaFor(facing, &dx, &dy);
  int8_t sx = px + dx, sy = py + dy;
  if (!inGrid(sx, sy)) return;
  fxX = sx; fxY = sy; fxMs = SWORD_FX_MS;
  int8_t fi = foeAt(sx, sy);
  if (fi >= 0) {
    foes[fi].alive = false;
    return;
  }
  if (inBoss(sx, sy)) {
    if (--bossHp == 0) {
      bossAlive = false;
      gfx::invert(); display::flush(); delay(80);
      gfx::invert(); display::flush(); delay(80);
      gfx::invert(); display::flush(); delay(80);
      gfx::invert(); display::flush();
    } else {
      bossHurtMs = BOSS_HURT_MS;
      /* shove it back along the strike */
      int8_t tx = bx + dx, ty = by + dy;
      bool ok = true;
      for (uint8_t cy = 0; cy < 2 && ok; cy++) {
        for (uint8_t cx = 0; cx < 2 && ok; cx++) {
          int8_t cxx = tx + cx, cyy = ty + cy;
          if (!inGrid(cxx, cyy) || wallAt(cxx, cyy) ||
              doorAt(cxx, cyy) >= 0 || foeAt(cxx, cyy) >= 0 ||
              (cxx == px && cyy == py)) {
            ok = false;
          }
        }
      }
      if (ok) { bx = tx; by = ty; }
    }
  }
}

/* ------------------------------ monsters -------------------------- */

static bool foeCellFree(int8_t x, int8_t y) {
  return inGrid(x, y) && !wallAt(x, y) && doorAt(x, y) < 0 &&
         foeAt(x, y) < 0 && !inBoss(x, y);
}

/* One step for every enemy: adjacent ones strike, the rest mostly chase,
   sometimes wander (a perfect chaser is unescapable on 12x12).
   Returns true if the player just died (room rebuilt — stop iterating). */
static bool stepFoes() {
  for (uint8_t i = 0; i < nFoes; i++) {
    if (!foes[i].alive) continue;
    int8_t ddx = px - foes[i].x, ddy = py - foes[i].y;
    if ((int8_t)(abs(ddx) + abs(ddy)) == 1) {  // adjacent: attack
      if (!iFrames) {
        if (hurtPlayer(ddx, ddy)) return true;
      }
      continue;
    }
    uint8_t cand[4], n = 0;
    for (uint8_t d = 0; d < 4; d++) {
      int8_t dx, dy;
      deltaFor(d, &dx, &dy);
      if (foeCellFree(foes[i].x + dx, foes[i].y + dy)) cand[n++] = d;
    }
    if (n == 0) continue;
    uint8_t chosen;
    if (random(0, 100) < FOE_GREEDY_PCT) {
      uint8_t best = 0;
      int16_t bestDist = 32767;
      for (uint8_t c = 0; c < n; c++) {
        int8_t dx, dy;
        deltaFor(cand[c], &dx, &dy);
        int16_t dist = abs((int16_t)(foes[i].x + dx) - px) +
                       abs((int16_t)(foes[i].y + dy) - py);
        if (dist < bestDist) { bestDist = dist; best = c; }
      }
      chosen = cand[best];
    } else {
      chosen = cand[random(0, n)];
    }
    int8_t dx, dy;
    deltaFor(chosen, &dx, &dy);
    foes[i].x += dx;
    foes[i].y += dy;
  }
  return false;
}

static bool bossFits(int8_t tx, int8_t ty) {
  for (uint8_t cy = 0; cy < 2; cy++) {
    for (uint8_t cx = 0; cx < 2; cx++) {
      int8_t x = tx + cx, y = ty + cy;
      if (!inGrid(x, y) || wallAt(x, y) || doorAt(x, y) >= 0 ||
          foeAt(x, y) >= 0 || (x == px && y == py)) {
        return false;
      }
    }
  }
  return true;
}

static bool stepBoss() {
  /* Adjacent (or overlapped) player: strike instead of moving. */
  int8_t cdx = (px < bx) ? (int8_t)(px - bx) : (px > bx + 1) ? (int8_t)(px - bx - 1) : 0;
  int8_t cdy = (py < by) ? (int8_t)(py - by) : (py > by + 1) ? (int8_t)(py - by - 1) : 0;
  if ((int8_t)(abs(cdx) + abs(cdy)) <= 1) {
    if (!iFrames) {
      int8_t kx = (cdx > 0) - (cdx < 0);
      int8_t ky = (cdy > 0) - (cdy < 0);
      if (kx == 0 && ky == 0) ky = 1;  // overlapped: shove south
      return hurtPlayer(kx, ky);
    }
    return false;
  }
  /* Lumber toward the player, longer axis first. */
  int8_t sx = (cdx > 0) - (cdx < 0);
  int8_t sy = (cdy > 0) - (cdy < 0);
  if (abs(cdx) >= abs(cdy)) {
    if (sx && bossFits(bx + sx, by)) { bx += sx; return false; }
    if (sy && bossFits(bx, by + sy)) { by += sy; return false; }
  } else {
    if (sy && bossFits(bx, by + sy)) { by += sy; return false; }
    if (sx && bossFits(bx + sx, by)) { bx += sx; return false; }
  }
  return false;
}

/* ------------------------------ the loop -------------------------- */

static inline void tickDown(uint16_t* t) {
  *t = (*t > FRAME_MS) ? (uint16_t)(*t - FRAME_MS) : 0;
}

static void frame() {
  bool frozen = input::chordHeld();  // world holds its breath mid-gesture

  if (!input::downA()) aArmed = true;
  if (input::tapA() && aArmed) {
    aArmed = false;
    facing = (facing + 1) & 3;
  }
  if (input::tapB() && stepCool == 0) {
    stepCool = STEP_COOL_MS;
    attemptStep();
  }
  if (input::chordTap()) trySword();

  tickDown(&stepCool);
  tickDown(&swordCool);
  tickDown(&iFrames);
  tickDown(&fxMs);
  tickDown(&doorFxMs);
  tickDown(&bossHurtMs);
  tickDown(&overlayMs);

  if (!frozen) {
    foeAcc += FRAME_MS;
    uint16_t period = (room == 7) ? FOE_STEP_DUNGEON_MS : FOE_STEP_MS;
    if (foeAcc >= period) {
      foeAcc = 0;
      if (stepFoes()) { draw(); return; }
    }
    if (bossHere && bossAlive) {
      bossAcc += FRAME_MS;
      if (bossAcc >= BOSS_STEP_MS) {
        bossAcc = 0;
        if (stepBoss()) { draw(); return; }
      }
    }
  }

  foeBlink += FRAME_MS;
  if (foeBlink >= (foeBright ? FOE_ON_MS : FOE_DIP_MS)) {
    foeBlink = 0;
    foeBright = !foeBright;
  }
  itemBlink += FRAME_MS;
  if (itemBlink >= (itemOn ? ITEM_ON_MS : ITEM_OFF_MS)) {
    itemBlink = 0;
    itemOn = !itemOn;
  }
  doorBlink += FRAME_MS;
  if (doorBlink >= (doorOn ? DOOR_ON_MS : DOOR_OFF_MS)) {
    doorBlink = 0;
    doorOn = !doorOn;
  }
  if (iFrames && hurtVis) {
    hurtBlink += FRAME_MS;
    if (hurtBlink >= HURT_BLINK_MS) {
      hurtBlink = 0;
      playerOn = !playerOn;
    }
  } else {
    playerOn = true;
  }

  draw();
}

static void wakeUp() {
  input::flush();
  showFace(KeyChaininoFace, 350);
  overlayMs = OVERLAY_MS;
  iFrames = ENTER_GRACE_MS;
  hurtVis = false;
  lastFrame = lastPress = millis();
}

static void sleepNow() {
  storeState();
  gfx::clear();
  display::flush();
  power::sleepUntilWake();
  wakeUp();
}

/* The exit chord fired (1.2s of A+B). Release = save and sleep. Keep
   holding and a bright bar grows along the bottom row: when it spans the
   screen (~3s more) the save is wiped and a fresh quest begins. */
static void exitFlow() {
  uint32_t t0 = millis();
  bool reset = false;
  while (input::downA() && input::downB()) {
    uint32_t held = millis() - t0;
    if (held >= RESET_HOLD_MS) {
      reset = true;
      break;
    }
    draw();
    gfx::hline(0, (int8_t)(held * (MATRIX_W - 1) / RESET_HOLD_MS), MATRIX_H - 1,
               BR_MAX);
    display::flush();
    delay(30);
  }
  if (reset) {
    gfx::fillAll(BR_MAX); display::flush(); delay(150);
    gfx::clear(); display::flush(); delay(150);
    save::wipe();
    newGame(true);
    lastFrame = lastPress = millis();
  } else {
    sleepNow();
  }
  input::flush();
}

void boot() {
  /* boot() runs right after the human wake gesture: micros() is decent
     entropy. (analogRead would hang here — display::begin() already
     powered the ADC off; the .ino seeds from the floating pin earlier.) */
  randomSeed(micros());
  swordId = world::itemIdOfType('S');
  save::State s;
  if (save::load(&s)) {
    room = s.room; px = s.x; py = s.y; facing = s.facing;
    hearts = s.hearts; keys = s.keys; taken = s.taken; doorsOpen = s.doors;
    hasSword = (taken & ((uint16_t)1 << swordId)) != 0;
    showFace(KeyChaininoFace, 400);
    loadRoom();
    overlayMs = OVERLAY_MS;
    input::flush();
  } else {
    newGame(true);
  }
  playerOn = foeBright = itemOn = doorOn = true;
  lastFrame = lastPress = millis();
}

void tick() {
  input::poll();
  if (input::exitHold()) {
    exitFlow();
    return;
  }
  uint32_t now = millis();
  if (input::downA() || input::downB()) lastPress = now;
  if (now - lastPress >= IDLE_SLEEP_MS) {  // pocketed: save and sleep
    sleepNow();
    return;
  }
  if (now - lastFrame >= FRAME_MS) {
    lastFrame += FRAME_MS;
    /* after a blocking ceremony skip the lost frames, don't burst */
    if (now - lastFrame > 4 * FRAME_MS) lastFrame = now;
    frame();
    display::flush();
  }
}

}  // namespace quest
