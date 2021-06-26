/**
 * @file    KeyChainino.cpp
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

#include <Arduino.h>

#include "KeyChainino.h"

/* We need to create a global instance so that the ISR knows what to talk to */
KeyChainino KC;

/**
 * @brief Define static member for the location of the LEDs
 */
const charlieLed KeyChainino::charlie_leds_[] = {
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


/**
 * @brief Constructor - Instantiates LED array object
 */
KeyChainino::KeyChainino()
{
    /* Initialize members */
    scrolling_ = 0;
    shift_count_ = 0;
    shift_delay_ = 200; // Arbitrary long wait before scrolling
    scroll_index_ = 0;
    scroll_len_ = 0;

}

/**
 * @brief Destructor
 */
KeyChainino::~KeyChainino()
{

}

/**
 * @brief Configures the pins on the Charlieplex array.
 *
 * You must call this function before performing any other actions on the
 * LED array.
 *
 * @param[in] pins Array of pin numbers. Must be 8 bytes long.
 * @return True if array configured. False on error.
 */
bool KeyChainino::init()
{

// Right now, we only compile for the ATmega 168/328-based Arduinos
#if defined __AVR_ATmega168__ || \
    defined __AVR_ATmega328__ || \
    defined __AVR_ATmega328P__

    /* If we are scrolling, stop and delete our string buffer */
    if ( scrolling_ ) 
    {
        stopScrolling();
    }

    /* Initialize members (again) */
    scrolling_ = 0;
    shift_count_ = 0;
    shift_delay_ = DEFAULT_SHIFT_DELAY;
    scroll_index_ = 0;
    scroll_len_ = 0;

    /* Print out the pins we are using */
# if LED_8X7_DEBUG
    Serial.print(F("Using pins: "));
    for ( int i = 0; i < NUM_CHAPLEX_PINS; i++ ) 
    {
        Serial.print(pins[i]);
        Serial.print(" ");
    }
    Serial.println();
# endif

    /* If we alread have a Chaplex object, delete it */
    /*if ( chaplex_ != NULL ) 
    {
        delete chaplex_;
    }*/
    
    /* Create a new Chaplex object so we can write stuff to the LEDs */
    //chaplex_ = new Chaplex(pins, NUM_CHAPLEX_PINS);

    //pins;
    //numberOfPins = numberOfUserPins;
    ledCtrl = (byte *)malloc(sizeof(byte) * NUM_CHAPLEX_PINS * NUM_CHAPLEX_PINS);
    ledRow = 0;
    allClear();
    outRow();

    /* Calculate the Timer 2 reset number. Aim for 2.048 ms refresh.
       count = 256 - (2.048 ms * F_CPU) / 1024 */
    timer2_count_ =  250;//256 - (2 * (F_CPU / 1000000));  // Aim for 2.048ms refresh //250
    
    /* Initialize Timer 2 */
    noInterrupts();                                 // Disable interrupts
    TCCR2A = 0;                                     // Normal operation
    TCCR2B = 0;                                     // Clear prescaler
    TCNT2 = timer2_count_;                          // Load counter
    TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20);  // Prescaler = 1024
    TIMSK2 |= (1 << TOIE2);                         // Enable timer overflow
    interrupts();                                   // Enable all interrupts


    //disabling all unnecessary peripherals to reduce power
    ADCSRA = 0;
    bitSet(ADCSRA, ADEN); //disable ADC
    bitSet(ACSR, ACD); // disable Analog comparator, saves 4 uA
    power_adc_disable(); // disable ADC converter
    power_spi_disable();
    power_usart0_disable();
    power_twi_disable();
    wdt_disable();


    /* Clear and load frame buffer */
    clear();
    display();
    
    return true;
    
#else

    return false;
    
#endif
}

void KeyChainino::goSleep() {
  //going sleep to reduce power consuming

    clear();
    display();

    for (byte i=0; i<NUM_CHAPLEX_PINS; i++) 
        pinMode(pins[i],INPUT);

  power_timer0_disable(); //disable Timer 0
  power_timer1_disable(); //disable Timer 1
  power_timer2_disable(); //disable Timer 2


  //enable interrupt buttons to allow wakeup from button interrupts
  bitSet(EIMSK, INT0); //enable interrupt button B - INT0
  bitSet(EIMSK, INT1); //enable interrupt button A - INT1

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  while (digitalRead(BUTTON_B) || digitalRead(BUTTON_A)) { //until all the two buttons are pressend
    sleep_mode();
}

  //disable interrupt buttons after sleep
  bitClear(EIMSK, INT0); //enable interrupt button B - INT0
  bitClear(EIMSK, INT1); //enable interrupt button A - INT1

  power_timer0_enable(); //enable Timer 0
  power_timer1_enable(); //enable Timer 1
  power_timer2_enable(); //enable Timer 2


}


