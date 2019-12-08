/*************************************************************************
   SNAKE GAME FOR KEYCHAININO www.keychainino.com

  The aim of the game is moving the snake around the LED display trying to eat the food that blinks.
  Use the two buttons in order to change the snake's head direction clockwise or counter clockwise.
  Pay attention to the other snake's pieces:
  Every two foods eaten, the snake increases itself!
  Don't eat yourself! :D

  created by Alessandro Matera

  14/05/16

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
//game variables

const byte MAXSNAKEPIECES = 8; //max number of snake pieces

//snake pieces' position. Each piece has two variable Y and X. If its values = -1 means that the piece doesn't exist yet.
byte snakePosition[MAXSNAKEPIECES][2] = {
  {3, 4}, {3, 3}, {3, 2}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
};

byte food[2] = {4, 6};

byte numSnakePieces = 3;

//Snake's Head Direction Y, X
//Y: 0 = STOP, -1 = UP, 1 = DOWN
//X: 0 = STOP, 1 = RIGHT, -1 = LEFT
int snakeDirection[2] = {0, 1};

const byte snakeSpeed = 15; //this number is inversely proportional to the speed of the snake
byte snakeUpdatePositionCounter = 0; //it is a counter to update wall position
byte snakeUpdatePositionSpeed = snakeSpeed; //the actual speed of the snake

//variable used to make the food blink
byte foodUpdateFlashingCounter = 0;
byte foodBlink = true;

//the game score calculated in the number of food eaten
int score = 0; //MAX 255 for byte

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

  //THIS PART IS USED TO UPDATE THE Snake'S MOVIMENT IN THE GAME

  //if game is started change snake position
  if (gameStarted) {
    //it is a counter used to update the snake position after it reach the snakeUpdatePositionSpeed value
    //becouse timer interrupt is to fast
    snakeUpdatePositionCounter++;
    if (snakeUpdatePositionCounter > snakeUpdatePositionSpeed) {
      updateSnakePosition();
      snakeUpdatePositionCounter = 0;
    }

    //food blink counter
    foodUpdateFlashingCounter++;
    if (foodUpdateFlashingCounter > 5) {
      foodBlink = !foodBlink;
      foodUpdateFlashingCounter = 0;
    }
    if (foodBlink) {
      //set food position
      setMatrixStateBit(food[0], food[1]);
    } else {
      //clear food position
      clearMatrixStateBit(food[0], food[1]);
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

  resetGame(); //initialize the game variables

}

void loop() {
  if (gameStarted) { //if game is started
    game(); //go to game function
  } else { //else end the game
    endGame(); //to to end game
  }
}

void game() {
  changeSnakePosition();
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

void changeSnakePosition() {

  //when a button is pressed we change the snake's head direction.
  //if button A is pressed, we chance the direction to CCW = Counter Clockwise
  //if button B is pressed, we chance the direction to CW = Clockwise

  if (!digitalRead(BUTTON_A)) {//CCW
    delay(30);
    if (!digitalRead(BUTTON_A)) {
      if (snakeDirection[0] == -1) { //snake is going UP, we go LEFT
        snakeDirection[0] = 0;
        snakeDirection[1] = -1;
      }
      else if (snakeDirection[0] == 1) { //snake is going DOWN, we go RIGHT
        snakeDirection[0] = 0;
        snakeDirection[1] = 1;
      }
      else if (snakeDirection[1] == 1) { //snake is going RIGHT, we go UP
        snakeDirection[0] = -1;
        snakeDirection[1] = 0;
      }
      else if (snakeDirection[1] == -1) { //snake is going LEFT, we go DOWN
        snakeDirection[0] = 1;
        snakeDirection[1] = 0;
      }
      while (!digitalRead(BUTTON_A)); //while the button is pressed we do nothing.
    }
  }

  if (!digitalRead(BUTTON_B)) {//CW
    delay(30);
    if (!digitalRead(BUTTON_B)) {
      if (snakeDirection[0] == -1) { //snake is going UP, we go RIGHT
        snakeDirection[0] = 0;
        snakeDirection[1] = 1;
      }
      else if (snakeDirection[0] == 1) { //snake is going DOWN, we go LEFT
        snakeDirection[0] = 0;
        snakeDirection[1] = -1;
      }
      else if (snakeDirection[1] == 1) { //snake is going RIGHT, we go DOWN
        snakeDirection[0] = 1;
        snakeDirection[1] = 0;
      }
      else if (snakeDirection[1] == -1) { //snake is going LEFT, we go UP
        snakeDirection[0] = -1;
        snakeDirection[1] = 0;
      }

      while (!digitalRead(BUTTON_B)); //while the button is pressed we do nothing.
    }
  }


}

void updateSnakePosition() {

  int newSnakePosition[MAXSNAKEPIECES][2]; //variable that contains the new calcolated snake's position

  //update snake's head position according to the snakeDirection
  newSnakePosition[0][0] = snakePosition[0][0] + snakeDirection[0];

  //adjust head position
  if (newSnakePosition[0][0] > MATRIX_ROW - 1) {
    newSnakePosition[0][0] = 0;
  }
  if (newSnakePosition[0][0] < 0) {
    newSnakePosition[0][0] = MATRIX_ROW - 1;
  }
  newSnakePosition[0][1] = snakePosition[0][1] + snakeDirection[1];
  if (newSnakePosition[0][1] > MATRIX_COL - 1) {
    newSnakePosition[0][1] = 0;
  }
  if (newSnakePosition[0][1] < 0) {
    newSnakePosition[0][1] = MATRIX_COL - 1;
  }

  //check if snake's head collide with the others snake's pieces. If it is, we end the game.
  for (byte i = 1; i < numSnakePieces; i++) {
    if ((newSnakePosition[0][0] == snakePosition[i][0]) && (newSnakePosition[0][1] == snakePosition[i][1])) {
      gameStarted = false; //endGame
    }
  }


  //check if snake collide with food
  if ((newSnakePosition[0][0] == food[0]) && (newSnakePosition[0][1] == food[1])) {

    //clear food position
    clearMatrixStateBit(food[0], food[1]);

    score++; //increse score

    if (score % 2 == 0) { //every two food eaten, increase snake's pieces
      numSnakePieces++;
    }

    if (numSnakePieces > MAXSNAKEPIECES) { //if max pieces rieched, set pieces to max.
      numSnakePieces = MAXSNAKEPIECES;
    }


    //randomize new food position
    food[0] = random(0, MATRIX_ROW);
    food[1] = random(0, MATRIX_COL);
    //if food position is in the same position of the snake, change position
    for (byte i = 0; i < numSnakePieces; i++) {
      if (food[0] == newSnakePosition[i][0] && food[1] == newSnakePosition[i][1] || food[0] == snakePosition[i][0] && food[1] == snakePosition[i][1]) {
        food[0] = random(0, MATRIX_ROW);
        food[1] = random(0, MATRIX_COL);
        i = 0;
      }
    }
  }

  //shift the other snake's pieces according to the previous pieces.
  for (byte i = 1; i < numSnakePieces; i++) {
    newSnakePosition[i][0] = snakePosition[i - 1][0];

    //adjust snake's piece position
    if (newSnakePosition[i][0] > MATRIX_ROW - 1) {
      newSnakePosition[i][0] = 0;
    }
    if (newSnakePosition[i][0] < 0) {
      newSnakePosition[i][0] = MATRIX_ROW - 1;
    }
    newSnakePosition[i][1] = snakePosition[i - 1][1];
    if (newSnakePosition[i][1] > MATRIX_COL - 1) {
      newSnakePosition[i][1] = 0;
    }
    if (newSnakePosition[i][1] < 0) {
      newSnakePosition[i][1] = MATRIX_COL - 1;
    }
  }


  //delete current snake Position

  for (byte i = 0; i < numSnakePieces; i++) {
    clearMatrixStateBit(snakePosition[i][0], snakePosition[i][1]); //clear
  }

  //set current snake Position
  for (byte i = 0; i < numSnakePieces; i++) {
    snakePosition[i][0] = newSnakePosition[i][0];
    snakePosition[i][1] = newSnakePosition[i][1];
    setMatrixStateBit(snakePosition[i][0], snakePosition[i][1]); //set
  }

  //set new food position
  setMatrixStateBit(food[0], food[1]);
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

  snakePosition[0][0] = 3;
  snakePosition[0][1] = 4;
  snakePosition[1][0] = 3;
  snakePosition[1][1] = 3;
  snakePosition[2][0] = 3;
  snakePosition[2][1] = 2;

  numSnakePieces = 3;
  snakeDirection[0] = 0;
  snakeDirection[1] = 1;

  //randomize food position
  food[0] = random(0, MATRIX_ROW);
  food[1] = random(0, MATRIX_COL);
  //if food position is in the same position of the snake, change position
  for (byte i = 0; i < numSnakePieces; i++) {
    if (food[0] == snakePosition[i][0] && food[1] == snakePosition[i][1]) {
      food[0] = random(0, MATRIX_ROW);
      food[1] = random(0, MATRIX_COL);
      i = 0;
    }
  }

  matrixState[food[0]][food[1]] = 1; //Set food position

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

void showKeyChaininoFace() {
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(KeyChaininoFace[i][j])); //here we read the matrix from FLASH
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

