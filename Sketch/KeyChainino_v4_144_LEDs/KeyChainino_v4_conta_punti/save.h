/*
 * save.h — the count, kept in EEPROM so it survives a flat or swapped
 * battery.
 *
 * Every count is a write, and an EEPROM cell is only good for ~100k of
 * them — one busy project could wear a single fixed cell out. So the
 * count is appended to a ring of 128 slots instead: each store goes to
 * the next slot, and each cell is rewritten only once every 128 counts
 * (~12.8 million counts of endurance).
 *
 * A slot is {count lo, count hi, sequence, check}. load() takes the valid
 * slot with the newest sequence. The check byte is written last, so a
 * write cut short by a dying battery leaves an invalid slot, and the
 * previous count is loaded instead of garbage.
 *
 * A magic word marks the area as formatted; the first run of this
 * firmware wipes the ring once, so bytes left by another sketch are
 * never mistaken for a count.
 */
#ifndef RETHINK_SAVE_H
#define RETHINK_SAVE_H

#include "board.h"

namespace save {

uint16_t load();          // the last stored count (0 if none); call once at boot
void store(uint16_t v);   // append v to the ring (~10ms, blocking)

}  // namespace save

#endif
