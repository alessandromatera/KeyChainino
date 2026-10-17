/*************************************************************************
  PICTURES FOR KEYCHAININO 144
  https://www.keychainino.com/product/keychainino-144

  A picture book for small hands. The KeyChainino shows one picture at a
  time; the buttons flip through them with a swift slide, and every
  picture that arrives plays its little animation:

    A (left)  — previous picture
    B (right) — next picture
    A + B     — the picture comes alive again: built-in pictures play their own
                little animation, the child's own drawings get a different
                trick every time (hop, jelly, sparkle, wave, flip, beat,
                burst, spin)

  Waking: press A+B together. The last picture rises back into view and
  says hello. After 30 seconds without a press it sinks away and the
  KeyChainino goes to sleep (about 1uA typical).

  The pictures are the child's drawings first (newest first), then the
  built-ins chosen for this KeyChainino (all of tools/pictures.mjs is in
  flash; a fresh one shows smile, heart, star, dog, car). Drawings and the
  choice are made on the web page upload/keychainino-pictures.html, which
  writes them into the EEPROM over the ISP programmer — see album.h.

  Build: node tools/build.mjs   (regenerates pictures_data.*, compiles,
  and embeds the firmware + picture library into the web page)

  board.h, display.*, gfx.* and power.* are copies of the shared v144
  engine in ../KeyChainino_v144_library (edit there, then run its
  sync.sh); input.* is this sketch's own, tuned for a 3-year-old.

  created by Alessandro Matera
* ************************************************************************/

#include "album.h"
#include "board.h"
#include "pictures_data.h"
#include "display.h"
#include "input.h"
#include "power.h"
#include "show.h"

#define IDLE_SLEEP_MS 30000UL  // no press this long -> bedtime

static uint32_t lastPlay;

void setup() {
  randomSeed(analogRead(A0));  // floating matrix pin, read before the ADC goes down

  display::begin();  // starts the scan, powers off unused peripherals
  /* No brown-out detector to switch off: the fuses keep it off from
     power-on (efuse 0xFF, written and checked by the upload page). */

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);

  album::begin();
  input::flush();
  (void)pgm_read_byte(&BUILD_ID[0]);  // keep the build id in flash: the web page checks it

  /* Power-on (battery in, or the web page just sent a drawing): show the
     current picture right away — after a send, that is the new drawing. */
  show::wake();
  lastPlay = millis();
}

void loop() {
  input::poll();

  if (input::tapB()) {
    album::step(+1);
    show::slide(+1);
  }
  if (input::tapA()) {
    album::step(-1);
    show::slide(-1);
  }
  if (input::chord()) show::animate();
  if (input::activity()) lastPlay = millis();

  show::tick();

  if (millis() - lastPlay >= IDLE_SLEEP_MS) {
    show::bedtime();
    album::saveSelection();
    power::sleepUntilWake();  // returns after the A+B wake gesture
    input::flush();           // that chord is not an "animate" press
    show::wake();
    lastPlay = millis();
  }
}
