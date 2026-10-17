/*************************************************************************
  WATCH FIRMWARE FOR KEYCHAININO 144
  https://www.keychainino.com/product/keychainino-144

  A standalone clock, independent of the multi-game firmware:
    tap A+B together  -> wake, scroll the current time once, sleep again
    hold A+B ~1.2s     -> SET mode: A adjusts the flashing field (hold to
                          fast-forward), B confirms and moves on
                          (HOUR -> MINUTE -> save & sleep)

  Timekeeping note: this board has no external crystal. While asleep the
  only oscillator still running is the ATmega328P's internal ~128kHz
  watchdog RC, so the clock is kept by waking on the watchdog interrupt
  every ~8s and counting ticks -- the same trick every "WDT as a timer"
  low-power AVR project uses. Per the datasheet that oscillator is only
  rated 76-180kHz (128kHz typical) across voltage/temperature, i.e. can be
  off by several percent, which is minutes/day -- there's no way to do
  better without an external 32.768kHz crystal on TOSC1/TOSC2, which this
  PCB doesn't have. Re-set the time occasionally, like a cheap watch.

  Power note: the multi-game firmware already spends nearly all its time
  in this same SLEEP_MODE_PWR_DOWN (~40uA typical per datasheet). Enabling
  the watchdog for ticking adds only ~4uA on top of that (~44uA total,
  per the ATmega328P datasheet's "Power-Down Supply Current" table) --
  on a 220mAh CR2032 that's still many months of runtime, comparable to
  what the keychain already gets between game sessions today.

  created by Alessandro Matera
* ************************************************************************/

#include "KeyChainino.h"
#include <EEPROM.h>

/* --- Timing --- */
#define SET_ENTER_HOLD_MS   1200UL  // hold A+B this long from sleep to enter SET mode
#define SET_REPEAT_DELAY_MS 500UL   // delay before holding A auto-repeats in SET mode
#define SET_REPEAT_MS       180UL   // auto-repeat period while holding A in SET mode
#define SET_IDLE_TIMEOUT_MS 15000UL // abandon SET mode after this much inactivity
#define DEBOUNCE_MS         20UL

/* --- EEPROM layout --- */
#define EE_MAGIC_ADDR  0
#define EE_MAGIC_VALUE 0x57  // 'W' -- bump if this layout ever changes
#define EE_HOUR        1
#define EE_MINUTE      2

/* --- Clock state --- lives in SRAM, which survives power-down sleep; only
   lost if the battery is pulled (restored from EEPROM on the next boot).
   Ticked by the watchdog interrupt every ~8s so it keeps advancing while
   the MCU is otherwise fully asleep. */
/* WDTO_8S is nominally 2^20 watchdog-oscillator cycles / 128kHz = 8.192s,
   not a flat 8s -- accumulating in whole seconds would throw away ~2.4%
   accuracy for free (34 min/day) on top of the oscillator's own tolerance. */
static const uint16_t WDT_TICK_MS = 8192;
volatile uint16_t g_minutesOfDay = 0;  // 0..1439
volatile uint32_t g_msAccum = 0;       // 0..59999, sub-minute remainder;
                                        // only ever touched inside this ISR

ISR(WDT_vect) {
  WDTCSR |= (1 << WDIE);  // re-arm interrupt-only mode: hardware clears WDIE
                           // on entry, and a missed re-arm just stops the
                           // clock (WDE is never set, so no reset risk)
  g_msAccum += WDT_TICK_MS;
  while (g_msAccum >= 60000UL) {
    g_msAccum -= 60000UL;
    g_minutesOfDay++;
    if (g_minutesOfDay >= 1440) g_minutesOfDay = 0;
  }
}

/* Empty button ISRs: they exist only so INT0/INT1 can wake the MCU from
   power-down inside KC.goSleep(). All gesture decoding below is polled. */
ISR(INT1_vect) {}  // BUTTON A
ISR(INT0_vect) {}  // BUTTON B

static bool pressedA() { return digitalRead(BUTTON_A) == LOW; }
static bool pressedB() { return digitalRead(BUTTON_B) == LOW; }

/* Re-arms the watchdog in interrupt-only (WDE=0) mode at ~8s -- KC.init()
   calls wdt_disable(), so this must run after it. */
static void wdtSetup() {
  cli();
  wdt_reset();
  MCUSR &= ~(1 << WDRF);
  WDTCSR |= (1 << WDCE) | (1 << WDE);               // open the timed change window
  WDTCSR = (1 << WDIE) | (1 << WDP3) | (1 << WDP0);  // interrupt-only, ~8s
  sei();
}

static void readClock(uint8_t &hh, uint8_t &mm) {
  noInterrupts();
  uint16_t t = g_minutesOfDay;
  interrupts();
  hh = t / 60;
  mm = t % 60;
}

