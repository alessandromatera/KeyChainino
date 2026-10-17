# KeyChainino

KeyChainino is the first keychain you can program with Arduino: a tiny game
console with an LED matrix, two buttons and a coin cell. This repo holds
the open-source firmware for every KeyChainino, plus an in-browser
simulator of the KeyChainino 144.

More at [keychainino.com](https://www.keychainino.com).

## Boards and sketches

| Folder | Board |
|---|---|
| `Sketch/KeyChainino_v4_144_LEDs/` | **KeyChainino 144**: ATmega328P, 12×12 LEDs (the current board) |
| `Sketch/KeyChainino_v3_64_LEDs/` | KeyChainino v3: ATmega328P, 8×8 LEDs |
| `Sketch/KeyChainino_v1_&_THT/`, `Sketch/Tutorial-WALLED/` | The original KeyChainino and the through-hole kit (THT), 5×6 LEDs |
| `THT/`, `Schematic/` | THT kit schematics and board files |

### KeyChainino 144

Every sketch folder is self-contained, with no library to install:

| Sketch | What it is |
|---|---|
| `KeyChainino_v144_multigame` | The 17-game firmware with a menu: Bricks, Snake, Blocks, Invaders, Tennis, Tower, Frog, Flap, Racer, Echo, Jump, Catch, Whack, Dice, Life, Muncher and Steroids |
| `KeyChainino_v144_bubble` | Bubble shooter |
| `KeyChainino_v144_snake` | Snake, standalone |
| `KeyChainino_v144_quest` | An adventure across 16 rooms: find the sword and two keys, then beat the dungeon boss |
| `KeyChainino_v144_pictures` | A picture book for small children, with a web page where they draw their own pictures and send them to the KeyChainino |
| `KeyChainino_v4_conta_punti` | Stitch and row counter for knitting and sewing |
| `KeyChainino_pet` | A virtual pet to feed, clean and play with |
| `KeyChainino_watch` | A clock: A+B shows the time |
| `test_charlie` | LED matrix test |

## Building and flashing

The 144 is programmed over ISP (e.g. a USBasp). With
[arduino-cli](https://arduino.github.io/arduino-cli/):

```sh
arduino-cli compile --fqbn arduino:avr:pro:cpu=8MHzatmega328 \
  --output-dir build Sketch/KeyChainino_v4_144_LEDs/KeyChainino_v144_multigame

avrdude -patmega328p -cusbasp -Pusb \
  -U lfuse:w:0xe2:m -U hfuse:w:0xd7:m -U efuse:w:0xff:m \
  -U flash:w:build/KeyChainino_v144_multigame.ino.hex:i
```

See [`docs/DEVELOPMENT.md`](docs/DEVELOPMENT.md) for the hardware limits
and how the sketch folders are organized.

## Simulator

Open `simulator/index.html` in a browser to play the KeyChainino 144
without hardware. It runs the real compiled firmware, and you can drop
your own `.hex` onto it. See [`simulator/README.md`](simulator/README.md).

## License

GPL-2.0. See [LICENSE](LICENSE). The LED driver bundled with the pet
and watch sketches derives from SparkFun's 8×7 Charlieplex LED Array
library, and the scrolling text uses SparkFun's "Font 1". Both are
public domain (Beerware).
