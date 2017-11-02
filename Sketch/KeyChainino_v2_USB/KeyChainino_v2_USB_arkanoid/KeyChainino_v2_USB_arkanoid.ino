/*************************************************************************
   ARKANOID GAME FOR KEYCHAININO V2 USB www.keychainino.com

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

//BAR VARIABLES
byte barCurrentPosition[2][2] = { //the current bar position
  {7, 3},
  {7, 4}
};

byte barNewPosition[2][2] = { //the new bar position. initialized as current position
  {7, 3},
  {7, 4}
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

int ballUpdatePositionCounter = 0; //it is a counter to update ball position
const int ballUpdatePositionCONSTANT = 1400;; //this number is directly proportional to the speed of the ball

//the game score calculated in the number of collision between bar and ball
byte score = 0; //0 //MAX 255 for byte

bool gameStarted = false; //indicates if the game is started

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


//NUMBERS used in score stored in FLASH in order to reduce RAM size

const PROGMEM bool one[MATRIX_ROW][MATRIX_COL]  = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 1, 0, 0, 0},
  {0, 0, 1, 0, 1, 0, 0, 0},
  {0, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 0, 1, 0, 0, 0}
};

const PROGMEM bool two[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 1, 1, 0, 0},
  {0, 0, 1, 0, 0, 0, 1, 0},
  {0, 0, 0, 0, 0, 0, 1, 0},
  {0, 0, 0, 0, 0, 1, 0, 0},
  {0, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0},
  {0, 0, 1, 1, 1, 1, 1, 0}
};

const PROGMEM bool three[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 1, 0},
  {0, 0, 0, 0, 0, 0, 1, 0},
  {0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 1, 0},
  {0, 0, 0, 0, 0, 0, 1, 0},
  {0, 0, 0, 1, 1, 1, 0, 0}
};

const PROGMEM bool four[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 1, 0, 0},
  {0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 0, 1, 0, 1, 0, 0},
  {0, 0, 1, 0, 0, 1, 0, 0},
  {0, 1, 1, 1, 1, 1, 1, 0},
  {0, 0, 0, 0, 0, 1, 0, 0},
  {0, 0, 0, 0, 0, 1, 0, 0}
};

const PROGMEM bool five[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 1, 1, 1, 0},
  {0, 0, 0, 1, 0, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0},
  {0, 0, 0, 1, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 1, 0},
  {0, 0, 0, 0, 0, 0, 1, 0},
  {0, 0, 0, 1, 1, 1, 0, 0}
};

const PROGMEM bool six[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 1, 0, 0},
  {0, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0},
  {0, 0, 1, 1, 1, 1, 0, 0},
  {0, 0, 1, 0, 0, 0, 1, 0},
  {0, 0, 1, 0, 0, 0, 1, 0},
  {0, 0, 0, 1, 1, 1, 0, 0}
};

const PROGMEM bool seven[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 1, 1, 1, 1, 1, 0},
  {0, 0, 0, 0, 0, 0, 1, 0},
  {0, 0, 0, 0, 0, 0, 1, 0},
  {0, 0, 0, 0, 0, 1, 0, 0},
  {0, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0},
  {0, 0, 1, 0, 0, 0, 0, 0}
};

const PROGMEM bool eight[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 1, 1, 0, 0},
  {0, 0, 1, 0, 0, 0, 1, 0},
  {0, 0, 1, 0, 0, 0, 1, 0},
  {0, 0, 0, 1, 1, 1, 0, 0},
  {0, 0, 1, 0, 0, 0, 1, 0},
  {0, 0, 1, 0, 0, 0, 1, 0},
  {0, 0, 0, 1, 1, 1, 0, 0}
};

const PROGMEM bool nine[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 1, 1, 0, 0},
  {0, 0, 1, 0, 0, 0, 1, 0},
  {0, 0, 1, 0, 0, 0, 1, 0},
  {0, 0, 0, 1, 1, 1, 1, 0},
  {0, 0, 0, 0, 0, 1, 0, 0},
  {0, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0}
};
const PROGMEM bool zero[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 1, 1, 0, 0},
  {0, 0, 1, 0, 0, 0, 1, 0},
  {0, 0, 1, 0, 0, 0, 1, 0},
  {0, 0, 1, 0, 0, 0, 1, 0},
  {0, 0, 1, 0, 0, 0, 1, 0},
  {0, 0, 1, 0, 0, 0, 1, 0},
  {0, 0, 0, 1, 1, 1, 0, 0}
};




ISR(TIMER1_OVF_vect) {  // timer1 overflow interrupt service routine
  cli(); //disable interrupt
  TCNT1 = timer;// 65405;

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

  /*showKeyChaininoFace(); //show KeyChainino smile face
    delay(500);
    clearMatrix(); //clear the Matrix
    gameStarted = true; //Start the game
  */


  goSleep();
  resetGame();

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
  matrixState[ballCurrentPosition[0]][ballCurrentPosition[1]] = 0;
  //set current bar position to new position
  ballCurrentPosition[0] = ballNewPosition[0];
  ballCurrentPosition[1] = ballNewPosition[1];
  //show new bar Position
  matrixState[ballNewPosition[0]][ballNewPosition[1]] = 1;
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
    matrixState[barCurrentPosition[0][0]][barCurrentPosition[0][1]] = 0;
    matrixState[barCurrentPosition[1][0]][barCurrentPosition[1][1]] = 0;
  }
  //set current bar position to new position
  barCurrentPosition[0][0] = barNewPosition[0][0];
  barCurrentPosition[0][1] = barNewPosition[0][1];
  barCurrentPosition[1][0] = barNewPosition[1][0];
  barCurrentPosition[1][1] = barNewPosition[1][1];

  //show new bar Position
  matrixState[barNewPosition[0][0]][barNewPosition[0][1]] = 1;
  matrixState[barNewPosition[1][0]][barNewPosition[1][1]] = 1;
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
      delay(80);
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
  delay(1000);
  clearMatrix();
  delay(300);

  //check for programming mode
  if (!digitalRead(BUTTON_B) && !digitalRead(BUTTON_A)) {
    //programming mode
    programmingMode();
  }


  barCurrentPosition[0][0] = 0;
  barCurrentPosition[0][1] = 0;
  barCurrentPosition[1][0] = 0;
  barCurrentPosition[1][1] = 0;

  barNewPosition[0][0] = 7;
  barNewPosition[0][1] = 3;
  barNewPosition[1][0] = 7;
  barNewPosition[1][1] = 4;

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

  /*while (digitalRead(BUTTON_B) || digitalRead(BUTTON_A)) { //until all the two buttons are pressend
    sleep_mode();
    }*/

  while (falsePush) {
    if (!digitalRead(BUTTON_B) && !digitalRead(BUTTON_A)) { //until all the two buttons are pressend
      power_timer0_enable(); //enable Timer 0 in order to enable delay() function
      delay(2000);
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

