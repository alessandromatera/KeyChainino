/**
 * @file    KeyChainino.h
 * @brief   Library for the SparkFun 8x7 Charlieplex LED Array
 * @author  Shawn Hymel, Jim Lindblom (SparkFun Electronics)
 *
 * @copyright   This code is public domain but you buy me a beer if you use
 * this and we meet someday (Beerware license).
 *
 * This library controls the 8x7 Charlieplex LED array. Note that Timer2 is used
 * in this library to control the LED refresh. You will not be able to use it
 * for other uses.
 *
 * The graphics algorithms are based on Jim Lindblom's Micro OLED library:
 * https://github.com/sparkfun/Micro_OLED_Breakout
 *
 * Relies on the Chaplex library written by Stefan Götze.
 */
 
#ifndef KeyChainino_H
#define KeyChainino_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "LED_Font_1.h"
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

/* Debug */
#define LED_8X7_DEBUG   0

/* Constants */
static const unsigned int NUM_CHAPLEX_PINS = 13; // Number of pins  
static const unsigned int DEFAULT_SHIFT_DELAY = 60; // Ticks before scrolling
static const unsigned int MAX_CHARS = 150;      // Maximum characters to scroll
static const unsigned int CHAR_OFFSET = 0x20;   // Starting place for ASCII
static const unsigned int CHAR_SPACE = 1;       // Number columns between chars
static const unsigned int END_SPACE = 12;        // Number columns after text
static const unsigned int COL_SIZE = 12;         // Number LEDs in a column
static const unsigned int ROW_SIZE = 12;         // Number LEDs in a row

/* The 13 LED pins live in PROGMEM inside KeyChainino.cpp (they are private
   to the driver; keeping them out of the header avoids one copy per
   translation unit in multi-file sketches). */

/* Derived constants */
static const unsigned int NUM_LEDS = COL_SIZE * ROW_SIZE;
static const unsigned int ALL_BUT_LAST_COL = NUM_LEDS - COL_SIZE;

/* Global variables */
ISR(TIMER2_OVF_vect);

#define ON 1
#define OFF 0


#define MATRIX_ROW 12
#define MATRIX_COL 12
#define PIN_NUMBER 13

#define BUTTON_A 3 // INT1
#define BUTTON_B 2 // INT0

struct CharlieLed {
  byte r;
  byte c; 
} ;

typedef struct CharlieLed charlieLed;

/* LED Array class */
class KeyChainino {
    
    /* The ISR is our friend! It can call our functions. */
    friend void TIMER2_OVF_vect();
    
public:
    
    /* Initialization */
    KeyChainino();
    ~KeyChainino();
    bool init();
    
    /* LED drawing methods */
    void display();
    void clear();
    void full();
    void pixel(uint8_t x, uint8_t y, uint8_t on = 1);
    void line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
    void rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
    void rectFill(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
    void circle(uint8_t x0, uint8_t y0, uint8_t radius);
    void circleFill(uint8_t x0, uint8_t y0, uint8_t radius);
    void drawBitmap(const byte bitmap[NUM_LEDS]);
    void drawBitmapP(const byte bitmap[NUM_LEDS]); // same, but bitmap lives in PROGMEM

    /* Scrolling text methods */
    void scrollText(char *in_string);
    void scrollText(char *in_string, int times, bool blocking = false);
    void stopScrolling();
    bool isScrolling() { return scrolling_ != 0; }

    /* Support methods */
    uint8_t getArrayWidth();
    uint8_t getArrayHeight();

    /* onIdleWake, if given, is called every time the MCU wakes from
       power-down for a reason OTHER than a completed button chord (i.e. a
       WDT tick, or one button pressed but not yet both) -- right after
       waking, before going back to sleep. Never invoked on the wake that
       actually satisfies the exit condition, so it can't add a spurious
       delay/flash to the caller's own wake gesture.

       IMPORTANT: INT0/INT1 are level-triggered, so holding a single button
       while asleep re-fires that interrupt (and this callback) continuously
       for as long as it's held -- potentially thousands of times for one
       brief press, not just once. The callback MUST be cheap on the common
       "nothing to do" path (e.g. gate its real work behind its own "did
       something actually change" flag, not just run unconditionally) and
       is responsible for its own complete cleanup if it draws anything:
       power_timer0_enable()/power_timer2_enable() to use delay()/display(),
       then prepSleepIdle() before returning. goSleep() does NOT re-idle
       after the callback for you -- calling prepSleepIdle() unconditionally
       on every one of those thousands of spurious re-entries was tried and
       caused real state corruption, not just wasted cycles, so it's the
       callback's job to call it only on the rare occasions it's needed. */
    void goSleep(void (*onIdleWake)() = nullptr);

    /* Blanks the display, tri-states all 13 charlieplex pins (no leakage
       while asleep) and gates Timer0/1/2. Called once by goSleep() before
       its first sleep_cpu(); an onIdleWake callback that draws anything
       must call this itself before returning -- see goSleep() above. */
    void prepSleepIdle();


private:

    /* Helper functions */
    unsigned char getPGMFontByte(int idx, int offset = 0);
    void swap(uint8_t &a, uint8_t &b);
    void ledWrite(charlieLed led, byte state);
    void allClear();
    void outRow();

    /* Interrupt service routine that is called by the system's ISR */
    inline void isr();

    /* Members */
    //Chaplex *chaplex_;          /// Chaplex object for controlling the LEDs
    byte frame_buffer_[144];     /// Storing the state of each LED to be written
    uint8_t timer2_count_;      /// Stores the next start point for Timer2
    byte *scroll_buf_;          /// Buffer of text graphics to scroll
    volatile byte scrolling_;   /// Boolean to indicate if we are scrolling text
    unsigned int shift_count_;  /// Count number of ticks before shifting text
    unsigned int shift_delay_;  /// Number of ticks to wait before shifting text
    unsigned int scroll_index_; /// Index of where to scroll text
    unsigned int scroll_len_;   /// Number of bytes in scroll_buf_
    unsigned int scroll_times_; /// Number of times to scroll text
    unsigned int scroll_count_; /// Counter for times text has scrolled
    /* The (row,col) pin map of the 144 LEDs lives in PROGMEM in the .cpp */

    byte ledCtrl[NUM_CHAPLEX_PINS * NUM_CHAPLEX_PINS]; /// Charlieplex on/off control array (static, no heap)
    volatile byte ledRow;

    //void Chaplex(byte* userPins,byte numberOfUserPins);
    
    
};

/* Yup, we're going to declare a global instance of our object */
extern KeyChainino KC;

#endif // KeyChainino_H