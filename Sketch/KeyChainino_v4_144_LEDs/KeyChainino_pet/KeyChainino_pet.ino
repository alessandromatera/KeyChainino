/*************************************************************************
  VIRTUAL PET FOR KEYCHAININO 144
  https://www.keychainino.com/product/keychainino-144

  A standalone virtual pet, independent of the multi-game firmware:
    press A+B together -> wake, and while awake:
                            tap A = give medicine if sick, else clean up
                                    poop if any is waiting, else feed
                            tap B = answer a discipline call if one is
                                    pending, else play
                            press A+B together AGAIN, any time while awake
                                  = status: [sick], age, hunger, happiness,
                                    poop, discipline -- then the session
                                    just resumes
                            (idle a few seconds -> save & sleep)
    (pet has died)
    hold A+B ~1.2s     -> hatch a brand new egg

  Checking status isn't tied to how long you hold the wake gesture -- every
  wake goes straight into the normal session, and pressing both buttons
  together at any point while it's open shows status, since that's more
  discoverable than needing to guess a hold duration exactly at wake-up.

  The pet ages, gets hungry/bored, poops, and calls for discipline in real
  time, even while the device is asleep, using the same "WDT as a timer"
  trick as KeyChainino_watch: the only oscillator alive during
  SLEEP_MODE_PWR_DOWN is the ~128kHz watchdog RC, so a watchdog interrupt
  every ~8s accumulates elapsed minutes. Stats are only recomputed (and
  saved to EEPROM) when you actually glance at the pet, not continuously --
  keeps EEPROM wear low and matches how classic virtual pets "catch you up"
  only when you look.

  Poop piles up faster while young (like the original hardware) and, left
  uncleaned, drags happiness down -- and if the pile sits completely full
  for too long, the pet falls ill (same as the real toy). Illness shows a
  medical-cross icon during the awake idle-cycle; give medicine (tap A)
  before it does more damage. A discipline call shows an alert icon the
  same way; answer it (tap B) within 15 real minutes or it's logged as a
  care mistake. Reaching adulthood well-disciplined (and without too many
  missed calls) gets the neatly-groomed adult form; otherwise it's the
  scruffier one -- same as how the real toy's discipline meter shapes which
  adult you end up with.

  Neglect it for too long -- hunger stuck at 0, or illness left untreated,
  across several glances -- and it dies; hold A+B while it's a ghost to
  hatch a new egg.

  The screen also wakes itself, unprompted, when something newly needs
  attention -- a brief single-icon blip, not a full glance -- piggybacking
  on the watchdog tick that's already firing every ~8s to age the pet, so
  there's no new wake source or extra sleep-current cost. Only the flash
  itself costs anything, and it's kept short (~200ms) and rare (once when a
  need appears, then at most every 15 real minutes while still ignored) so
  an unaddressed need can't quietly drain the battery over hours.

  Like the watch, this board has no RTC -- a real power loss (battery
  pull) resets the age clock to whatever was last saved to EEPROM, same
  tradeoff the watch already accepts for timekeeping.

  created by Alessandro Matera
* ************************************************************************/

#include "KeyChainino.h"
#include "pet_sprites.h"
#include <EEPROM.h>

/* Declared up front (rather than where it's first used) because the
   Arduino builder auto-generates function prototypes and inserts them
   above the first function in the file -- if this type were declared
   later, those auto-prototypes would reference it before it exists. */
enum class Stage : uint8_t { EGG,
                              BABY,
                              CHILD,
                              TEEN,
                              ADULT };

/* What, if anything, the background alert should flash for -- ordered by
   urgency (illness worst, hunger least, matching drawIdleFrame()'s
   priority for the same icons while awake). */
enum class Attention : uint8_t { NONE,
                                  SICK,
                                  POOP,
                                  CALL,
                                  HUNGRY };

/* --- Life-stage pacing (real minutes of age since hatching; tune to taste) --- */
#define STAGE_EGG_MIN    5UL     // hatches after 5 min
#define STAGE_BABY_MIN   60UL    // baby -> child after 1h
#define STAGE_CHILD_MIN  360UL   // child -> teen after 6h
#define STAGE_TEEN_MIN   1440UL  // teen -> adult after 24h

/* --- Care tuning --- */
#define HUNGER_DECAY_PER_HOUR  8    // hunger points lost per real hour
#define HAPPY_DECAY_PER_HOUR   6    // happiness points lost per real hour
#define FEED_HUNGER_GAIN      35
#define PLAY_HAPPY_GAIN        30
#define PLAY_HUNGER_COST         8
#define NEGLECT_LIMIT            6  // consecutive starving glances -> death

/* --- Poop tuning (classic virtual pets poop more often when young) --- */
#define POOP_INTERVAL_YOUNG_MIN  45UL   // egg/baby/child
#define POOP_INTERVAL_OLD_MIN   180UL   // teen/adult, ~3h like the real thing
#define POOP_MAX                  4
#define POOP_HAPPY_PENALTY_PER_HOUR 4   // extra happiness decay per uncleaned poop

