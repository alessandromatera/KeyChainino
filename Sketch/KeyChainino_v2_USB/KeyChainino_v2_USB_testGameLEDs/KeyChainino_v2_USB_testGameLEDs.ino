/*************************************************************************
  LEDs test FOR KEYCHAININO www.keychainino.com

  created by Alessandro Matera
* ************************************************************************
*/

#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

#define MATRIX_ROW 8
#define MATRIX_COL 8
#define PIN_NUMBER 9

#define BUTTON_A 2 // INT1
#define BUTTON_B 3 // INT0

unsigned long timer = 64900;

byte i_Charlie = 0;
byte j_Charlie = 0;

const byte pins[PIN_NUMBER] = {4, 5, 6, 7, 8, 9, 10, 11, 12}; //the number of the pin used for the LEDs in ordered

const byte connectionMatrix[MATRIX_ROW][MATRIX_COL][2] = { //the matrix that show the LEDs pin connections. Firs Value is the Anode, second is the Catode
  {{1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}, {8, 0}},
  {{0, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 1}, {7, 1}, {8, 1}},
  {{0, 2}, {1, 2}, {3, 2}, {4, 2}, {5, 2}, {6, 2}, {7, 2}, {8, 2}},
  {{0, 3}, {1, 3}, {2, 3}, {4, 3}, {5, 3}, {6, 3}, {7, 3}, {8, 3}},
  {{0, 4}, {1, 4}, {2, 4}, {3, 4}, {5, 4}, {6, 4}, {7, 4}, {8, 4}},
  {{0, 5}, {1, 5}, {2, 5}, {3, 5}, {4, 5}, {6, 5}, {7, 5}, {8, 5}},
  {{0, 6}, {1, 6}, {2, 6}, {3, 6}, {4, 6}, {5, 6}, {7, 6}, {8, 6}},
  {{0, 7}, {1, 7}, {2, 7}, {3, 7}, {4, 7}, {5, 7}, {6, 7}, {8, 7}},
};

bool matrixState[MATRIX_ROW][MATRIX_COL] = { //the matrix that will be always used to turn ON or OFF the LEDs
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0}
};

//KeyChainino Face stored in FLASH in order to reduce RAM size
const PROGMEM bool KeyChaininoFace[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 1, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 1, 0},
  {0, 0, 1, 1, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0}
};


ISR(TIMER1_OVF_vect) {  // timer1 overflow interrupt service routine
  cli(); //disable interrupt
  TCNT1 = timer;// 65405;

  //THIS PART IS USED TO UPDATE THE BALL'S MOVIMENT IN THE GAME
  //if game is started change ball position

  // THIS PART IS USED TO UPDATE THE CHARLIEPLEXING LEDS MATRIX
  // YOU CAN JUST DON'T CARE ABOUT THIS PART
  // BECAUSE YOU CAN CODE LIKE A STANDARD MATRIX BY MANIPULATING THE
  // VALUE OF THE matrixState MATRIX


  pinMode(pins[connectionMatrix[i_Charlie][j_Charlie][0]], INPUT); //set both positive pole and negative pole
  pinMode(pins[connectionMatrix[i_Charlie][j_Charlie][1]], INPUT); // to INPUT in order to turn OFF the LED


  j_Charlie++;
  if (j_Charlie >= MATRIX_COL) {
    j_Charlie = 0;
    i_Charlie++;
    if (i_Charlie >= MATRIX_ROW) {
      i_Charlie = 0;
    }
  }

  if (matrixState[i_Charlie][j_Charlie] == 1) { //turn on LED with 1 in matrixState
    pinMode(pins[connectionMatrix[i_Charlie][j_Charlie][0]], OUTPUT); //set positive pole to OUTPUT
    pinMode(pins[connectionMatrix[i_Charlie][j_Charlie][1]], OUTPUT); //set negative pole to OUTPUT
    digitalWrite(pins[connectionMatrix[i_Charlie][j_Charlie][0]], HIGH); //set positive pole to HIGH
    digitalWrite(pins[connectionMatrix[i_Charlie][j_Charlie][1]], LOW); //set negative pole to LOW
  }


  sei(); //enable interrupt
}


ISR(INT1_vect) { //BUTTON A INTERRUPT
  //do nothing
}

