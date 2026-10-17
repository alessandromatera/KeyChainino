#include "save.h"

#include <EEPROM.h>

namespace save {

#define EE_MAGIC_ADDR 0  // 2 bytes
#define EE_MAGIC_0 'C'   // "CP": conta punti. Change it if the layout
#define EE_MAGIC_1 'P'   // ever changes, to force a one-time re-format.
#define EE_RING 2        // first slot

/* 128 slots: the sequence is 8 bits, and with every live sequence inside
   a window of 128 consecutive values, "newer" is a plain signed
   difference even across the 255 -> 0 wrap. Don't go above 128. */
#define SLOTS 128
#define SLOT_SIZE 4

static uint8_t head;  // slot holding the newest record
static uint8_t seq;   // its sequence number

/* Fails on a blank slot (all 0xFF) and on any single corrupted byte. */
static uint8_t check(uint8_t lo, uint8_t hi, uint8_t s) {
  return lo ^ hi ^ s ^ 0xA5;
}

static void format() {
  for (int a = EE_RING; a < EE_RING + SLOTS * SLOT_SIZE; a++) {
    EEPROM.update(a, 0xFF);
  }
  /* Magic last: a format cut short runs again on the next boot. */
  EEPROM.update(EE_MAGIC_ADDR, EE_MAGIC_0);
  EEPROM.update(EE_MAGIC_ADDR + 1, EE_MAGIC_1);
}

uint16_t load() {
  if (EEPROM.read(EE_MAGIC_ADDR) != EE_MAGIC_0 ||
      EEPROM.read(EE_MAGIC_ADDR + 1) != EE_MAGIC_1) {
    format();
  }

  /* Nothing stored yet: the first store() goes to slot 0, sequence 0. */
  head = SLOTS - 1;
  seq = 0xFF;
  uint16_t v = 0;
  bool found = false;

  for (uint8_t i = 0; i < SLOTS; i++) {
    int a = EE_RING + i * SLOT_SIZE;
    uint8_t lo = EEPROM.read(a);
    uint8_t hi = EEPROM.read(a + 1);
    uint8_t s = EEPROM.read(a + 2);
    if (EEPROM.read(a + 3) != check(lo, hi, s)) continue;  // blank or torn
    if (found && (int8_t)(s - seq) <= 0) continue;          // older
    found = true;
    head = i;
    seq = s;
    v = lo | (hi << 8);
  }
  return v;
}

void store(uint16_t v) {
  head = (head + 1) % SLOTS;  // overwrite the oldest
  seq++;
  int a = EE_RING + head * SLOT_SIZE;
  uint8_t lo = v & 0xFF;
  uint8_t hi = v >> 8;
  EEPROM.update(a, lo);
  EEPROM.update(a + 1, hi);
  EEPROM.update(a + 2, seq);
  EEPROM.update(a + 3, check(lo, hi, seq));  // last: commits the record
}

}  // namespace save