/* --- Discipline tuning: the pet occasionally "calls" for training; miss the
   response window and it's logged as a care mistake (same 15-min window the
   original hardware uses). Discipline + mistakes decide which adult you get. */
#define CALL_INTERVAL_MIN        90UL
#define CALL_TIMEOUT_MIN         15UL
#define DISCIPLINE_GAIN           15
#define CARE_MISTAKE_HAPPY_PENALTY 10
#define ADULT_GOOD_DISCIPLINE_MIN  50
#define ADULT_GOOD_MISTAKES_MAX     4

/* --- Illness tuning: wallowing in max poop for too long makes the pet
   sick (matches the original hardware); untreated illness stacks onto the
   same neglect counter that leads to death, same as ignored starvation. */
#define SICK_ONSET_MIN            90UL  // real minutes stuck at POOP_MAX before falling ill
#define SICK_TIMEOUT_MIN          30UL  // real minutes untreated illness -> another neglect strike
#define SICK_HAPPY_PENALTY_PER_HOUR 6   // extra happiness decay while sick

/* --- Background attention alert: briefly lights up on its own (no button
   press needed) when the pet newly needs something, so you don't have to
   keep checking in blind. Piggybacks on the watchdog tick that's already
   waking the MCU every ~8s to age the pet -- see goSleep()'s onIdleWake
   hook -- so there's no new wake source or extra sleep-current cost; the
   only added cost is the brief flash itself, kept short and infrequent. */
#define ALERT_BLINK_MS            200UL  // how long the attention blip stays lit
#define ALERT_REPEAT_MIN           15UL  // real minutes between reminders while still unaddressed

/* --- Timing --- */
#define INTERACT_IDLE_TIMEOUT_MS 8000UL
#define HOLD_ENTER_MS             1200UL  // still used for the dead-pet "hold to hatch" gesture
#define CHORD_WINDOW_MS            150UL  // how long a single button-down waits for the other
                                           // to join before committing to a single-button action
#define ANIM_FRAME_MS              200UL  // hold time per feed/play animation frame
#define IDLE_BOB_MS                700UL  // idle "breathing" bob period while awake
#define DEBOUNCE_MS                 20UL

/* --- EEPROM layout --- */
#define EE_MAGIC_ADDR      0
#define EE_MAGIC_VALUE  0x56  // bump if this layout ever changes
#define EE_AGE_0           1  // uint32_t, 4 bytes: age (minutes) as of last save
#define EE_HUNGER          5
#define EE_HAPPY           6
#define EE_NEGLECT         7
#define EE_ALIVE           8
#define EE_STAGE           9
#define EE_POOP           10
#define EE_DISCIPLINE     11
#define EE_MISTAKES       12
#define EE_CALL_PENDING   13
#define EE_CALL_START_0   14  // uint32_t, 4 bytes: age when the pending call began
#define EE_CALL_COOL_0    18  // uint32_t, 4 bytes: age from which the next call is timed
#define EE_POOP_CHECK_0   22  // uint32_t, 4 bytes: age at which poop spawn was last evaluated
#define EE_SICK           26
#define EE_SICK_START_0   27  // uint32_t, 4 bytes: age when the illness began
#define EE_FILTH_ONSET_0  31  // uint32_t, 4 bytes: age since which poop has sat at POOP_MAX

/* --- WDT-as-timer: same trick as KeyChainino_watch. Real period is
   8.192s (2^20 watchdog-oscillator cycles / 128kHz), not a flat 8s. --- */
static const uint16_t WDT_TICK_MS = 8192;
volatile uint32_t g_ageMinutes = 0;   // grows forever from "hatch"
volatile uint32_t g_msAccum = 0;      // sub-minute remainder; ISR-only
volatile uint32_t g_wdtTickCount = 0; // increments on every real tick (~8.192s);
                                       // used by checkAndMaybeAlert() below to gate
                                       // its real work at tick granularity -- g_ageMinutes
                                       // only changes once a minute (up to ~7 ticks later),
                                       // which is too coarse for "fires within one tick"

