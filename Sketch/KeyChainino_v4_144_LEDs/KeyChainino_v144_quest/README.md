# KeyChainino 144 — QUEST

A single-game adventure with a real world inside: a 4×4 grid of rooms
(16 hand-authored 12×12 screens) on the 144-LED matrix. Find the sword
in its cave, hunt down two keys, unlock the dungeon, slay the boss and
claim the treasure. Built on the rethought multigame machine (greyscale
charlieplex driver, zero-heap text, everything constant in PROGMEM) —
no library to install, everything is in this folder.

## How to play

| Gesture | Action |
| --- | --- |
| tap **A** | turn 90° clockwise |
| tap / hold **B** | step / walk where you're facing |
| tap **A+B** | sword strike into the faced cell (once you own it) |
| hold **A+B** 1.2 s | save and sleep |
| …keep holding ~3 s | a bar fills along the bottom: wipe the save, fresh quest |
| press **A+B** asleep | wake up exactly where you left off |

A dim pixel one cell ahead of you is your **facing marker** — it sits
where B would take you; no marker means you're facing something solid.

Reading the screen (three honest brightness levels):

* **static dim** — walls; **dim, winking off** — a locked door
* **solid bright** — you (fast-blinking: just hurt, invulnerable)
* **bright with dim dips** — an enemy; a **2×2 bright block** — the boss
* **slow on/off beacon** — an item: key, heart, sword, treasure
* after any event, row 0 shows pips for ~1.5 s: **hearts left, keys right**

## The quest

You wake at home in the southwest. The sword is one room north, in a
walled cave chamber ("it's dangerous to go alone"). Two keys sit in
guarded groves; the northeast corner is a locked two-room dungeon — a
door into the antechamber gauntlet, a second door into the boss room.
The boss takes three sword hits and knocks back when struck. Touch the
treasure to win.

Touching an enemy (or walking into one) costs a heart, shoves you back
and grants ~1 s of invulnerability; three hearts total, heart pickups
refill (they wait for you if you're full). Death is forgiving: the
world keeps every item and opened door, you wake at home with full
hearts. Enemies respawn when you re-enter a room; items and doors never
come back.

## The keychain IS the save slot

Every room change and event mirrors the quest to EEPROM (10 bytes,
`EEPROM.update`, magic-validated): position, facing, hearts, keys, the
taken-items bitmask, the opened-doors bitmask. The 30 s idle sleep and
the exit chord both save first — pocket it mid-dungeon, wake it next
week, keep playing. A corrupt or foreign save falls back to a fresh
quest; victory wipes the save for the next run.

## The world is data

Rooms are ASCII maps in `world.cpp` — 12 literal strings per room, so
the world is edited with a text editor (`#` wall, `.` floor, `D` door,
`K`/`H`/`S`/`T` items, `E` enemy, `B` boss, `@` start). Rooms connect
through 1-cell gaps at row/col 5, so shared edges line up by
construction. `tools/check_world.py` machine-checks the whole thing:
sizes, charset, edge agreement, door placement, item counts, flood-fill
reachability of every item/enemy/boss (with doors closed the dungeon
must be sealed and both keys still reachable — no soft locks), plus a
lone-wall-pixel ghosting lint. Run it after any map edit:

```sh
python3 tools/check_world.py
```

## Layout

| File | Role |
| --- | --- |
| `*.ino` | boot, wake ISRs, setup, the two-line loop |
| `quest.*` | the game: movement, combat, items, doors, lifecycle |
| `world.*` | 16 ASCII rooms in PROGMEM + item/door identity |
| `save.*` | 10-byte EEPROM quest save |
| `icons.*` | treasure title + the KeyChainino faces |
| `board.h`, `display.*`, `gfx.*`, `text.*`, `font_data.h`, `input.*`, `power.*` | the rethought driver stack, copied unchanged from `KeyChainino_v144_multigame` |

## Building

Arduino IDE with MiniCore (`MiniCore:avr:328`), **Clock: Internal
8 MHz** — `board.h` refuses any other clock (a bare FQBN silently builds
half-speed `millis()`). Or:

```sh
arduino-cli compile --fqbn MiniCore:avr:328:clock=8MHz_internal KeyChainino_v144_quest
```

~13.5 KB flash (41 %), ~620 B RAM (30 %).

## Testing without hardware

The repo's `simulator/` runs the real compiled hex cycle-accurately.
Compile with `--output-dir build`, then drop
`build/KeyChainino_v144_quest.ino.hex` onto `simulator/index.html` — or
run the headless suites:

```sh
node simulator/src/test-quest.mjs        # boot, walk to the sword, save/resume
node simulator/src/test-quest-story.mjs  # seeded saves: door/keys, boss, victory
```
