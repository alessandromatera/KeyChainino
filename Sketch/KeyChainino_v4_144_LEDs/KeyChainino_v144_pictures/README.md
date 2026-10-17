# KeyChainino 144 — Pictures

A picture book for a 3-year-old, plus a web page where they draw their own
pictures and send them to the KeyChainino.

## On the KeyChainino

| Button | Does |
|---|---|
| **A** (left) | previous picture, with a swift slide (ease-out, overshoot, motion smear); the picture then plays its animation |
| **B** (right) | next picture, same |
| **A + B** | the picture comes alive: built-ins play their own animation (the dog wags, the rocket launches, the fish swims off and back…); the child's drawings get a different effect each time — hop, jelly, sparkle, wave, flip, beat, burst, spin |

- **Wake:** press A+B together. The last picture rises into view and plays its animation.
- Every picture plays its animation when it arrives (wake or A/B); any
  press cuts it short at once, so fast flipping stays snappy.
- **Sleep:** after 30 s without a press it sinks away and powers down (about 1 µA typical).
  The current picture is remembered in EEPROM.
- **Order:** the child's drawings first (newest first), then the built-ins
  chosen for this KeyChainino. All 68 live in flash; a fresh KeyChainino
  shows 5 (smile, heart, star, dog, car — `ON_DEVICE_AT_START` in
  `tools/pictures.mjs`) and the grown-ups' editor on the page changes the
  choice in about a second. The library:
  faces, shapes (circle, square, triangle, rectangle, diamond, oval,
  pentagon, hexagon, plus — each traces its outline, then fills in),
  sky & nature, animals (zebra, lion, tiger, monkey, elephant,
  giraffe, unicorn, snake, penguin, sheep, octopus, whale, ladybug…),
  toys (teddy bear, book, crown, castle, kite, truck…), food.
- Toddler-proof input: the chord fires the moment both buttons are down (no
  tap-vs-hold), auto-repeat is slow and gives up after ~11 s, and wake uses
  pin-change interrupts so a button squeezed in a pocket cannot keep the
  chip awake or drain the CR2032.

## The web page — `upload/keychainino-pictures.html`

One self-contained file (firmware + sticker library embedded). Open it in
Chrome or Edge (WebUSB), with a USBasp on the ISP pads.

- The board is drawn like the real KeyChainino 144, only bigger: white PCB
  with the tab and its hole, ISP pads, passives, the ATmega328P, the two
  tact switches with A / B silkscreen. The lights stay big round dots
  rather than the real tiny SMD LEDs: easier for small fingers.
  It fills the window height; on narrow screens the tools go underneath.
- Drawing: tap a light to toggle it, or press and drag. Or hold the
  **space bar** and just move the mouse: no button to keep pressed, easier
  for small hands. The first light a stroke touches
  decides whether it switches lights on or off; fast moves are filled in
  as solid lines; Undo takes back a whole stroke. Every row plays a note.
- ↩️ Undo · 🧽 Clear · 🐻 Pictures (all built-ins, animated, to start from) ·
  ✨ Dance (plays exactly what A+B will do on the KeyChainino; also the A/B
  keys drawn on the board) · 🚀 Send.
- **Send** reads the KeyChainino, adds the drawing to the EEPROM album (only
  the changed bytes, ~1 s, verified by read-back) and releases RESET: the
  KeyChainino restarts showing the new drawing. The first time it offers to
  install the Pictures program (~15 s; replaces whatever is on it). Sending a
  drawing that is already there just re-selects it; when 40 are stored the
  oldest makes room.
- **Grown-ups:** hold ⚙️ for a second — programmer status, install/update,
  read, delete drawings, sound, ISP speed, log.
- **Choosing what is on the KeyChainino:** hold ✏️ on the "On my
  KeyChainino" shelf for a second (a tap does nothing, so little fingers
  can't). The editor shows what is on it — drawings and chosen pictures,
  × takes one off — and the whole library below, tap to add. Changes are
  staged and written together with **Save to KeyChainino**; it won't save
  an empty KeyChainino. Drawings are matched by their picture, so a list
  read earlier can never take off the wrong one.
- **Kid's 🚀 with a picture:** a built-in picked from the sheet and left
  unchanged goes on as that built-in, with its animation (✅ marks the
  ones already on it); anything drawn or changed goes on as a drawing.
- **Sending is quick:** the page checks the program by reading the
  firmware's 26-byte build id (`BUILD_ID`, a hash of the sketch sources made
  by `build.mjs`) instead of kilobytes of flash, reads only the album bytes
  in use, verifies only what it wrote, and connects once at full speed when
  it already knows the chip runs at 8 MHz. The 🚀 ring shows up the moment
  it is pressed and fills through connect → check → read → write → verify;
  the grown-ups log ends each send with a `timing:` line.
- **Always the latest program:** any change needs the Pictures program in
  this page. With an older one on the KeyChainino the page says an update
  is needed and offers "Update & send" / "Update & save" (about 15 s,
  drawings kept) — nothing is written before that.

Nothing is ever written to the KeyChainino before a grown-up has said yes to
installing the program; the EEPROM header is written after the picture
bytes, so an interrupted send never leaves a half-written slot counted.

## EEPROM album (shared by firmware and page)

See `album.h`: magic `KCP`, version 2, count, selected index, 40 slots of
24 bytes (12 rows, uint16 little endian, bit x = column x), and at 0x3E0 a
32-byte choice: bit (uid & 7) of byte uid/8 = that built-in is shown. Every
picture has a permanent uid in `tools/uids.json` (assigned by `build.mjs`,
never reused — commit it), so adding or reordering pictures never scrambles
a device's choice. Version 1 albums (before the choice) are upgraded in
place, drawings kept. Bump the version in both `album.h` and the page if
the layout changes again.

## Building

```sh
node tools/build.mjs          # pictures.mjs -> pictures_data.*, compile, embed into the page
node tools/preview.mjs out.png  # contact sheet of every picture and frame
node tests/test_sim.mjs       # firmware on the AVR simulator (boot, slides, effects, sleep, EEPROM, stuck button)
node tests/test_page.mjs      # page core vs a simulated USBasp + chip, then that chip booted on the simulator
```

- The art lives in `tools/pictures.mjs` (ASCII, `#` = on) with each
  picture's animation as a list of steps; `pictures_data.*` is generated.
- Compiles with `arduino-cli compile --fqbn arduino:avr:pro:cpu=8MHzatmega328`
  (15.5 KB flash, 855 B RAM). `build.mjs` puts the hex in the workspace's
  `build/pictures/` (next to this repo) or the temp dir.
- The tests use the simulator in the workspace (`simulator/src/core.mjs`,
  next to this repo, with `npm install` done there).
- `board.h`, `display.*`, `gfx.*`, `power.*` are the shared v144 engine,
  the same files in every v144 sketch folder. `input.*`, `album.*` and
  `show.*` belong to this sketch.