// set control-array for one LED ON/OFF
void KeyChainino::ledWrite(charlieLed led, byte state){
    if ((led.r<NUM_CHAPLEX_PINS) && (led.c<NUM_CHAPLEX_PINS) && (led.r!=led.c)) 
        *(ledCtrl+led.r*NUM_CHAPLEX_PINS+led.c) = state;
}

//set control-array for all LEDs OFF
void KeyChainino::allClear(){
    for (byte i=0; i<NUM_CHAPLEX_PINS; i++)
        for (byte j=0; j<NUM_CHAPLEX_PINS; j++)
            *(ledCtrl+i*NUM_CHAPLEX_PINS+j) = OFF;
    }

// write control-array to LEDs of one row and set count (ledRow) to next
    void KeyChainino::outRow() {
        for (byte i=0; i<NUM_CHAPLEX_PINS; i++) 
            pinMode(pins[i],INPUT);

        pinMode(pins[ledRow],OUTPUT);
        digitalWrite(pins[ledRow],HIGH);

        for (byte i=0; i<NUM_CHAPLEX_PINS; i++)  
            if (*(ledCtrl+ledRow*NUM_CHAPLEX_PINS+i)) {
                pinMode(pins[i],OUTPUT);
                digitalWrite(pins[i],LOW);
            } 

            if (++ledRow >= NUM_CHAPLEX_PINS)
                ledRow = 0;
        }


/**
 * @brief Writes the frame buffer to the LED buffer.
 */
        void KeyChainino::display()
        {
            for ( byte i = 0; i < NUM_LEDS; i++ ) 
            {
                ledWrite(charlie_leds_[i], frame_buffer_[i]);
            }
        }

/**
 * @brief Clears the Charlieplex array.
 */
        void KeyChainino::clear()
        {
            memset(frame_buffer_, 0, NUM_LEDS);
        }


//Set all LEDs on
        void KeyChainino::full()
        {
            memset(frame_buffer_, 1, NUM_LEDS);
        }

/**
 * @brief Turns a pixel at a given (x, y) on or off
 *
 * Coordinates start (0, 0) from the top-left of the display.
 *
 * @param[in] x X coordinate for the pixel
 * @param[in] y Y coordinate for the pixel
 * @param[in] on 1 for on, 0 for off.
 */
void KeyChainino::pixel(uint8_t x, uint8_t y, uint8_t on /* = 1 */) {
    /* Check to make sure that we are not accessing outside the array */
        if ( x >= ROW_SIZE || y >= COL_SIZE ) 
        {
            return;
        }

    /* Turn the specified LED on or off. 
     */
      /*      if ( on ) 
            {
                frame_buffer_[(y * COL_SIZE) + x] = 1;
            }
            else 
            {
                frame_buffer_[(y * COL_SIZE) + x] = 0;
            }
            }*/


 /* Turn the specified LED on or off. Note that we need to switch our X and Y
     * for the user, as X goes down and Y goes across on the actual LED display.
     */

        if ( on ) 
        {
            frame_buffer_[(x * COL_SIZE) + y] = 1;
        }
        else 
        {
            frame_buffer_[(x * COL_SIZE) + y] = 0;
        }


    }




/**
 * @brief Draws a line from (x0, y0) to (x1, y1)
 *
 * @param[in] x0 X coordinate for the beginning of the line
 * @param[in] y0 Y coordinate for the beginning of the line
 * @param[in] x1 X coordinate for the ending of the line
 * @param[in] y1 Y coordinate for the ending of the line
 */
    void KeyChainino::line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
        uint8_t steep;
        uint8_t x;
        uint8_t y;
        uint8_t dx;
        uint8_t dy;
        int8_t err;
        int8_t y_step;

    /* Determine if we need to increment in x or y direction */
        steep = (abs(y1 - y0) > abs(x1 - x0)) ? 1 : 0;
        if ( steep ) 
        {
            swap(x0, y0);
            swap(x1, y1);
        }

    /* Make sure we always increment up */
        if ( x0 > x1 ) 
        {
            swap(x0, x1);
            swap(y0, y1);
        }

    /* Calculate rise and span */
        dx = x1 - x0;
        dy = abs(y1 - y0);

    /* Set initial rise/fall error */
    err = dx >> 1;      // Divide by 2
    
    /* Determine our y step depending on rise or fall direction */
    if ( y0 < y1 ) 
    {
        y_step = 1;
    } else {
        y_step = -1;
    }
    
    /* Step through the line, turning on pixels */
    y = y0;
    for ( x = x0; x <= x1; x++ ) 
    {
        if ( steep ) 
        {
            pixel(y, x);
        } 
        else 
        {
            pixel(x, y);
        }
        err -= dy;
        if ( err < 0 ) 
        {
            y += y_step;
            err += dx;
        }
    }
}

