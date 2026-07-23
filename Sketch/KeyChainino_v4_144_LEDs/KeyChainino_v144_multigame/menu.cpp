#include "menu.h"

#include "display.h"
#include "engine.h"
#include "gfx.h"
#include "input.h"
#include "power.h"
#include "save.h"
#include "text.h"

namespace menu {

#define NAME_SETTLE_MS 500    // pause on a game this long -> scroll its name
#define IDLE_SLEEP_MS 25000UL // no input this long -> power down

static uint8_t selected;
static uint32_t lastNav, lastActivity, lastTextStep;
static bool nameShown;

void begin() {
  selected = save::lastGame();
  if (selected >= engine::numGames()) selected = 0;
}

static void drawIcon() {
  gfx::clear();
  GameDef g;
  engine::gameDef(selected, &g);
  gfx::drawIconP(g.icon);
  display::flush();
}

void enter() {
  input::flush();  // swallow buttons held from wake gesture / game exit
  text::stop();
  drawIcon();
  lastNav = lastActivity = millis();
  nameShown = false;
}

/* Quick iris-out flash confirming the launch */
static void confirmAnimation() {
  for (uint8_t r = 1; r < 9; r += 2) {
    gfx::clear();
    gfx::circle(5, 5, r, BR_MAX);
    display::flush();
    delay(45);
  }
  gfx::clear();
  display::flush();
}

int8_t tick() {
  bool navA = input::tapA();
  bool navB = input::tapB();
  uint8_t n = engine::numGames();

  if (navA || navB) {
    text::stop();
    selected = (selected + (navB ? 1 : n - 1)) % n;
    drawIcon();
    lastNav = lastActivity = millis();
    nameShown = false;
  }

  /* Settled long enough: scroll the game's name once (it pushes the icon
     off the screen, then the icon comes back). */
  if (!nameShown && millis() - lastNav >= NAME_SETTLE_MS) {
    GameDef g;
    engine::gameDef(selected, &g);
    text::startP(g.name);
    lastTextStep = millis();
    nameShown = true;
  }
  if (text::active()) {
    if (millis() - lastTextStep >= TEXT_STEP_MS) {
      lastTextStep += TEXT_STEP_MS;
      if (!text::step()) drawIcon();  // name gone: icon back
    }
  }

  if (input::chordTap()) {
    text::stop();
    save::setLastGame(selected);
    confirmAnimation();
    return (int8_t)selected;
  }

  input::exitHold();  // chord-hold means nothing here: discard

  if (!input::idle()) lastActivity = millis();

  if (millis() - lastActivity >= IDLE_SLEEP_MS) {
    text::stop();
    power::sleepUntilWake();  // returns after the A+B wake gesture
    enter();
  }

  return -1;
}

}  // namespace menu
