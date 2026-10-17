/*
 * display.cpp — compiled-scanline charlieplex driver with greyscale.
 * See display.h for the design. Timer2 belongs to this file.
 *
 * Greyscale on charlieplexed hardware, honestly: all lit LEDs of a
 * scanline share the anode pin's current, so a pixel's absolute
 * brightness already varies (up to a few x) with how many neighbours
 * share its line — the classic firmware has the same behavior. Subtle
 * duty ratios drown in that variation, so the two brightness planes are
 * weighted 1:7, not 1:2: BR_DIM is ~1/8 duty and reads unmistakably
 * dimmer than BR_MAX on every line, whatever the load. Think of it as
 * three honest levels — off, dim (scenery), bright (action); BR_MID
 * (~7/8) is just a hair under full and is used where "bright" is enough.
 *
 * The scan table is double-buffered: flush() builds the back table and
 * flips an index, so the ISR never displays a half-built frame (an
 * earlier in-place rebuild made whole scanlines go dark for a slot on
 * every flush — visible as blinking at the games' 50Hz redraw rate).
 */
#include "display.h"

#include <avr/power.h>
#include <avr/wdt.h>

namespace display {

uint8_t fb[MATRIX_W * MATRIX_H];

/* ------------------------------------------------------------------ *
 *  Hardware map                                                       *
 * ------------------------------------------------------------------ */

#define N_PINS 13

/* The 13 matrix lines are Arduino pins 4..10 and A0..A5, in this order.
   We never go through Arduino pin functions: each line is (port, bit).
   Port ids: 0 = B, 1 = C, 2 = D. */
#define PB_ 0
#define PC_ 1
#define PD_ 2

static const uint8_t PIN_PORT[N_PINS] PROGMEM = {
  PD_, PD_, PD_, PD_,        // D4..D7  -> PD4..PD7
  PB_, PB_, PB_,             // D8..D10 -> PB0..PB2
  PC_, PC_, PC_, PC_, PC_, PC_  // A0..A5 -> PC0..PC5
};
static const uint8_t PIN_MASK[N_PINS] PROGMEM = {
  _BV(4), _BV(5), _BV(6), _BV(7),
  _BV(0), _BV(1), _BV(2),
  _BV(0), _BV(1), _BV(2), _BV(3), _BV(4), _BV(5)
};

/* LED i lights when line .a sources current (high) and line .c sinks it
   (low). Table copied verbatim from the proven classic library; index i
   matches the framebuffer (fb[x*12+y] -> i = x*12+y). */
struct LedMap { uint8_t a, c; };

static const LedMap LED_MAP[MATRIX_W * MATRIX_H] PROGMEM = {
    {1, 0}, {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {0, 6}, {0, 7}, {0, 8}, {0, 9}, {0, 10}, {0, 11},
    {2, 0}, {2, 1}, {1, 2}, {1, 3}, {1, 4}, {1, 5}, {1, 6}, {1, 7}, {1, 8}, {1, 9}, {1, 10}, {1, 11},
    {3, 0}, {3, 1}, {3, 2}, {2, 3}, {2, 4}, {2, 5}, {2, 6}, {2, 7}, {2, 8}, {2, 9}, {2, 10}, {2, 11},
    {4, 0}, {4, 1}, {4, 2}, {4, 3}, {3, 4}, {3, 5}, {3, 6}, {3, 7}, {3, 8}, {3, 9}, {3, 10}, {3, 11},
    {5, 0}, {5, 1}, {5, 2}, {5, 3}, {5, 4}, {4, 5}, {4, 6}, {4, 7}, {4, 8}, {4, 9}, {4, 10}, {4, 11},
    {6, 0}, {6, 1}, {6, 2}, {6, 3}, {6, 4}, {6, 5}, {5, 6}, {5, 7}, {5, 8}, {5, 9}, {5, 10}, {5, 11},
    {7, 0}, {7, 1}, {7, 2}, {7, 3}, {7, 4}, {7, 5}, {7, 6}, {6, 7}, {6, 8}, {6, 9}, {6, 10}, {6, 11},
    {8, 0}, {8, 1}, {8, 2}, {8, 3}, {8, 4}, {8, 5}, {8, 6}, {8, 7}, {7, 8}, {7, 9}, {7, 10}, {7, 11},
    {9, 0}, {9, 1}, {9, 2}, {9, 3}, {9, 4}, {9, 5}, {9, 6}, {9, 7}, {9, 8}, {8, 9}, {8, 10}, {8, 11},
    {10, 0}, {10, 1}, {10, 2}, {10, 3}, {10, 4}, {10, 5}, {10, 6}, {10, 7}, {10, 8}, {10, 9}, {9, 10}, {9, 11},
    {11, 0}, {11, 1}, {11, 2}, {11, 3}, {11, 4}, {11, 5}, {11, 6}, {11, 7}, {11, 8}, {11, 9}, {11, 10}, {10, 11},
    {12, 0}, {12, 1}, {12, 2}, {12, 3}, {12, 4}, {12, 5}, {12, 6}, {12, 7}, {12, 8}, {12, 9}, {12, 10}, {12, 11},
};

/* ------------------------------------------------------------------ *
 *  Compiled scan tables (double-buffered)                             *
 * ------------------------------------------------------------------ */

/* One entry per (scanline, brightness plane): the exact register values
   the ISR writes. Layout: {DDRB, PORTB, DDRC, PORTC, DDRD, PORTD}. */
#define E_DDR(port) ((port) * 2)
#define E_PORT(port) ((port) * 2 + 1)

static uint8_t scanTabs[2][N_PINS][2][6];
static volatile uint8_t frontTab;  // table the ISR displays

/* Scanline timing is CYCLE-hardcoded, exactly like the classic driver
   (6144 CPU cycles per line — proven on the real board no matter what
   clock the build claims), NOT derived from F_CPU. With Timer2 at /64
   that is 96 ticks per line (768us at a true 8MHz -> ~100Hz refresh),
   split 1:7 between the brightness planes. */
#define SLOT0_TICKS 12  // plane 0: pixel bit 0, weight 1
#define SLOT1_TICKS 84  // plane 1: pixel bit 1, weight 7

void flush() {
  uint8_t (*tab)[2][6] = scanTabs[1 - frontTab];  // build the back table

  /* Base pass: empty slots. PORTD keeps the button pull-ups alive
     because the ISR writes the whole register. */
  for (uint8_t l = 0; l < N_PINS; l++) {
    for (uint8_t pl = 0; pl < 2; pl++) {
      uint8_t* e = tab[l][pl];
      e[0] = e[1] = e[2] = e[3] = e[4] = 0;
      e[5] = PORTD_BUTTON_PULLUPS;
    }
  }

  /* Pixel pass: each lit pixel adds its cathode (output, low) to the
     plane(s) selected by its brightness bits. */
  for (uint8_t i = 0; i < MATRIX_W * MATRIX_H; i++) {
    uint8_t v = fb[i] & 3;
    if (!v) continue;
    uint8_t a = pgm_read_byte(&LED_MAP[i].a);
    uint8_t c = pgm_read_byte(&LED_MAP[i].c);
    uint8_t m = pgm_read_byte(&PIN_MASK[c]);
    uint8_t d = E_DDR(pgm_read_byte(&PIN_PORT[c]));
    if (v & 1) tab[a][0][d] |= m;
    if (v & 2) tab[a][1][d] |= m;
  }

  /* Anode pass: energize a line only in the planes where it actually
     has lit pixels. Driving the anode of an empty slot (as the first
     version did) charges all 12 floating LED lines through the LEDs —
     invisible next to bright pixels, but real "ghost" light next to a
     lone dim pixel. */
  for (uint8_t l = 0; l < N_PINS; l++) {
    uint8_t p = pgm_read_byte(&PIN_PORT[l]);
    uint8_t m = pgm_read_byte(&PIN_MASK[l]);
    for (uint8_t pl = 0; pl < 2; pl++) {
      uint8_t* e = tab[l][pl];
      if (e[0] | e[2] | e[4]) {  // some cathode is set: line is in use
        e[E_DDR(p)] |= m;        // anode: output...
        e[E_PORT(p)] |= m;       // ...driving high
      }
    }
  }

  /* Publish: make sure the table stores land before the flip. */
  asm volatile("" ::: "memory");
  frontTab = 1 - frontTab;
}

/* ------------------------------------------------------------------ *
 *  Timer2 scan ISR                                                    *
 * ------------------------------------------------------------------ */

static uint8_t curLine = 0;
static uint8_t curPlane = 0;

/* Every matrix line as output, for the discharge clamp (buttons PD2/PD3
   stay inputs with their pull-ups). */
#define ALL_B (_BV(0) | _BV(1) | _BV(2))
#define ALL_C (_BV(0) | _BV(1) | _BV(2) | _BV(3) | _BV(4) | _BV(5))
#define ALL_D (_BV(4) | _BV(5) | _BV(6) | _BV(7))

/* Between slots: drive every line LOW for an instant to dump the charge
   parked on floating pins (each energized slot charges them through the
   LEDs; released as a faint ghost flash into whatever conducts next),
   then release to Hi-Z. All-low is safe — no potential difference, no
   LED conducts — and even one clamped cycle empties ~10pF completely. */
static inline void dischargeAndBlank() {
  PORTB = 0;
  PORTC = 0;
  PORTD = PORTD_BUTTON_PULLUPS;
  DDRB = ALL_B;
  DDRC = ALL_C;
  DDRD = ALL_D;
  DDRB = 0;
  DDRC = 0;
  DDRD = 0;
}

ISR(TIMER2_COMPA_vect) {
  /* Advance to the next slot: plane 0 -> plane 1 -> next line. */
  curPlane ^= 1;
  if (!curPlane && ++curLine >= N_PINS) curLine = 0;

  const uint8_t* e = scanTabs[frontTab][curLine][curPlane];

  dischargeAndBlank();  // kill residual charge: no ghosting between slots
  PORTB = e[1];
  PORTC = e[3];
  PORTD = e[5];
  DDRB = e[0];
  DDRC = e[2];
  DDRD = e[4];

  OCR2A = (curPlane ? SLOT1_TICKS : SLOT0_TICKS) - 1;  // this slot's length
}

/* ------------------------------------------------------------------ *
 *  Lifecycle                                                          *
 * ------------------------------------------------------------------ */

void begin() {
  memset(fb, 0, sizeof(fb));
  frontTab = 0;
  flush();  // builds table 1 and flips to it
  flush();  // builds table 0 too, so both start clean
  dischargeAndBlank();

  noInterrupts();
  TCCR2A = _BV(WGM21);  // CTC on OCR2A
  TCCR2B = _BV(CS22);   // prescaler /64
  TCNT2 = 0;
  OCR2A = SLOT0_TICKS - 1;
  TIMSK2 = _BV(OCIE2A);
  interrupts();

  /* Power down everything this firmware never uses. Timer1 too: all
     pacing (debounce, scroll, show time) runs off millis(). */
  ADCSRA = 0;
  power_adc_disable();
  power_spi_disable();
  power_usart0_disable();
  power_twi_disable();
  power_timer1_disable();
  wdt_disable();
}

void park() {
  TIMSK2 = 0;
  dischargeAndBlank();
}

void resume() {
  noInterrupts();
  TCNT2 = 0;
  OCR2A = SLOT0_TICKS - 1;
  TIMSK2 = _BV(OCIE2A);
  interrupts();
}

}  // namespace display