static void writeClock(uint8_t hh, uint8_t mm) {
  noInterrupts();
  g_minutesOfDay = (uint16_t)hh * 60 + mm;
  g_msAccum = 0;
  interrupts();
}

static void saveEEPROM(uint8_t hh, uint8_t mm) {
  EEPROM.update(EE_MAGIC_ADDR, EE_MAGIC_VALUE);
  EEPROM.update(EE_HOUR, hh);
  EEPROM.update(EE_MINUTE, mm);
}

/* Returns true the first time the device ever boots (fresh/erased EEPROM). */
static bool loadEEPROM() {
  if (EEPROM.read(EE_MAGIC_ADDR) != EE_MAGIC_VALUE) {
    writeClock(0, 0);
    saveEEPROM(0, 0);
    return true;
  }
  uint8_t hh = EEPROM.read(EE_HOUR);
  uint8_t mm = EEPROM.read(EE_MINUTE);
  if (hh > 23) hh = 0;
  if (mm > 59) mm = 0;
  writeClock(hh, mm);
  return false;
}

static void formatTime(char *buf, uint8_t hh, uint8_t mm) {
  buf[0] = '0' + hh / 10;
  buf[1] = '0' + hh % 10;
  buf[2] = ':';
  buf[3] = '0' + mm / 10;
  buf[4] = '0' + mm % 10;
  buf[5] = '\0';
}

static void showTimeGlance() {
  uint8_t hh, mm;
  readClock(hh, mm);
  saveEEPROM(hh, mm);  // cheap: EEPROM.update() only writes bytes that changed
  char buf[6];
  formatTime(buf, hh, mm);
  KC.clear();
  KC.display();
  KC.scrollText(buf, 1, true);  // blocking: one pass is enough for a glance
  KC.clear();
  KC.display();
}

static void showSetField(char label, uint8_t value) {
  char buf[4] = { label, char('0' + value / 10), char('0' + value % 10), '\0' };
  KC.scrollText(buf, 0);  // times=0 -> loops until the next call replaces it
}

/* A adjusts the flashing field (tap, or hold to auto-repeat); B confirms
   and moves on: HOUR -> MINUTE -> save & sleep. Idle too long -> save &
   sleep with whatever was set so far. */
static void runSetMode() {
  uint8_t hh, mm;
  readClock(hh, mm);

  enum class Phase : uint8_t { HOUR,
                                MINUTE } phase = Phase::HOUR;
  showSetField('H', hh);

  unsigned long lastActivity = millis();
  unsigned long aHeldSince = 0;
  unsigned long lastRepeat = 0;
  bool aWasDown = false;

  for (;;) {
    bool aDown = pressedA();

    if (aDown && !aWasDown) {
      aHeldSince = millis();
      lastRepeat = 0;  // sentinel: fire immediately below
    }
    if (aDown) {
      bool firstFire = (lastRepeat == 0);
      bool repeatDue = !firstFire && millis() - aHeldSince > SET_REPEAT_DELAY_MS
                        && millis() - lastRepeat > SET_REPEAT_MS;
      if (firstFire || repeatDue) {
        if (phase == Phase::HOUR) hh = (hh + 1) % 24;
        else mm = (mm + 1) % 60;
        showSetField(phase == Phase::HOUR ? 'H' : 'M', phase == Phase::HOUR ? hh : mm);
        lastActivity = millis();
        lastRepeat = millis();
      }
    }
    aWasDown = aDown;

    if (pressedB()) {
      while (pressedB()) {}  // wait for release, avoid double-fire
      delay(DEBOUNCE_MS);
      if (phase == Phase::HOUR) {
        phase = Phase::MINUTE;
        showSetField('M', mm);
        lastActivity = millis();
      } else {
        break;  // minute confirmed -> done
      }
    }

    if (millis() - lastActivity >= SET_IDLE_TIMEOUT_MS) break;  // abandoned: save what we have
  }

  KC.stopScrolling();
  writeClock(hh, mm);
  saveEEPROM(hh, mm);
  KC.clear();
  KC.display();
}

void setup() {
  KC.init();
  KC.clear();
  KC.display();

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);

  bool firstBoot = loadEEPROM();
  wdtSetup();  // KC.init() just disabled the WDT; re-arm it in ~8s interrupt mode

  if (firstBoot) runSetMode();  // guide a fresh device straight into setting the time
}

void loop() {
  KC.goSleep();  // returns once A+B are both down; the WDT keeps ticking the clock while asleep

  unsigned long chordStart = millis();
  bool isHold = false;
  while (pressedA() && pressedB()) {
    if (millis() - chordStart >= SET_ENTER_HOLD_MS) {
      isHold = true;
      break;
    }
  }

  if (isHold) {
    while (pressedA() || pressedB()) {}  // wait for full release before opening SET mode
    delay(DEBOUNCE_MS);
    runSetMode();
  } else {
    delay(DEBOUNCE_MS);
    showTimeGlance();
  }
}
