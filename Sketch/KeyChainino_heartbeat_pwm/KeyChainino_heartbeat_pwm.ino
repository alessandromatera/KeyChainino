/*************************************************************************
   HEARTBEAT PWM FOR KEYCHAININO www.keychainino.com

   created by Alessandro Matera

   Simulate the heartbeat by using pwm to turn on the heart matrix in a softly way.
   After 2 beats, the micro goes on standby and it will be wake up by the watchdog interrupt, set to 8 sec.

 * ************************************************************************
*/

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

#define MATRIX_ROW 5
#define MATRIX_COL 6
#define PIN_NUMBER 7

#define BUTTON_A 6 //pin 6 - PCINT6
#define BUTTON_B 8 //pin 8 - INT0

unsigned long timer = 65535;

const unsigned int max_pwm_counter = 20;
unsigned int pwm_counter = 0;
byte pwm_matrix_set = max_pwm_counter;

byte i_Charlie = 0;
byte j_Charlie = 0;

const byte pins[PIN_NUMBER] = {0, 1, 2, 3, 7, 9, 10}; //the number of the pin used for the LEDs in ordered

const byte connectionMatrix[MATRIX_ROW][MATRIX_COL][2] = { //the matrix that shows the LEDs pin connections. First Value is the Anode, second is the Cathode
  {{1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}},
  {{0, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 1}},
  {{0, 2}, {1, 2}, {3, 2}, {4, 2}, {5, 2}, {6, 2}},
  {{0, 3}, {1, 3}, {2, 3}, {4, 3}, {5, 3}, {6, 3}},
  {{0, 4}, {1, 4}, {2, 4}, {3, 4}, {5, 4}, {6, 4}}
};

bool matrixState[MATRIX_ROW][MATRIX_COL] = { //the matrix that will be always used to turn ON or OFF the LEDs
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0}
};

//KeyChainino Face stored in FLASH in order to reduce RAM size
const PROGMEM bool KeyChaininoFace[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 0, 0, 0},
  {0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {1, 0, 0, 0, 0, 1},
  {0, 1, 1, 1, 1, 0}
};

//heart matrix stored in FLASH in order to reduce RAM size
const PROGMEM bool heart[MATRIX_ROW][MATRIX_COL] = {
  {1, 1, 0, 0, 1, 1},
  {1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1},
  {0, 1, 1, 1, 1, 0},
  {0, 0, 1, 1, 0, 0}
};

ISR(TIM1_OVF_vect) {  // timer1 overflow interrupt service routine

  pwm_counter++;
  if (pwm_counter >= pwm_matrix_set) {
    pinMode(pins[connectionMatrix[i_Charlie][j_Charlie][0]], INPUT); //set both positive pole and negative pole
    pinMode(pins[connectionMatrix[i_Charlie][j_Charlie][1]], INPUT); // to INPUT in order to turn OFF the LED
  }

  if (pwm_counter > max_pwm_counter) {

    pwm_counter = 0;

    // THIS PART IS USED TO UPDATE THE CHARLIEPLEXING LEDS MATRIX
    // YOU CAN JUST DON'T CARE ABOUT THIS PART
    // BECAUSE YOU CAN CODE LIKE A STANDARD MATRIX BY MANIPULATING THE
    // VALUE OF THE matrixState MATRIX

    pinMode(pins[connectionMatrix[i_Charlie][j_Charlie][0]], INPUT); //set both positive pole and negative pole
    pinMode(pins[connectionMatrix[i_Charlie][j_Charlie][1]], INPUT); // to INPUT in order to turn OFF the LED


    j_Charlie++;
    if (j_Charlie == MATRIX_COL) {
      j_Charlie = 0;
      i_Charlie++;
      if (i_Charlie == MATRIX_ROW) {
        i_Charlie = 0;
      }
    }

    if (pwm_matrix_set != 0) {
      if (matrixState[i_Charlie][j_Charlie] == 1) { //turn on LED with 1 in matrixState
        pinMode(pins[connectionMatrix[i_Charlie][j_Charlie][0]], OUTPUT); //set positive pole to OUTPUT
        pinMode(pins[connectionMatrix[i_Charlie][j_Charlie][1]], OUTPUT); //set negative pole to OUTPUT
        digitalWrite(pins[connectionMatrix[i_Charlie][j_Charlie][0]], HIGH); //set positive pole to HIGH
        digitalWrite(pins[connectionMatrix[i_Charlie][j_Charlie][1]], LOW); //set negative pole to LOW
      }
    }
  }

  TCNT1 = timer;
}

ISR(PCINT0_vect) { //BUTTON A INTERRUPT
  //do nothing
}

ISR(INT0_vect) { //BUTTON B INTERRUPT
  //do nothing
}

