/*****************************************************************************
   WALLED GAME FOR KEYCHAININO USB www.keychainino.com

   created by Alessandro Matera

   naming by *Don Marco Furi*

   The goal of this game is to move the "man" (a dot) inside the walls's gates
   that coming down from the top of the screen.

   The walls speed increase every 10 walls spawned
 * *****************************************************************************
*/
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

#define MATRIX_ROW 5
#define MATRIX_COL 6
#define PIN_NUMBER 7

#define BUTTON_A 2 // INT1
#define BUTTON_B 3 // INT0

unsigned long timer = 64900;

byte i_Charlie = 0;
byte j_Charlie = 0;

const byte pins[PIN_NUMBER] = {4, 5, 6, 7, 8, 9, 10}; //the number of the pin used for the LEDs in ordered

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


//MAN VARIABLES
byte manXPosition = 2;//the position of the man

//Wall variables
byte wallGateXPosition = random(0, MATRIX_COL); //randomized wall's gate
byte wallYPosition = 0; //Y value of the wall
unsigned int wallCounter = 0; //the number of walls spawned used to increase the speed

const int wallSpeed = 2500; //this number is inversely proportional to the speed of the wall
int wallUpdatePositionCounter = 0; //it is a counter to update wall position
int wallUpdatePositionSpeed = wallSpeed; //the actual speed of the wall

byte keySensibility = 70; //the sensibility of the two touch buttons. Decrease to have more sensitivity

//the game score calculated in the number of gates passed
byte score = 0; //0 //MAX 255 for byte

bool gameStarted = false; //indicates if the game is started

// KeyChainino Face stored in FLASH in order to reduce RAM size

//KeyChainino Face stored in FLASH in order to reduce RAM size
const PROGMEM bool KeyChaininoFace[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 0, 0, 0},
  {0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {1, 0, 0, 0, 0, 1},
  {0, 1, 1, 1, 1, 0}
};

//NUMBERS used in score stored in FLASH in order to reduce RAM size

const PROGMEM bool one[MATRIX_ROW][MATRIX_COL]  = {
  {0, 0, 0, 1, 0, 0},
  {0, 0, 1, 1, 0, 0},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 0, 1, 0, 0}
};

const PROGMEM bool two[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 1, 0},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 1, 0, 0, 0},
  {0, 0, 1, 1, 1, 0}
};

const PROGMEM bool three[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 1, 0},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 0, 0, 1, 0},
  {0, 0, 1, 1, 0, 0}
};

const PROGMEM bool four[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 0, 1, 0},
  {0, 0, 0, 1, 1, 0},
  {0, 0, 1, 0, 1, 0},
  {0, 1, 1, 1, 1, 1},
  {0, 0, 0, 0, 1, 0}
};

const PROGMEM bool five[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 1, 1, 1, 0},
  {0, 0, 1, 0, 0, 0},
  {0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 1, 0},
  {0, 0, 1, 1, 0, 0}
};

const PROGMEM bool six[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 1, 0, 0},
  {0, 0, 1, 0, 0, 0},
  {0, 1, 1, 1, 0, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 0, 1, 1, 0, 0}
};

const PROGMEM bool seven[MATRIX_ROW][MATRIX_COL] = {
  {0, 1, 1, 1, 1, 0},
  {0, 0, 0, 0, 1, 0},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 1, 0, 0, 0},
  {0, 1, 0, 0, 0, 0}
};

const PROGMEM bool eight[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 1, 1, 0, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 0, 1, 1, 0, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 0, 1, 1, 0, 0}
};

const PROGMEM bool nine[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 1, 1, 0, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 0, 1, 1, 1, 0},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 1, 0, 0, 0}
};

const PROGMEM bool zero[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 1, 1, 0, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 0, 1, 1, 0, 0}
};

ISR(TIMER1_OVF_vect) {  // timer1 overflow interrupt service routine
  cli(); //disable interrupt
  TCNT1 = timer;// 65405;

  //THIS PART IS USED TO UPDATE THE Wall'S MOVIMENT IN THE GAME
  //if game is started change wall position
  if (gameStarted) {
    //it is a counter used to update the wall position after it reach the wallUpdatePositionSpeed value
    //becouse timer interrupt is to fast
    wallUpdatePositionCounter++;
    if (wallUpdatePositionCounter > wallUpdatePositionSpeed) {
      updateWallPosition();
      wallUpdatePositionCounter = 0;
    }
  }


  // THIS PART IS USED TO UPDATE THE CHARLIEPLEXING LEDS MATRIX
  // YOU CAN JUST DON'T CARE ABOUT THIS PART
  // BECAUSE YOU CAN CODE LIKE A STANDARD MATRIX BY MANIPULATING THE
  // VALUE OF THE matrixState MATRIX

  //check from matrixState which LED to turn ON or OFF

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

  //show wall
  for (byte x = 0; x < MATRIX_COL; x++) {
    if (x != wallGateXPosition) {
      matrixState[wallYPosition][x] = 1;
    }
  }

  gameStarted = true; //Start the game

}

void loop() {
  if (gameStarted) { //if game is started
    game(); //go to game function
  } else { //else end the game
    endGame(); //to to end game
  }
}

void game() {
  updateManPosition(); //update the man position by checking buttons
}

void endGame() {
  //SHOW SCORE
  showScore(score); //shows the score number
  clearMatrix(); //clear all the LEDs
  showKeyChaininoFace(); //show KeyChaininoFace
  delay(500);
  goSleep(); //sleep to reduce power
  resetGame(); //reset game variables
}

