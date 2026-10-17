#include "engine.h"

#include "display.h"
#include "gfx.h"
#include "icons.h"
#include "input.h"
#include "save.h"
#include "text.h"

/* No button for this long mid-round = the board was put down: give the
   round up so the sketch powers down. The next A+B wake starts fresh
   (Snake never ends on its own if you walk away — it just wraps forever). */
#define IDLE_SLEEP_MS 30000UL

namespace engine {

uint16_t score;

void gameDef(uint8_t idx, GameDef* out) {
  memcpy_P(out, &GAME_TABLE[idx], sizeof(GameDef));
}

void showFaceP(const uint16_t* iconP, uint16_t holdMs) {
  gfx::clear();
  gfx::drawIconP(iconP);
  display::flush();
  delay(holdMs);
  gfx::clear();
  display::flush();
}

/* Pump a started text scroll to completion; any button skips it. */
static void scrollRun() {
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

void scrollP(const char* msgP) {
  gfx::clear();
  display::flush();
  text::startP(msgP);
  scrollRun();
}

void scrollNumP(const char* labelP, uint16_t n) {
  static char num[6];
  utoa(n, num, 10);
  gfx::clear();
  display::flush();
  text::startP2(labelP, num);
  scrollRun();
}

static void endRound(uint8_t idx, const GameDef* g, uint8_t result) {
  input::flush();
  if (result == GAME_WIN) {
    showFaceP(KeyChaininoFace, 600);
    scrollP(PSTR("YOU WIN"));
  } else {
    showFaceP(KeyChaininoSadFace, 600);
  }
  if (!(g->flags & GF_SCORELESS)) {
    scrollNumP(g->scoreLabel, score);
    /* Always follow the score with the EEPROM record: RECORD! when this
       round just set it (the record is the score on screen), otherwise
       the standing best to beat. */
    if (save::submitScore(idx, score)) {
      scrollP(PSTR("RECORD!"));
    } else {
      scrollNumP(PSTR("RECORD "), save::hiScore(idx));
    }
  }
}

void runGame(uint8_t idx) {
  GameDef g;
  gameDef(idx, &g);

  score = 0;
  showFaceP(KeyChaininoFace, 500);
  input::flush();
  randomSeed(micros());  // human timing = decent entropy for the round

  gfx::clear();
  g.reset();
  display::flush();

  uint32_t last = millis();
  uint32_t lastPress = last;
  for (;;) {
    input::poll();
    if (input::exitHold()) return;  // A+B hold: straight to sleep, no ceremony

    uint32_t now = millis();
    if (input::downA() || input::downB()) lastPress = now;
    if (now - lastPress >= IDLE_SLEEP_MS) return;  // abandoned: the round is
                                                   // dropped, the sketch sleeps
    if (now - last >= FRAME_MS) {
      last += FRAME_MS;
      /* after a blocking effect (or any hiccup) skip the lost frames
         instead of bursting to catch up */
      if (now - last > 4 * FRAME_MS) last = now;

      uint8_t r = g.frame();
      display::flush();
      if (r != GAME_ON) {
        endRound(idx, &g, r);
        return;
      }
    }
  }
}

}  // namespace engine