/**
 * @brief Draws a rectangle beginning at (x, y) with specified width and height
 *
 * param[in] x X coordinate for top left of rectangle
 * param[in] y Y coordinate for top left of rectangle
 * param[in] width Width of rectangle in pixels
 * param[in] height Height of rectangle in pixels
 */
void KeyChainino::rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
    /* We can't have a width or height of 0; there would be no rectangle */
    if ( width == 0 || height == 0 ) 
    {
        return;
    }

    /* Draw our horizontal lines */
    line(x, y, x + width - 1, y);
    line(x, y + height - 1, x + width - 1, y + height - 1);
    
    /* Don't draw vertical lines if the rectangle is height 2 or less */
    if ( height <= 2 ) 
    {
        return;
    }
    
    /* Draw our vertical lines */
    line(x, y, x, y + height - 1);
    line(x + width - 1, y, x + width - 1, y + height - 1);
}

/**
 * @brief Draws a filled rectangle beginning at (x, y) with width, height
 *
 * param[in] x X coordinate for top left of rectangle
 * param[in] y Y coordinate for top left of rectangle
 * param[in] width Width of rectangle in pixels
 * param[in] height Height of rectangle in pixels
 */
void KeyChainino::rectFill(uint8_t x, 
    uint8_t y, 
    uint8_t width, 
    uint8_t height)
{
    int i;
    
    /* We can't have a width or height of 0; there would be no rectangle */
    if ( width == 0 || height == 0 ) 
    {
        return;
    }
    
    /* Make a filled rectangle with a  bunch of horizontal lines */
    for ( i = y; i < y + height; i++ ) 
    {
        line(x, i, x + width - 1, i);
    }
}        

/**
 * @brief Draws a circle on the LED array
 *
 * @param[in] x X coordinate for the center of the circle
 * @param[in] y Y coordinate for the center of the circle
 * @param[in] radius Distance (in pixels) from center to edge of circle
 */