void updateWallPosition() {

  //checkCollision
  if (wallYPosition == MATRIX_ROW - 2) { //if wall is in the same man's Y position
    //NOTE: we check if the wallYposition is in the position MATRIX_ROW - 2 and
    // not on MATRIX_ROW - 1 because we increment the wallYPosition after this check
    if (manXPosition == wallGateXPosition) { //if man passes the wall
      score++; //increase score
    } else {
      gameStarted = false; //esle we end the game
    }
  }

  int wallOldYPosition = wallYPosition; //store the old wall position

  if (wallYPosition < MATRIX_ROW - 1) { //if wall position isn't the bottom
    wallYPosition++; //increase Y position - go down
  } else { //else, if the wall touch the bottom
    wallYPosition = 0; //reset its position
    wallGateXPosition = random(0, MATRIX_COL); //randomize another gate
    wallCounter++; //increase wall counter
    //increase wall spaw update
    if (wallCounter % 10 == 0) { //if wallCounter is egual to 10
      wallUpdatePositionSpeed -= 100; //we can increase the speed of the wall
      if (wallUpdatePositionSpeed < 7) { //by decreasing the wallUpdatePositionSpeed variable
        wallUpdatePositionSpeed = 7; //if this variable is under 7, we stay at this speed.
      }
    }
  }

  //delete previous wall position
  for (byte x = 0; x < MATRIX_COL; x++) {
    matrixState[wallOldYPosition][x] = 0;
  }

  //show new wall position
  for (byte x = 0; x < MATRIX_COL; x++) {
    if (x != wallGateXPosition) {
      matrixState[wallYPosition][x] = 1;
    }
  }


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


void showScore(byte scoreNumber) {

  clearMatrix();

  char scoreChar[5]; //char were to put the score number

  //converting the score to scoreChar
  String str = String(scoreNumber) + ' ';
  str.toCharArray(scoreChar, 5);

  for (char c = 0; scoreChar[c] != '\0'; c++) {
    for (int col = MATRIX_COL - 1; col >= 0; col--) { // we start to display the charter matrix from right to left
      for (byte i = 0; i < MATRIX_COL; i++) { //put the charter into the matrixState
        for (byte j = 0; j < MATRIX_ROW; j++) { //as usual
          if (i >= col) { //if the number of col(i) is higher than the scrolling col, we show the correct charter according to charterToShow var.
            writeCharter(scoreChar[c], i, j, col);
          } else { //else, if col (i) is less than col, we shift the matrixState
            matrixState[j][i] = matrixState[j][i + 1];
          }
        }

      }
      delay(100);
    }
  }
}

//show the number according to the matrix number
void writeCharter(char charterToShow, byte i, byte j, byte col) {
  if (charterToShow == '0') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(zero[j][i - col]));
  }
  else if (charterToShow == '1') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(one[j][i - col]));
  }
  else if (charterToShow == '2') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(two[j][i - col]));
  }
  else if (charterToShow == '3') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(three[j][i - col]));
  }
  else if (charterToShow == '4') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(four[j][i - col]));
  }
  else if (charterToShow == '5') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(five[j][i - col]));
  }
  else if (charterToShow == '6') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(six[j][i - col]));
  }
  else if (charterToShow == '7') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(seven[j][i - col]));
  }
  else if (charterToShow == '8') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(eight[j][i - col]));
  }
  else if (charterToShow == '9') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(nine[j][i - col]));
  }
  else if (charterToShow == ' ') { //SYMBOLS FOR SPACE
    matrixState[j][i] = 0;
  }
}

void resetGame() {
  //reset all game variables to the start condition
  clearMatrix();
  showKeyChaininoFace();
  delay(500);
  clearMatrix();
  delay(300);

  //check for programming mode
  if (!digitalRead(BUTTON_B) && !digitalRead(BUTTON_A)) {
    //programming mode
    programmingMode();
  }

  manXPosition = 2;

  wallGateXPosition = random(0, MATRIX_COL);
  wallYPosition = 0; //top screen
  wallCounter = 0; //the number of walls spawned

  wallUpdatePositionCounter = 0;
  wallUpdatePositionSpeed = wallSpeed;

  score = 0;

  //show wall
  for (byte x = 0; x < MATRIX_COL; x++) {
    if (x != wallGateXPosition) {
      matrixState[wallYPosition][x] = 1;
    }
  }

  gameStarted = true;
}

void programmingMode() {
  while (1) {
    fullMatrix();
    delay(300);
    clearMatrix();
    delay(300);
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

  bool falsePush = true;

  while (falsePush) {
    if (!digitalRead(BUTTON_B) && !digitalRead(BUTTON_A)) { //until all the two buttons are pressend
      power_timer0_enable(); //enable Timer 0 in order to enable delay() function
      delay(1000);
      if (!digitalRead(BUTTON_B) && !digitalRead(BUTTON_A)) {
        falsePush = false;
      } else {
        power_timer0_disable();
        sleep_mode();
      }
    } else {
      sleep_mode();
    }
  }

  //disable interrupt buttons after sleep
  bitClear(EIMSK, INT0); //enable interrupt button B - INT0
  bitClear(EIMSK, INT1); //enable interrupt button A - INT1

  power_timer0_enable(); //enable Timer 0
  power_timer1_enable(); //enable Timer 1

  power_usb_enable();

  USBDevice.attach();
  // delay(100);

}

