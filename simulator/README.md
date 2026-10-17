# KeyChainino 144 Simulator

`index.html` is a single-file web page that emulates the KeyChainino 144:
an ATmega328P at 8 MHz, the 12×12 charlieplexed LED matrix and the two
buttons. It runs the **real compiled firmware**, instruction by
instruction, in your browser.

Open `index.html` in any browser. Double-clicking works, and no server is
needed. The 17-game firmware is built in. Hold **A + B** to power on, just
like the real board.

## Run your own sketch

Browsers can't compile C++, so give the page a compiled hex. From the repo
root:

```sh
arduino-cli compile --fqbn arduino:avr:pro:cpu=8MHzatmega328 \
  --output-dir build Sketch/KeyChainino_v4_144_LEDs/<sketch>
```

Then drop `build/<sketch>.ino.hex` anywhere onto the page.

The display isn't read from the firmware's memory. The page watches the 13
charlieplex pins and lights an LED only while its pin pair is actually
driven, so scan timing, ghosting and dimming look like they do on the
hardware. Sleep current isn't modeled.
