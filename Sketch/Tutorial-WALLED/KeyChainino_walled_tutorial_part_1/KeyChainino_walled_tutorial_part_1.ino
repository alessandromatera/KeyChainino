/*******************************************************************************************************
 * KeyChainino Walled Game TUTORIAL PART 1 - FOR KEYCHAININO www.keychainino.com  
 * 
 * learn more here: http://www.keychainino.com/tutorial-how-to-make-the-walled-game-step-by-step-part-1/
 * 
 * created by Alessandro Matera 03/01/2016
 * ******************************************************************************************************
*/

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/power.h>

#define MATRIX_ROW 5
#define MATRIX_COL 6
#define PIN_NUMBER 7

#define BUTTON_A 6 //pin 6 - PCINT6
#define BUTTON_B 8 //pin 8 - INT0

const byte pins[PIN_NUMBER] = {0, 1, 2, 3, 7, 9, 10}; //the number of the pin used for the LEDs in ordered

const byte connectionMatrix[MATRIX_ROW][MATRIX_COL][2] = { //the matrix that shows the LEDs pin connections. First Value is the Anode, second is the Catode
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

//MAN VARIABLES
byte manXPosition = 2;//the position of the man

byte keySensibility = 70; //the sensibility of the two touch buttons. Decrease to have more sensitivity


ISR(TIM1_OVF_vect) {  // timer1 overflow interrupt service routine
  cli(); //disable interrupt
  TCNT1 = 65406;

  // THIS PART IS USED TO UPDATE THE CHARLIEPLEXING LEDS MATRIX
  // YOU CAN JUST DON'T CARE ABOUT THIS PART
  // BECAUSE YOU CAN CODE LIKE A STANDARD MATRIX BY MANIPULATING THE
  // VALUE OF THE matrixState MATRIX

  //check from matrixState which LED to turn ON or OFF
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      if (matrixState[i][j] == 1) { //turn on LED with 1 in matrixState
        pinMode(pins[connectionMatrix[i][j][0]], OUTPUT); //set positive pole to OUTPUT
        pinMode(pins[connectionMatrix[i][j][1]], OUTPUT); //set negative pole to OUTPUT
        digitalWrite(pins[connectionMatrix[i][j][0]], HIGH); //set positive pole to HIGH
        digitalWrite(pins[connectionMatrix[i][j][1]], LOW); //set negative pole to LOW
        delayMicroseconds(250);
        pinMode(pins[connectionMatrix[i][j][0]], INPUT); //set both positive pole and negative pole
        pinMode(pins[connectionMatrix[i][j][1]], INPUT); // to INPUT in order to turn OFF the LED
      }
      if (matrixState[i][j] == 0) { //turn off LED with 0 in matrixState
        pinMode(pins[connectionMatrix[i][j][0]], INPUT); //set both positive pole and negative pole
        pinMode(pins[connectionMatrix[i][j][1]], INPUT); // to INPUT in order to turn OFF the LED
      }
    }
  }
  sei(); //enable interrupt
}

ISR(PCINT0_vect) { //BUTTON A INTERRUPT
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
  TCCR1B = 0;    // set entire TCCR1B register to 0

  // enable Timer1 overflow interrupt:
  bitSet(TIMSK1, TOIE1);

  // preload timer 65536 - (8000000 / 1024 / 60)
  TCNT1 = 65406;

  // set 1024 prescaler
  bitSet(TCCR1B, CS12);
  bitSet(TCCR1B, CS10);

  bitSet(GIMSK, PCIE0); //enable pingChange global interrupt

  //disabling all unnecessary peripherals to reduce power
  ADCSRA &= ~bit(ADEN); //disable ADC
  power_adc_disable(); // disable ADC converter
  power_usi_disable(); // disable USI
  // enable global interrupts:
  sei();

  clearMatrix(); //clear the Matrix

}

void loop() {
  updateManPosition();
}

void updateManPosition() {
  //depends on which button is pressed, change the man position
  // to left (button A) or right (button B)
  int manXNewPosition = manXPosition; //store man X position that is going to change
  
  //if we press the button B we go Right
  if (!digitalRead(BUTTON_B)) {
    delay(keySensibility);
    if (!digitalRead(BUTTON_B)) {
      manXNewPosition++;
    }
  }
  
  //if we press the button A we go Left
  if (!digitalRead(BUTTON_A)) {
    delay(keySensibility);
    if (!digitalRead(BUTTON_A)) {
      manXNewPosition--;
    }
  }

  //fix man X position
  if (manXNewPosition > MATRIX_COL - 1) {
    manXNewPosition--;
  }
  if (manXNewPosition < 0) {
    manXNewPosition = 0;
  }
  //only if the man position is different
  // (means that the button was pressed)
  if (manXNewPosition != manXPosition) {
    //delete current man Position
    matrixState[MATRIX_ROW - 1][manXPosition] = 0;
  }

  //set current man position to new position
  manXPosition = manXNewPosition;
  //show new man Position
  matrixState[MATRIX_ROW - 1][manXPosition] = 1;
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
      matrixState[i][j] = 1;
    }
  }
}

void goSleep() {
  //going sleep to reduce power consuming
  //enable interrupt buttons to allow wakeup from button interrupts
  bitSet(GIMSK, INT0); //enable interrupt pin 8 - button B - INT0
  bitSet(PCMSK0, PCINT6); //enable interrupt pin 6 - button A - PCINT6
  power_timer0_disable(); //disable Timer 0
  power_timer1_disable(); //disable Timer 1
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  while (digitalRead(BUTTON_B) || digitalRead(BUTTON_A)) { //until all the two buttons are pressend
    sleep_mode();
  }
  //disable interrupt buttons after sleep
  bitClear(GIMSK, INT0); //disable interrupt pin 8 - button B - INT0
  bitClear(PCMSK0, PCINT6); //disable interrupt pin 6 - button A - PCINT6
  power_timer0_enable(); //enable Timer 0
  power_timer1_enable(); //enable Timer 1
}