ISR(WDT_vect) {
  WDTCSR |= (1 << WDIE);  // re-arm interrupt-only mode: hardware clears WDIE
                           // on entry, and a missed re-arm just stops the
                           // clock (WDE is never set, so no reset risk)
  g_msAccum += WDT_TICK_MS;
  while (g_msAccum >= 60000UL) {
    g_msAccum -= 60000UL;
    g_ageMinutes++;
  }
  g_wdtTickCount++;
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

static uint32_t readAge() {
  noInterrupts();
  uint32_t a = g_ageMinutes;
  interrupts();
  return a;
}

static void writeAge(uint32_t a) {
  noInterrupts();
  g_ageMinutes = a;
  g_msAccum = 0;
  interrupts();
}

/* Pet state lives in SRAM, which survives power-down sleep; only lost if
   the battery is pulled (restored from EEPROM on the next boot). */
uint8_t g_hunger = 100;
uint8_t g_happy = 100;
uint8_t g_neglect = 0;
bool g_alive = true;
uint8_t g_poopCount = 0;
uint8_t g_discipline = 0;
uint8_t g_careMistakes = 0;
bool g_callPending = false;
uint32_t g_callStartAge = 0;     // valid iff g_callPending
uint32_t g_callCooldownAge = 0;  // age from which the next call is timed
uint32_t g_lastPoopCheckAge = 0;
bool g_sick = false;
uint32_t g_sickStartAge = 0;     // valid iff g_sick
uint32_t g_filthOnsetAge = 0;    // age since which poop has sat at POOP_MAX

/* Background-alert pacing. Deliberately RAM-only, never written to EEPROM:
   losing this on a battery pull just means the first alert after power-up
   fires immediately instead of respecting the old back-off, which is
   harmless -- and it keeps this feature from adding any new EEPROM wear. */
bool g_wasNeedingAttention = false;
uint32_t g_lastAlertFlashAge = 0;

static uint32_t readEEUint32(int addr) {
  uint32_t v = 0;
  for (uint8_t i = 0; i < 4; i++) v |= (uint32_t)EEPROM.read(addr + i) << (8 * i);
  return v;
}

static void writeEEUint32(int addr, uint32_t v) {
  for (uint8_t i = 0; i < 4; i++) EEPROM.update(addr + i, (byte)(v >> (8 * i)));
}

static Stage currentStage(uint32_t ageMin) {
  if (ageMin < STAGE_EGG_MIN) return Stage::EGG;
  if (ageMin < STAGE_BABY_MIN) return Stage::BABY;
  if (ageMin < STAGE_CHILD_MIN) return Stage::CHILD;
  if (ageMin < STAGE_TEEN_MIN) return Stage::TEEN;
  return Stage::ADULT;
}

static const byte *stageSprite(Stage s) {
  switch (s) {
    case Stage::EGG: return PET_EGG;
    case Stage::BABY: return PET_BABY;
    case Stage::CHILD: return PET_CHILD;
    case Stage::TEEN: return PET_TEEN;
    default:
      // Which adult you get depends on how it was raised, same as the real thing.
      bool wellRaised = g_discipline >= ADULT_GOOD_DISCIPLINE_MIN && g_careMistakes <= ADULT_GOOD_MISTAKES_MAX;
      return wellRaised ? PET_ADULT : PET_ADULT_BAD;
  }
}

static uint8_t clampSub(uint8_t v, uint32_t amount) {
  return (amount >= v) ? 0 : (uint8_t)(v - amount);
}

static uint8_t clampAdd(uint8_t v, int amount) {
  int r = (int)v + amount;
  if (r < 0) return 0;
  if (r > 100) return 100;
  return (uint8_t)r;
}

static void saveState() {
  writeEEUint32(EE_AGE_0, readAge());
  EEPROM.update(EE_HUNGER, g_hunger);
  EEPROM.update(EE_HAPPY, g_happy);
  EEPROM.update(EE_NEGLECT, g_neglect);
  EEPROM.update(EE_ALIVE, g_alive ? 1 : 0);
  EEPROM.update(EE_STAGE, (byte)currentStage(readAge()));
  EEPROM.update(EE_POOP, g_poopCount);
  EEPROM.update(EE_DISCIPLINE, g_discipline);
  EEPROM.update(EE_MISTAKES, g_careMistakes);
  EEPROM.update(EE_CALL_PENDING, g_callPending ? 1 : 0);
  writeEEUint32(EE_CALL_START_0, g_callStartAge);
  writeEEUint32(EE_CALL_COOL_0, g_callCooldownAge);
  writeEEUint32(EE_POOP_CHECK_0, g_lastPoopCheckAge);
  EEPROM.update(EE_SICK, g_sick ? 1 : 0);
  writeEEUint32(EE_SICK_START_0, g_sickStartAge);
  writeEEUint32(EE_FILTH_ONSET_0, g_filthOnsetAge);
  EEPROM.update(EE_MAGIC_ADDR, EE_MAGIC_VALUE);
}

static void hatchNewPet() {
  writeAge(0);
  g_hunger = 100;
  g_happy = 100;
  g_neglect = 0;
  g_alive = true;
  g_poopCount = 0;
  g_discipline = 0;
  g_careMistakes = 0;
  g_callPending = false;
  g_callStartAge = 0;
  g_callCooldownAge = 0;
  g_lastPoopCheckAge = 0;
  g_sick = false;
  g_sickStartAge = 0;
  g_filthOnsetAge = 0;
  saveState();
}

/* Returns true the first time the device ever boots (fresh/erased EEPROM). */
static bool loadState() {
  if (EEPROM.read(EE_MAGIC_ADDR) != EE_MAGIC_VALUE) {
    hatchNewPet();
    return true;
  }
  writeAge(readEEUint32(EE_AGE_0));
  g_hunger = EEPROM.read(EE_HUNGER);
  g_happy = EEPROM.read(EE_HAPPY);
  g_neglect = EEPROM.read(EE_NEGLECT);
  g_alive = EEPROM.read(EE_ALIVE) != 0;
  if (g_hunger > 100) g_hunger = 100;
  if (g_happy > 100) g_happy = 100;
  g_poopCount = EEPROM.read(EE_POOP);
  if (g_poopCount > POOP_MAX) g_poopCount = POOP_MAX;
  g_discipline = EEPROM.read(EE_DISCIPLINE);
  if (g_discipline > 100) g_discipline = 100;
  g_careMistakes = EEPROM.read(EE_MISTAKES);
  g_callPending = EEPROM.read(EE_CALL_PENDING) != 0;
  g_callStartAge = readEEUint32(EE_CALL_START_0);
  g_callCooldownAge = readEEUint32(EE_CALL_COOL_0);
  g_lastPoopCheckAge = readEEUint32(EE_POOP_CHECK_0);
  g_sick = EEPROM.read(EE_SICK) != 0;
  g_sickStartAge = readEEUint32(EE_SICK_START_0);
  g_filthOnsetAge = readEEUint32(EE_FILTH_ONSET_0);
  return false;
}

static void scrollMsg(const char *msgP) {
  char buf[16];
  strncpy_P(buf, msgP, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';
  KC.scrollText(buf, 1, true);
}

/* Shifts a 12x12 RAM frame buffer vertically by whole rows: positive moves
   content down, negative moves it up. Rows shifted past an edge are lost
   (clipped, not wrapped) -- on this small a display that clipping *is* the
   animation: a "jump" pokes the head off the top edge, a "lean down" empties
   the top row as the pet dips toward the bottom. Done in-place, processing
   rows in the direction that never overwrites a row before it's been read. */
static void shiftBufferVertical(byte *buf, int8_t rows) {
  if (rows > 0) {
    for (int8_t r = 11; r >= 0; r--) {
      int8_t src = r - rows;
      if (src >= 0) memcpy(&buf[r * 12], &buf[src * 12], 12);
      else memset(&buf[r * 12], 0, 12);
    }
  } else if (rows < 0) {
    for (int8_t r = 0; r < 12; r++) {
      int8_t src = r - rows;
      if (src < 12) memcpy(&buf[r * 12], &buf[src * 12], 12);
      else memset(&buf[r * 12], 0, 12);
    }
  }
}

/* Draws one animation frame of the pet: rowOffset shifts it via
   shiftBufferVertical() (0 = resting position) so feed/play/idle can all
   share the same handful of stage bitmaps instead of needing dedicated
   per-stage animation art. */
static void drawPetFrame(Stage stage, int8_t rowOffset) {
  byte buf[144];
  memcpy_P(buf, stageSprite(stage), 144);
  if (rowOffset != 0) shiftBufferVertical(buf, rowOffset);
  KC.drawBitmap(buf);
  KC.display();
}

static void drawPet(Stage stage) {
  drawPetFrame(stage, 0);
}

static void drawIconFrame(const byte *icon) {
  KC.drawBitmapP(icon);
  KC.display();
}

/* Like drawPetFrame(), but ALSO composites a small corner icon into the
   very same frame (OR'd together, not a separate full-screen cutaway) --
   so feeding/playing reads as "the pet and the food/heart in one picture,
   doing something" instead of a slideshow alternating between the pet and
   an unrelated icon. */
static void drawPetWithIcon(Stage stage, int8_t rowOffset, const byte *icon) {
  byte buf[144];
  memcpy_P(buf, stageSprite(stage), 144);
  if (rowOffset != 0) shiftBufferVertical(buf, rowOffset);
  byte iconBuf[144];
  memcpy_P(iconBuf, icon, 144);
  for (uint8_t i = 0; i < 144; i++) buf[i] |= iconBuf[i];
  KC.drawBitmap(buf);
  KC.display();
}

/* Pet leans in and visibly bites the food (a nodding motion via the row
   shift), which shrinks with each bite, always shown together with the
   pet -- then one happy hop once it's gone. */
static void playFeedAnimation(Stage stage) {
  drawPetWithIcon(stage, 1, ICON_FOOD_CORNER);        // lean in, bite 1
  delay(ANIM_FRAME_MS);
  drawPetWithIcon(stage, 0, ICON_FOOD_CORNER_SMALL);  // back up, chewing -- food shrinks
  delay(ANIM_FRAME_MS);
  drawPetWithIcon(stage, 1, ICON_FOOD_CORNER_SMALL);  // lean in, bite 2
  delay(ANIM_FRAME_MS);
  drawPetFrame(stage, 0);                             // food gone
  delay(ANIM_FRAME_MS);
  drawPetFrame(stage, -1);                            // happy hop
  delay(ANIM_FRAME_MS);
  drawPetFrame(stage, 0);
}

/* Pet visibly bounces (the same row-shift motion feed uses to lean, here
   reading as a jump) with a heart shown right alongside it in every frame,
   not cut away to on its own. */
static void playPlayAnimation(Stage stage) {
  drawPetWithIcon(stage, -1, ICON_HEART_CORNER);  // jump, heart appears
  delay(ANIM_FRAME_MS);
  drawPetWithIcon(stage, 0, ICON_HEART_CORNER);   // land
  delay(ANIM_FRAME_MS);
  drawPetWithIcon(stage, -1, ICON_HEART_CORNER);  // jump again
  delay(ANIM_FRAME_MS);
  drawPetWithIcon(stage, 0, ICON_HEART_CORNER);   // land
  delay(ANIM_FRAME_MS);
  drawPetFrame(stage, -1);                        // one more happy hop
  delay(ANIM_FRAME_MS);
  drawPetFrame(stage, 0);
}

/* Pet notices the mess, leans down to clean it up, then hops in relief. */
static void playCleanAnimation(Stage stage) {
  drawIconFrame(ICON_POOP);
  delay(ANIM_FRAME_MS);
  drawPetFrame(stage, 1);
  delay(ANIM_FRAME_MS);
  drawPetFrame(stage, 1);
  delay(ANIM_FRAME_MS);
  drawPetFrame(stage, -1);
  delay(ANIM_FRAME_MS);
  drawPetFrame(stage, 0);
}

/* Alert, pet listens (head down), then a happy hop for responding in time. */
static void playScoldAnimation(Stage stage) {
  drawIconFrame(ICON_ALERT);
  delay(ANIM_FRAME_MS);
  drawPetFrame(stage, 1);
  delay(ANIM_FRAME_MS);
  drawIconFrame(ICON_HEART);
  delay(ANIM_FRAME_MS);
  drawPetFrame(stage, 0);
  delay(ANIM_FRAME_MS);
  drawPetFrame(stage, -1);
  delay(ANIM_FRAME_MS);
  drawPetFrame(stage, 0);
}

/* Pet droops while the medicine takes effect, then perks back up cured. */
static void playMedicineAnimation(Stage stage) {
  drawIconFrame(ICON_SICK);
  delay(ANIM_FRAME_MS);
  drawPetFrame(stage, 1);
  delay(ANIM_FRAME_MS);
  drawPetFrame(stage, 1);
  delay(ANIM_FRAME_MS);
  drawPetFrame(stage, -1);
  delay(ANIM_FRAME_MS);
  drawPetFrame(stage, 0);
}

/* Applies real-time decay since the last save and updates the neglect
   streak, possibly flipping g_alive false. Caller is responsible for
   persisting the result (saveState()) right after calling this, so the
   age marker never gets re-used and the same elapsed window double-counted. */
static void applyDecayAndCheckDeath() {
  uint32_t ageNow = readAge();
  uint32_t ageAtSave = readEEUint32(EE_AGE_0);
  uint32_t delta = (ageNow >= ageAtSave) ? (ageNow - ageAtSave) : 0;

  uint32_t hungerLoss = (delta * HUNGER_DECAY_PER_HOUR) / 60UL;
  uint32_t happyLoss = (delta * HAPPY_DECAY_PER_HOUR) / 60UL;
  g_hunger = clampSub(g_hunger, hungerLoss);
  g_happy = clampSub(g_happy, happyLoss);

  if (g_sick) {
    uint32_t sickLoss = (delta * SICK_HAPPY_PENALTY_PER_HOUR) / 60UL;
    g_happy = clampSub(g_happy, sickLoss);
  }

  if (g_hunger == 0) {
    if (g_neglect < 255) g_neglect++;
  } else {
    g_neglect = 0;
  }

  if (g_neglect >= NEGLECT_LIMIT) g_alive = false;
}

/* Spawns poop over time (faster while young, like the real thing) and runs
   the discipline-call state machine: a call spawns periodically, must be
   answered (scolded, see runInteractSession's tap-B handling) within
   CALL_TIMEOUT_MIN real minutes or it's logged as a care mistake. Uncleaned
   poop also eats into happiness, proportional to how much has piled up, and
   -- if the pile sits at POOP_MAX for too long -- makes the pet sick (see
   below). Call only while g_alive -- a dead pet doesn't accrue either. */
static void updatePoopAndCalls(uint32_t ageNow) {
  uint32_t poopInterval = (ageNow < STAGE_CHILD_MIN) ? POOP_INTERVAL_YOUNG_MIN : POOP_INTERVAL_OLD_MIN;
  uint32_t sincePoopCheck = (ageNow >= g_lastPoopCheckAge) ? (ageNow - g_lastPoopCheckAge) : 0;
  uint32_t newPoops = sincePoopCheck / poopInterval;
  if (newPoops > 0) {
    uint32_t total = (uint32_t)g_poopCount + newPoops;
    g_poopCount = (total > POOP_MAX) ? POOP_MAX : (uint8_t)total;
    g_lastPoopCheckAge += newPoops * poopInterval;  // keep the remainder, don't reset to ageNow
  }
  if (g_poopCount > 0) {
    uint32_t poopPenalty = (sincePoopCheck * POOP_HAPPY_PENALTY_PER_HOUR * g_poopCount) / 60UL;
    g_happy = clampSub(g_happy, poopPenalty);
  }

  if (g_poopCount < POOP_MAX) {
    g_filthOnsetAge = ageNow;  // pile isn't full -- keep resetting the exposure baseline
  } else if (!g_sick) {
    uint32_t exposure = (ageNow >= g_filthOnsetAge) ? (ageNow - g_filthOnsetAge) : 0;
    if (exposure >= SICK_ONSET_MIN) {
      g_sick = true;
      g_sickStartAge = ageNow;
    }
  }

  if (g_sick) {
    uint32_t sinceSick = (ageNow >= g_sickStartAge) ? (ageNow - g_sickStartAge) : 0;
    if (sinceSick > SICK_TIMEOUT_MIN) {
      if (g_neglect < 255) g_neglect++;
      g_sickStartAge = ageNow;  // restart the window -- untreated illness keeps stacking strikes
      if (g_neglect >= NEGLECT_LIMIT) g_alive = false;
    }
  }

  if (g_callPending) {
    uint32_t sinceCall = (ageNow >= g_callStartAge) ? (ageNow - g_callStartAge) : 0;
    if (sinceCall > CALL_TIMEOUT_MIN) {
      if (g_careMistakes < 255) g_careMistakes++;
      g_happy = clampSub(g_happy, CARE_MISTAKE_HAPPY_PENALTY);
      g_callPending = false;
      g_callCooldownAge = ageNow;
    }
  } else {
    uint32_t sinceCooldown = (ageNow >= g_callCooldownAge) ? (ageNow - g_callCooldownAge) : 0;
    if (sinceCooldown >= CALL_INTERVAL_MIN) {
      g_callPending = true;
      g_callStartAge = ageNow;
    }
  }
}

/* Read-only projection of "would a real glance right now find something
   worth attending to" -- reuses the exact thresholds updatePoopAndCalls()/
   applyDecayAndCheckDeath() act on, but never mutates or saves state, so
   it's safe to call from the background wake hook on every WDT tick. Deaf
   to a dead pet (nothing to attend to but a fresh egg, which needs a
   deliberate hold gesture anyway). */
static Attention checkAttentionNeeded(uint32_t ageNow) {
  if (!g_alive) return Attention::NONE;
  if (g_sick) return Attention::SICK;
  if (g_poopCount > 0) return Attention::POOP;
  if (g_callPending) return Attention::CALL;

  uint32_t poopInterval = (ageNow < STAGE_CHILD_MIN) ? POOP_INTERVAL_YOUNG_MIN : POOP_INTERVAL_OLD_MIN;
  if (ageNow >= g_lastPoopCheckAge + poopInterval) return Attention::POOP;

  if (ageNow >= g_callCooldownAge + CALL_INTERVAL_MIN) return Attention::CALL;

  uint32_t ageAtSave = readEEUint32(EE_AGE_0);
  uint32_t delta = (ageNow >= ageAtSave) ? (ageNow - ageAtSave) : 0;
  uint32_t projectedHungerLoss = (delta * HUNGER_DECAY_PER_HOUR) / 60UL;
  if (projectedHungerLoss >= g_hunger) return Attention::HUNGRY;

  return Attention::NONE;
}

static const byte *attentionIcon(Attention kind) {
  switch (kind) {
    case Attention::SICK: return ICON_SICK;
    case Attention::POOP: return ICON_POOP;
    case Attention::CALL: return ICON_ALERT;
    default: return ICON_FOOD;  // HUNGRY
  }
}

/* Briefly re-powers just enough of the board to show one icon, then powers
   back down -- Timer0 (for delay()) and Timer2 (for the display scan) are
   normally off for the whole sleep duration; goSleep() itself disables
   them, so this hook is responsible for symmetric enable/disable around the
   flash. Deliberately NOT a full gameShowFace-style hold: short and sparse
   is the entire point of keeping this battery-cheap. */
static void flashAttentionBlip(Attention kind) {
  power_timer0_enable();
  power_timer2_enable();
  KC.drawBitmapP(attentionIcon(kind));
  KC.display();
  delay(ALERT_BLINK_MS);
  KC.prepSleepIdle();  // blank display, tri-state pins, gate timers back down --
                        // goSleep() does NOT do this for us (see its doc comment);
                        // only reached on a real tick with something due, never
                        // from the level-triggered held-button re-entry storm.
}

/* The goSleep() onIdleWake hook: fires on every real WDT tick (roughly
   every ~8s) while asleep, but ALSO on every spurious re-entry caused by
   holding a single button (INT0/INT1 are level-triggered, so a held button
   re-fires continuously -- thousands of times for one brief press, not
   just once). Gating on "has g_wdtTickCount actually advanced since the
   last time real work ran" -- a plain monotonic counter, only ever written
   by the WDT ISR, compared with one atomic read here -- means every one of
   those spurious re-entries is a fast no-op without needing a separate
   ISR-set/mainline-cleared boolean flag (an earlier version used exactly
   such a flag and it was timing-sensitive enough to occasionally corrupt
   unrelated state during a button-hold storm). Ticks, not elapsed minutes:
   an earlier version of this gate compared g_ageMinutes instead, which
   only changes once every ~7 ticks and made the alert take up to a minute
   to notice a newly-due need instead of firing on the very next tick. */
uint32_t g_lastSeenTickCount = 0;
bool g_haveCheckedAttention = false;

static void checkAndMaybeAlert() {
  noInterrupts();
  uint32_t tickCount = g_wdtTickCount;
  interrupts();
  if (g_haveCheckedAttention && tickCount == g_lastSeenTickCount) return;
  g_haveCheckedAttention = true;
  g_lastSeenTickCount = tickCount;

  uint32_t ageNow = readAge();
  Attention kind = checkAttentionNeeded(ageNow);
  bool needsNow = (kind != Attention::NONE);

  bool justAppeared = needsNow && !g_wasNeedingAttention;
  bool dueForReminder = needsNow && (ageNow - g_lastAlertFlashAge >= ALERT_REPEAT_MIN);
  g_wasNeedingAttention = needsNow;

  if (justAppeared || dueForReminder) {
    flashAttentionBlip(kind);
    g_lastAlertFlashAge = ageNow;
  }
}

static void showDeathAndWaitReset() {
  KC.clear();
  KC.display();
  KC.drawBitmapP(PET_GHOST);
  KC.display();
  delay(700);
  scrollMsg(PSTR("BYE"));
  KC.clear();
  KC.display();
}

/* Tap A = feed, tap B = play; idle timeout -> save & sleep. Between taps the
   pet gently bobs in place (a live-looking creature, not a static icon) via
   the same row-shift trick the feed/play animations use. */
/* Idle-cycle frame: alternates the resting/bobbed pet with a reminder icon
   (poop, or a pending discipline call) so a need is visible even if you
   never press a button this glance -- phase 2 shows the reminder only when
   there's something to show, otherwise it's just another resting frame. */
static void drawIdleFrame(Stage stage, uint8_t phase) {
  if (phase == 2 && g_sick) {
    drawIconFrame(ICON_SICK);
    return;
  }
  if (phase == 2 && g_poopCount > 0) {
    drawIconFrame(ICON_POOP);
    return;
  }
  if (phase == 2 && g_callPending) {
    drawIconFrame(ICON_ALERT);
    return;
  }
  drawPetFrame(stage, (phase == 1) ? 1 : 0);
}

/* Tap A = give medicine if sick, else clean poop if any is pending, else
   feed. Tap B = scold if a discipline call is pending, else play. Press
   A+B together at ANY point while awake (not just right at wake-up) -> a
   status check, then the session simply resumes -- checking on the pet
   shouldn't require guessing a hold duration at the exact moment you wake
   it. Idle timeout -> save & sleep.

   A single button-down doesn't act immediately: it's held "pending" for
   CHORD_WINDOW_MS, checked once per outer-loop pass (no nested busy-loop
   or delay() of its own), so a slightly staggered two-button press still
   resolves as a status chord instead of misfiring as a single action. This
   flat, single-pass-per-iteration shape mirrors the rest of the loop (the
   idle-bob timer below works the same way) rather than nesting a nested
   wait inside the edge handler -- an earlier draft used exactly such a
   nested millis()-polling (and later a nested delay()-stepped) wait here,
   and it turned out to be a fragile, timing-sensitive spot: it worked in
   isolated manual testing but intermittently dropped a second action
   within one session in the fuller regression suite, and adding debug
   instrumentation nearby was enough to hide the symptom -- a classic sign
   of a real race, not a logic bug worth chasing further at the level of
   this single-threaded polling loop. Flattening the wait into ordinary
   per-iteration state avoids whatever that was. */
static void runInteractSession(Stage stage) {
  uint8_t idlePhase = 0;
  drawPetFrame(stage, 0);

  unsigned long lastActivity = millis();
  unsigned long lastBob = millis();
  bool aWasDown = false, bWasDown = false;
  bool aPending = false, bPending = false;
  unsigned long aPendingSince = 0, bPendingSince = 0;

  while (millis() - lastActivity < INTERACT_IDLE_TIMEOUT_MS) {
    bool aDown = pressedA();
    bool bDown = pressedB();

    if (aDown && !aWasDown) {
      aPending = true;
      aPendingSince = millis();
    }
    if (bDown && !bWasDown) {
      bPending = true;
      bPendingSince = millis();
    }

    if (aPending && bPending) {
      while (pressedA() || pressedB()) {}  // wait for full release
      showStatus(stage);
      drawPetFrame(stage, 0);  // session resumes, pet visible again
      aPending = false;
      bPending = false;
      lastActivity = millis();
      lastBob = millis();
      idlePhase = 0;
    } else if (aPending && millis() - aPendingSince >= CHORD_WINDOW_MS) {
      // B never joined within the window -- commit to the single-button action
      if (g_sick) {
        g_sick = false;
        g_neglect = 0;
        playMedicineAnimation(stage);
      } else if (g_poopCount > 0) {
        g_poopCount--;
        playCleanAnimation(stage);
      } else {
        g_hunger = clampAdd(g_hunger, FEED_HUNGER_GAIN);
        g_neglect = 0;
        playFeedAnimation(stage);
      }
      aPending = false;
      lastActivity = millis();
      lastBob = millis();
      idlePhase = 0;
    } else if (bPending && millis() - bPendingSince >= CHORD_WINDOW_MS) {
      if (g_callPending) {
        g_discipline = clampAdd(g_discipline, DISCIPLINE_GAIN);
        g_callPending = false;
        g_callCooldownAge = readAge();
        playScoldAnimation(stage);
      } else {
        g_happy = clampAdd(g_happy, PLAY_HAPPY_GAIN);
        g_hunger = clampSub(g_hunger, PLAY_HUNGER_COST);
        playPlayAnimation(stage);
      }
      bPending = false;
      lastActivity = millis();
      lastBob = millis();
      idlePhase = 0;
    }

    aWasDown = aDown;
    bWasDown = bDown;

    if (millis() - lastBob >= IDLE_BOB_MS) {
      idlePhase = (idlePhase + 1) % 3;
      drawIdleFrame(stage, idlePhase);
      lastBob = millis();
    }
  }

  saveState();
  KC.clear();
  KC.display();
}

static void showStatus(Stage stage) {
  drawPetFrame(stage, 0);
  delay(300);
  drawPetFrame(stage, 1);  // one gentle bob so the pet doesn't look frozen
  delay(300);
  drawPetFrame(stage, 0);
  delay(300);

  char buf[16];
  char num[6];
  uint32_t hrs = readAge() / 60;

  if (g_sick) scrollMsg(PSTR("SICK!"));  // most urgent, shown first

  strcpy_P(buf, PSTR("AGE "));
  if (hrs < 100) {
    utoa((unsigned)hrs, num, 10);
    strcat(buf, num);
    strcat(buf, "H");
  } else {
    utoa((unsigned)(hrs / 24), num, 10);
    strcat(buf, num);
    strcat(buf, "D");
  }
  KC.scrollText(buf, 1, true);

  strcpy_P(buf, PSTR("FOOD "));
  utoa(g_hunger, num, 10);
  strcat(buf, num);
  KC.scrollText(buf, 1, true);

  strcpy_P(buf, PSTR("FUN "));
  utoa(g_happy, num, 10);
  strcat(buf, num);
  KC.scrollText(buf, 1, true);

  strcpy_P(buf, PSTR("POOP "));
  utoa(g_poopCount, num, 10);
  strcat(buf, num);
  KC.scrollText(buf, 1, true);

  strcpy_P(buf, PSTR("DISC "));
  utoa(g_discipline, num, 10);
  strcat(buf, num);
  KC.scrollText(buf, 1, true);

  KC.clear();
  KC.display();
}

void setup() {
  KC.init();
  KC.clear();
  KC.display();

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);

  bool firstBoot = loadState();
  wdtSetup();  // KC.init() just disabled the WDT; re-arm it in ~8s interrupt mode

  if (firstBoot) {
    scrollMsg(PSTR("NEW PET!"));
    drawPet(Stage::EGG);
    delay(700);
    KC.clear();
    KC.display();
  }
}

void loop() {
  KC.goSleep(checkAndMaybeAlert);  // returns once a button wakes it; the WDT keeps
                                    // ticking age and, via the callback, checking in

  /* Only still needed for the dead-pet "hold to hatch" gesture below --
     while alive, every wake goes straight into the interact session, and
     checking on the pet (A+B together) is handled continuously inside it
     (see runInteractSession()) rather than by timing the wake press itself. */
  unsigned long chordStart = millis();
  bool isHold = false;
  while (pressedA() && pressedB()) {
    if (millis() - chordStart >= HOLD_ENTER_MS) {
      isHold = true;
      break;
    }
  }
  while (pressedA() || pressedB()) {}  // wait for full release before any UI
  delay(DEBOUNCE_MS);

  if (!g_alive) {
    if (isHold) {
      hatchNewPet();
      scrollMsg(PSTR("NEW EGG!"));
      drawPet(Stage::EGG);
      delay(700);
      KC.clear();
      KC.display();
    }
    return;  // tap while dead: stays a ghost until you hold to reset
  }

  uint8_t stageBefore = EEPROM.read(EE_STAGE);
  applyDecayAndCheckDeath();
  if (g_alive) updatePoopAndCalls(readAge());
  saveState();  // commits the new age marker now, so a later glance never
                // re-applies the same elapsed window twice

  if (!g_alive) {
    showDeathAndWaitReset();
    return;
  }

  Stage stage = currentStage(readAge());
  if ((uint8_t)stage != stageBefore) scrollMsg(PSTR("GROW UP!"));
  runInteractSession(stage);  // saves again at the end (feed/play/status + final age)
}