void KeyChainino::circle(uint8_t x0, uint8_t y0, uint8_t radius)
{
    int8_t y;
    int8_t x;
    int8_t f;
    int8_t ddF_x;
    int8_t ddF_y;
    
    f = 1 - radius;
    ddF_x = 1;
    ddF_y = -2 * radius;
    x = 0;
    y = radius;
    
    pixel(x0, y0 + radius);
    pixel(x0, y0 - radius);
    pixel(x0 + radius, y0);
    pixel(x0 - radius, y0);
    
    while ( x < y ) 
    {
        if ( f >= 0 ) 
        {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        pixel(x0 + x, y0 + y);
        pixel(x0 - x, y0 + y);
        pixel(x0 + x, y0 - y);
        pixel(x0 - x, y0 - y);
        
        pixel(x0 + y, y0 + x);
        pixel(x0 - y, y0 + x);
        pixel(x0 + y, y0 - x);
        pixel(x0 - y, y0 - x);
    }
}

/**
 * @brief Draws a filled circle on the LED array
 *
 * @param[in] x X coordinate for the center of the circle
 * @param[in] y Y coordinate for the center of the circle
 * @param[in] radius Distance (in pixels) from center to edge of circle
 */
void KeyChainino::circleFill(uint8_t x0, uint8_t y0, uint8_t radius)
{
    int8_t y;
    int8_t x;
    int8_t f;
    int8_t ddF_x;
    int8_t ddF_y;
    int8_t i;
    
    f = 1 - radius;
    ddF_x = 1;
    ddF_y = -2 * radius;
    x = 0;
    y = radius;
    
    for ( i = y0 - radius; i <= y0 + radius; i++ ) 
    {
        pixel(x0, i);
    }
    
    while ( x < y ) 
    {
        if ( f >= 0 ) 
        {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        for ( i = y0 - y; i <= y0 + y; i++ ) 
        {
            pixel(x0 + x, i);
            pixel(x0 - x, i);
        }
        
        for ( i = y0 - x; i <= y0 + x; i++ ) 
        {
            pixel(x0 + y, i);
            pixel(x0 - y, i);
        }
    }
}    

/**
 * @brief Loads an array of LED states (on or off).
 *
 * param[in] bitmap Array of LED states. 0 = off, 1 = on.
 */
void KeyChainino::drawBitmap(const byte bitmap[NUM_LEDS])
{
    uint8_t x;
    uint8_t y;
    
    for ( x = 0; x < ROW_SIZE; x++ ) 
    {
        for ( y = 0; y < COL_SIZE; y++ ) 
        {
            //frame_buffer_[(y * COL_SIZE) + x] = (bitmap[(y * COL_SIZE) + x] ? 1: 0);
            frame_buffer_[(x * COL_SIZE) + y] = (bitmap[(y * ROW_SIZE) + x] ? 1: 0);
        }
    }
}


/**
 * @brief Sets text to scroll across the LED array indefinitely
 *
 * @param[in] in_string Text to scroll
 */
void KeyChainino::scrollText(char *in_string)
{
    scrollText(in_string, 0);
}

/**
 * @brief Scrolls text a specified number of times while blocking processor
 *
 * @param[in] in_string Text to scroll
 * @param[in] times Number of times to scroll the text
 * @param[in] blocking True to stop execution until scrolling complete
 */
void KeyChainino::scrollText(char *in_string, int times, bool blocking)
{
    int i;
    int j;
    int char_ind;
    int buf_ind;
    uint8_t text_len;
    uint8_t dict_size;
    uint8_t char_size;

    /* If we are scrolling, stop and delete our string buffer */
    if ( scrolling_ ) 
    {
        stopScrolling();
    }
    
    /* Reset our counters */
    shift_count_ = 0;
    scroll_index_ = 0;
    scroll_times_ = times;
    scroll_count_ = 0;
    
    /* Calculate characters in the string */
    text_len = strlen(in_string);
    if ( text_len > MAX_CHARS ) 
    {
        text_len = MAX_CHARS;
    }
#if LED_8X7_DEBUG
    Serial.print(F("String length: "));
    Serial.println(text_len, DEC);
#endif

    /* Calculate size of buffer */
    scroll_len_ = 0;
    dict_size = sizeof(char_table) / sizeof(char_table[0]);
#if LED_8X7_DEBUG
    Serial.print(F("Dictionary size: "));
    Serial.println(dict_size, DEC);
#endif
    for ( i = 0; i < text_len; i++ ) 
    {

        /* Find where in the dictionary the character occurs */
        char_ind = in_string[i] - CHAR_OFFSET;
        
        /* If character is not in the dictionary, don't count it */
        if ( char_ind < 0 || char_ind >= dict_size ) 
        {
            continue;
        }

        /* Add the number of columns in the character plus the space after */
        scroll_len_ += getPGMFontByte(char_ind);
        scroll_len_ += CHAR_SPACE;
    }
    scroll_len_ += END_SPACE;
#if LED_8X7_DEBUG
    Serial.print(F("Scroll buffer size: "));
    Serial.println(scroll_len_, DEC);
#endif

    /* Create buffer */
    buf_ind = 0;
    scroll_buf_ = (byte*)malloc(scroll_len_ * sizeof(byte));
    for ( i = 0; i < text_len; i++ ) 
    {

        /* Find where in the diction the character occurs */
        char_ind = in_string[i] - CHAR_OFFSET;
        
        /* If character is not in the dictionary, skip it */
        if ( char_ind < 0 || char_ind >= dict_size ) 
        {
            continue;
        }
        
        /* For that character, load in the definition (bytes show which LEDs) */
        char_size = getPGMFontByte(char_ind);
        for ( j = 1; j < char_size + 1; j++ ) 
        {
            scroll_buf_[buf_ind] = getPGMFontByte(char_ind, j);
            buf_ind++;
        }
        
        /* Add in spaces after each char */
        for ( j = 0; j < CHAR_SPACE; j++ ) 
        {
            scroll_buf_[buf_ind] = 0;
            buf_ind++;
        }
    }
    
    /* Add in spaces after text */
    for ( i = 0; i < END_SPACE; i++ ) 
    {
        scroll_buf_[buf_ind] = 0;
        buf_ind++;
    }
    
    /* Print buffer */
#if 0
    for ( i = 0; i < scroll_len_; i++ ) 
    {
        Serial.println(scroll_buf_[i], HEX);
    }
#endif

    /* Start scrolling */
    scrolling_ = 1;
    
    /* If we are blocking, stop execution until scrolling is done */
    if ( blocking ) 
    {
        while ( scrolling_ ) 
        {
#if 0
            Serial.println("Waiting...");
#endif
        }
    }
}

/**
 * @brief Stops scrolling text and deletes scroll buffer
 */
void KeyChainino::stopScrolling()
{
    scrolling_ = 0;
    if ( scroll_buf_ != NULL ) 
    {
        free(scroll_buf_);
        scroll_buf_ = NULL;
    }
    clear();
    display();
}

/**
 * @brief Returns the width of the LED array
 *
 * @return width of the array (number of LEDs)
 */
uint8_t KeyChainino::getArrayWidth()
{
    return ROW_SIZE;
}

/**
 * @brief Returns the width of the LED array
 *
 * @return width of the array (number of LEDs)
 */
uint8_t KeyChainino::getArrayHeight()
{
    return COL_SIZE;
}

/**
 * @brief Returns the byte at the specified location from the font PROGMEM
 *
 * @param[in] idx the index for the character
 * @param[in] offset number of bytes off from the beginning of the character
 * @return byte at the specified location
 */
unsigned char KeyChainino::getPGMFontByte(int idx, int offset /* = 0 */)
{
    return pgm_read_byte(pgm_read_word(&char_table[idx]) + offset);
}

/**
 * @brief Swaps the given bytes
 *
 * @param[in, out] a first byte (becomes b)
 * @param[in, out] b second byte (becomes a)
 */
void KeyChainino::swap(uint8_t &a, uint8_t &b) 
{
    uint8_t t = a;
    a = b;
    b = t;
}




#if defined __AVR_ATmega168__ || \
defined __AVR_ATmega328__ || \
defined __AVR_ATmega328P__

void KeyChainino::isr()
{

    /* Disable Timer2 interrupts */
    TIMSK2 &= ~(1 << TOIE2);

    /* Shift one column */
    if ( scrolling_ ) 
    {
        shift_count_++;
        if ( shift_count_ >= shift_delay_ ) 
        {
            shift_count_ = 0;
            byte i;
            byte bit_to_shift;
            
            /* Shift all but last column */
            /*for (i = 0; i < NUM_LEDS; i++) {
                if (((i+1) % (COL_SIZE) != 0)){
                    frame_buffer_[i] = frame_buffer_[i + 1];
                } else {
                    frame_buffer_[i] = 0;
                }
            }

            /* Shift in new column at the end */
            /*for (i = 0; i < (ROW_SIZE); i++) 
            {
                bit_to_shift = (scroll_buf_[scroll_index_] >> i) & 0x01;
                //frame_buffer_[i*ROW_SIZE+COL_SIZE-1] = bit_to_shift;
                frame_buffer_[i*ROW_SIZE+COL_SIZE-1] = 1;
            }*/



            /* Shift all but last column */
            for ( i = 0; i < ALL_BUT_LAST_COL; i++ ) 
            {
                frame_buffer_[i] = frame_buffer_[i + COL_SIZE];
            }
            
            /* Shift in new column at the end */
            for ( i = 0; i < COL_SIZE; i++ ) 
            {
                bit_to_shift = (scroll_buf_[scroll_index_] >> i) & 0x01;
                frame_buffer_[ALL_BUT_LAST_COL + i] = bit_to_shift;
            }



            /* Send everything in our new buffer to the LED matrix */
            display();

            /* Increment buffer index and reset if it reaches the end */
            scroll_index_++;
            if ( scroll_index_ >= scroll_len_ ) 
            {
                scroll_index_ = 0;
                if ( scroll_times_ > 0 ) 
                {
                    scroll_count_++;
                    if ( scroll_count_ >= scroll_times_ ) 
                    {
                        stopScrolling();
                    }
                }
            }
        }
    }

    /* Display a row and reset counter */
    outRow();            // Output for 1 LED row
    TCNT2 = timer2_count_;         // Load counter for next interrupt
    TIMSK2 |= (1 << TOIE2);        // Enable timer overflow interrupt
}

/**
 * @brief Global interrupt service routine for Timer 2
 *
 * We define Timer 2 ISR here to allow us to make calls to functions in the
 * KeyChainino class. To do this, we instantiate a KeyChainino object
 * (globally) in the .cpp file.
 **/
ISR(TIMER2_OVF_vect) 
{
    KC.isr();
}

#else

void KeyChainino::isr()
{

}

#endif