ISR(WDT_vect)
{
  // Don't do anything here but we must include this
  // block of code otherwise the interrupt calls an
  // uninitialized interrupt handler.
}




void setup() {
  //configure LED pins
  for (byte i = 0; i < PIN_NUMBER; i++) {
    pinMode(pins[i], INPUT);
  }

  //configure Buttons pins
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);

  // initialize Timer1
  cli();         // disable global interrupts
  TCCR1A = 0;    // set entire TCCR1A register to 0
  TCCR1B = 0;    // set entire TCCR1A register to 0

  // enable Timer1 overflow interrupt:
  TIMSK1 |= (1 << TOIE1);

  // preload timer 65536 - (8000000 / 1024 / 60) = 60Hz
  TCNT1 = timer;// 65405;

  // set no prescaler
  bitSet(TCCR1B, CS10);

  bitSet(GIMSK, PCIE0); //enable pingChange global interrupt

  //disabling all unnecessary peripherals to reduce power
  ADCSRA &= ~bit(ADEN); //disable ADC
  power_adc_disable(); // disable ADC converter
  power_usi_disable(); // disable USI
  wdt_disable(); //disable watchdog

  // enable global interrupts:
  sei();

  goSleep();

}

void loop() {

  heartbeat();
  goSleep();

}

void heartbeat() {

  byte delay_beat = 3;
  showHeart();
  pwm_matrix_set = 0;

  for (byte i = 0; i < 1; i++) {

    while (pwm_matrix_set < max_pwm_counter) {
      pwm_matrix_set++;
      delay(delay_beat);
    }
    while (pwm_matrix_set > 0) {
      pwm_matrix_set--;
      delay(delay_beat);
    }

    delay(100);

    while (pwm_matrix_set < max_pwm_counter) {
      pwm_matrix_set++;
      delay(delay_beat);
    }
    while (pwm_matrix_set > 0) {
      pwm_matrix_set--;
      delay(delay_beat);
    }

    delay(500);
  }

}


void clearMatrix() {
  //clear the matrix by inserting 0 to the matrixState
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = 0;
    }
  }
}

void fullMatrix() {
  //turn on all LEDs in the matrix by inserting 1 to the matrixState
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      setMatrixStateBit(i, j);
    }
  }
}


void showKeyChaininoFace() {
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(KeyChaininoFace[i][j])); //here we read the matrix from FLASH
    }
  }
}

void showHeart() {
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(heart[i][j])); //here we read the matrix from FLASH
    }
  }
}

//here we set or clear a single bit on the matrixState. We use this funciton in order
//to really set or clear the matrix's bit when an interrupt occours. To do that we disable the
//interrupt -> set or clear the bit -> enable interrupt

void setMatrixStateBit(byte i, byte j) {
  matrixState[i][j] = 1;
}
void clearMatrixStateBit(byte i, byte j) {
  matrixState[i][j] = 0;
}

void goSleep() {
  //going sleep to reduce power consuming

  power_timer0_disable(); //disable Timer 0
  power_timer1_disable(); //disable Timer 1

  //clean the charlieplexing
  i_Charlie = 0;
  j_Charlie = 0;

  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      pinMode(pins[connectionMatrix[i][j][0]], INPUT); //set both positive pole and negative pole
      pinMode(pins[connectionMatrix[i][j][1]], INPUT); // to INPUT in order to turn OFF the LED
    }
  }
  watchdogSetup(); //enable watchDog

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_mode();

  wdt_disable(); //disable watchdog after sleep

  power_timer0_enable(); //enable Timer 0
  power_timer1_enable(); //enable Timer 1
}

void watchdogSetup() {

  //WDP3 - WDP2 - WPD1 - WDP0 - time
  // 0      0      0      0      16 ms
  // 0      0      0      1      32 ms
  // 0      0      1      0      64 ms
  // 0      0      1      1      0.125 s
  // 0      1      0      0      0.25 s
  // 0      1      0      1      0.5 s
  // 0      1      1      0      1.0 s
  // 0      1      1      1      2.0 s
  // 1      0      0      0      4.0 s
  // 1      0      0      1      8.0 s

  // Reset the watchdog reset flag
  bitClear(MCUSR, WDRF);

  // Start timed sequence
  bitSet(WDTCSR, WDCE); //Watchdog Change Enable to clear WD

  // Set new watchdog timeout value to 8sec
  bitSet(WDTCSR, WDP3);
  bitClear(WDTCSR, WDP2);
  bitClear(WDTCSR, WDP1);
  bitSet(WDTCSR, WDP0);

  //Enable WD
  bitSet(WDTCSR, WDE);
  
  // Enable interrupts instead of reset
  bitSet(WDTCSR, WDIE);
}