ISR(INT0_vect) { //BUTTON B INTERRUPT
  //do nothing
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

  bitSet(TIMSK1, TOIE1); // enable Timer1 overflow interrupt:

  TCNT1 = timer;

  // set prescaler
  bitSet(TCCR1B, CS10);

  //disabling all unnecessary peripherals to reduce power
  ADCSRA = 0;
  bitSet(ADCSRA, ADEN); //disable ADC
  bitSet(ACSR, ACD); // disable Analog comparator, saves 4 uA
  power_adc_disable(); // disable ADC converter
  power_spi_disable();
  power_usart0_disable();
  power_usart1_disable();
  power_twi_disable();
  power_timer2_disable();
  power_timer3_disable();
  bitSet(PRR1, 4); //disable timer 4 (bit 4 = PRTIM4)
  wdt_disable();


  // enable global interrupts:
  sei();

  showKeyChaininoFace(); //show KeyChainino smile face
  delay(500);
  clearMatrix(); //clear the Matrix
}

int i;// = MATRIX_ROW - 1;
int j;// = 0;

void loop() {
  if (!digitalRead(BUTTON_B)) {
    i = MATRIX_ROW - 1;
    j = 0;
    while (!digitalRead(BUTTON_B)) {
      game(0);
    }
  }

  if (!digitalRead(BUTTON_A)) {
    i = 0;
    j = MATRIX_COL - 1;
    while (!digitalRead(BUTTON_A)) {
      game(1);
    }
  }

  clearMatrix();
  goSleep(); //sleep to reduce power
}

void game(byte gameN) {
  writeLED(i, j);

  if (gameN == 0) {
    i--;
    if (i < 0) {
      i = 0;
      j++;
      if (j > MATRIX_COL - 1) {
        i = MATRIX_ROW - 1;
        j = 0;
      }
    }
  }


  if (gameN == 1) {
    j--;
    if (j < 0) {
      j = 0;
      i++;
      if (i > MATRIX_ROW - 1) {
        i = 0;
        j = MATRIX_COL - 1;
      }
    }
  }

}
void writeLED(byte i, byte j) {
  byte i_old = i;
  byte j_old = j;
  for (j; j < MATRIX_COL; j++) {
    matrixState[i][j] = 1;
    i++;
  }
  delay(50);

  i = i_old;
  j = j_old;

  for (j; j < MATRIX_COL; j++) {
    matrixState[i][j] = 0;
    i++;
  }
}


void clearMatrix() {
  //clear the matrix by inserting 0 to the matrixState
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      clearMatrixStateBit(i, j);
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

void setMatrixStateBit(byte i, byte j) {
  cli();
  matrixState[i][j] = 1;
  sei();
}
void clearMatrixStateBit(byte i, byte j) {
  cli();
  matrixState[i][j] = 0;
  sei();
}

void goSleep() {
  //going sleep to reduce power consuming

  power_timer0_disable(); //disable Timer 0
  power_timer1_disable(); //disable Timer 1

  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      pinMode(pins[connectionMatrix[i][j][0]], INPUT); //set both positive pole and negative pole
      pinMode(pins[connectionMatrix[i][j][1]], INPUT); // to INPUT in order to turn OFF the LED
    }
  }


  //enable interrupt buttons to allow wakeup from button interrupts
  bitSet(EIMSK, INT0); //enable interrupt button B - INT0
  bitSet(EIMSK, INT1); //enable interrupt button A - INT1

  // Disable the USB interface
  bitClear(USBCON, USBE);

  // Disable the VBUS transition enable bit
  bitClear(USBCON, VBUSTE);

  // Disable the VUSB pad
  bitClear(USBCON, OTGPADE);

  // Freeze the USB clock
  bitClear(USBCON, FRZCLK);

  // Disable USB pad regulator
  bitClear(UHWCON, UVREGE);

  // Clear the IVBUS Transition Interrupt flag
  bitClear(USBINT, VBUSTI);

  // Physically detact USB (by disconnecting internal pull-ups on D+ and D-)
  bitSet(UDCON, DETACH);

  power_usb_disable();

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  sleep_mode();
  //disable interrupt buttons after sleep
  bitClear(EIMSK, INT0); //enable interrupt button B - INT0
  bitClear(EIMSK, INT1); //enable interrupt button A - INT1

  power_timer0_enable(); //enable Timer 0
  power_timer1_enable(); //enable Timer 1

  power_usb_enable();

  USBDevice.attach();
  // delay(100);

}

