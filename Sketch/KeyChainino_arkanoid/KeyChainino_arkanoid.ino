/*************************************************************************
   ARKANOID GAME FOR KEYCHAININO www.keychainino.com

   created by Alessandro Matera
 * ************************************************************************
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

//BAR VARIABLES
byte barCurrentPosition[2][2] = { //the current bar position
  {4, 2},
  {4, 3}
};

byte barNewPosition[2][2] = { //the new bar position. initialized as current position
  {4, 2},
  {4, 3}
};

int barX1 = barCurrentPosition[0][1]; //variable that indicates the position of the first bar DOT
int barY1 = barCurrentPosition[0][0]; // position as X and Y
int barX2 = barCurrentPosition[1][1]; //variable that indicates the position of the second bar DOT
int barY2 = barCurrentPosition[1][0]; // position as X and Y

//BALL VARIABLES
int ballCurrentPosition[2] = {0, random(0, MATRIX_COL)}; //the randomized ball position
int ballNewPosition[2]; //the new Ball position

int ballX = ballCurrentPosition[1]; //variable that indicates the position of the first ball DOT
int ballY = ballCurrentPosition[0]; //position as X and Y

//Direction
//Y: 0 = STOP, -1 = UP, 1 = down
//X: 0 = STOP, 1 = RIGHT, -1 = LEFT
int ballDirection[2] = {1, 0}; //Y, X  indicates the direction where the ball is going

byte ballUpdatePositionCounter = 0; //it is a counter to update ball position
const byte ballUpdatePositionCONSTANT = 12; //this number is directly proportional to the speed of the ball

//the game score calculated in the number of collision between bar and ball
int score = 0;

bool gameStarted = false; //indicates if the game is started

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



ISR(TIM1_OVF_vect) {  // timer1 overflow interrupt service routine
  cli(); //disable interrupt
  TCNT1 = 65405;

  //THIS PART IS USED TO UPDATE THE BALL'S MOVIMENT IN THE GAME
  //if game is started change ball position
  if (gameStarted) {
    //it is a counter to update ball after counter reset becouse timer interrupt is to fast
    ballUpdatePositionCounter++;
    if (ballUpdatePositionCounter > ballUpdatePositionCONSTANT) {
      updateBallPosition();
      ballUpdatePositionCounter = 0;
    }

  }


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
        delayMicroseconds(250); //250
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
  TCCR1B = 0;    // set entire TCCR1A register to 0

  // enable Timer1 overflow interrupt:
  TIMSK1 |= (1 << TOIE1);

  // preload timer 65536 - (8000000 / 1024 / 60) = 60Hz
  TCNT1 = 65405;

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

  showKeyChaininoFace(); //show KeyChainino smile face
  delay(500);
  clearMatrix(); //clear the Matrix
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
  updateBarPosition(); //update the bar position by checking buttons
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

void updateBallPosition() {
  //change ball position depending on the ball direction
  ballY = ballCurrentPosition[0] + ballDirection[0];
  ballX = ballCurrentPosition[1] + ballDirection[1];

  //checkCollision
  if (ballY >= (MATRIX_ROW - 2) && (ballX == barCurrentPosition[0][1] || ballX == barCurrentPosition[1][1])) { // ball touched bottom
    ballY = MATRIX_ROW - 2;
    if (ballX >= MATRIX_COL - 1) {
      ballX = MATRIX_COL - 1;
    }
    if (ballX <= 0) {
      ballX = 0;
    }
    if (ballX == barCurrentPosition[0][1] || ballX == barCurrentPosition[1][1]) { //ball touched bar
      ballDirection[0] = -1;
      ballDirection[1] = random(-1, 2);
      score++;
    }
  }

  else if (ballY >= MATRIX_ROW - 1) { // ball is getting touch bottom and bar
    ballY = MATRIX_ROW - 1;
    if (ballX >= MATRIX_COL - 1) {
      ballX = MATRIX_COL - 1;
    }
    if (ballX <= 0) {
      ballX = 0;
    }
    if (ballX == barCurrentPosition[0][1] || ballX == barCurrentPosition[1][1]) { //ball touched bar
      ballDirection[0] = -1;
      ballDirection[1] = random(-1, 2);
      score++;
    } else { //ball touched bottom = END
      //END GAME
      ballDirection[0] = 0;
      ballDirection[1] = 0;
      gameStarted = false;
    }

  }

  else if (ballX >= (MATRIX_COL - 1) && ballY <= 0) { //ball touched right & top
    ballX = MATRIX_COL - 1;
    ballY = 0;
    ballDirection[0] = 1;
    ballDirection[1] = -1;
  }

  else if (ballX <= 0 && ballY <= 0) { //ball touched left & top
    ballX = 0;
    ballY = 0;
    ballDirection[0] = 1;
    ballDirection[1] = 1;
  }

  else if (ballY <= 0) { //ball touch top
    ballY = 0;
    if (ballX >= (MATRIX_COL - 1)) {
      ballX = MATRIX_COL - 1;
    }
    if (ballX <= 0) {
      ballX = 0;
    }
    ballDirection[0] = 1;
  }

  else if (ballX >= (MATRIX_COL - 1)) { //ball touched right
    ballX = MATRIX_COL - 1;
    if (ballY >= MATRIX_ROW - 1) {
      ballY = MATRIX_ROW - 1;
    }
    if (ballY <= 0) {
      ballY = 0 ;
    }
    ballDirection[1] = -1;

  }
  else if (ballX <= 0) { //ball touched left
    ballX = 0;
    if (ballY >= (MATRIX_ROW - 1)) {
      ballY = MATRIX_ROW - 1;
    }
    if (ballY <= 0) {
      ballY = 0;
    }
    ballDirection[1] = 1;
  }

  //update position
  ballNewPosition[0] = ballY;
  ballNewPosition[1] = ballX;
  //delete current ball Position
  clearMatrixStateBit(ballCurrentPosition[0], ballCurrentPosition[1]);
  //set current bar position to new position
  ballCurrentPosition[0] = ballNewPosition[0];
  ballCurrentPosition[1] = ballNewPosition[1];
  //show new bar Position
  setMatrixStateBit(ballNewPosition[0], ballNewPosition[1]);
}

void updateBarPosition() {

  //depends on which button is pressed, change the bar position
  // to left (button A) or right (button B)

  if (!digitalRead(BUTTON_B)) {
    delay(80);
    if (!digitalRead(BUTTON_B)) {
      barX1++;
      barX2++;
    }
  }

  if (!digitalRead(BUTTON_A)) {
    delay(80);
    if (!digitalRead(BUTTON_A)) {
      barX1--;
      barX2--;
    }
  }

  if (barX2 >= MATRIX_COL) {
    barX2--;
    barX1--;
  }
  if (barX2 == 0) {
    barX1++;
    barX2++;
  }

  //changing only X Ax
  barNewPosition[0][1] = barX1;
  barNewPosition[1][1] = barX2;

  //only if the bar position is different
  // (means that the button was pressed)
  if (barNewPosition[0][1] != barCurrentPosition[0][1]) {

    //delete current bar Position
    clearMatrixStateBit(barCurrentPosition[0][0], barCurrentPosition[0][1]);
    clearMatrixStateBit(barCurrentPosition[1][0], barCurrentPosition[1][1]);
  }
  //set current bar position to new position
  barCurrentPosition[0][0] = barNewPosition[0][0];
  barCurrentPosition[0][1] = barNewPosition[0][1];
  barCurrentPosition[1][0] = barNewPosition[1][0];
  barCurrentPosition[1][1] = barNewPosition[1][1];

  //show new bar Position
  setMatrixStateBit(barNewPosition[0][0], barNewPosition[0][1]);
  setMatrixStateBit(barNewPosition[1][0], barNewPosition[1][1]);
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
      delay(150);
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

  barCurrentPosition[0][0] = 0;
  barCurrentPosition[0][1] = 0;
  barCurrentPosition[1][0] = 0;
  barCurrentPosition[1][1] = 0;

  barNewPosition[0][0] = 4;
  barNewPosition[0][1] = 2;
  barNewPosition[1][0] = 4;
  barNewPosition[1][1] = 3;

  barX1 = barNewPosition[0][1];
  barY1 = barNewPosition[0][0];
  barX2 = barNewPosition[1][1];
  barY2 = barNewPosition[1][0];

  ballCurrentPosition[0] = 0;
  ballCurrentPosition[2] = random(0, MATRIX_COL);

  ballX = ballCurrentPosition[1];
  ballY = ballCurrentPosition[0];

  ballDirection[0] = 1;
  ballDirection[1] = 0;

  ballUpdatePositionCounter = 0;

  score = 0;

  gameStarted = true;
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

//here we set or clear a single bit on the matrixState. We use this funciton in order
//to really set or clear the matrix's bit when an interrupt occours. To do that we disable the 
//interrupt -> set or clear the bit -> enable interrupt

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

