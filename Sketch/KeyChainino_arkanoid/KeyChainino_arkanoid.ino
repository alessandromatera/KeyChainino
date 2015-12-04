/*************************************************************************
 * ARKANOID GAME FOR KEYCHAININO www.keychainino.com
 *
 * created by Alessandro Matera
 * ************************************************************************
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>

#define MATRIX_ROW 5
#define MATRIX_COL 6
#define PIN_NUMBER 7

#define BUTTON_A 6 //pin 6 - PCINT6
#define BUTTON_B 8 //pin 8 - INT0

const byte pins[PIN_NUMBER] = {0, 1, 2, 3, 7, 9, 10}; //the number of the pin used for the LEDs in ordered

const byte connectionMatrix[MATRIX_ROW][MATRIX_COL][2] = { //the matrix that show the LEDs pin connections. Firs Value is the Anode, second is the Catode
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
const byte ballUpdatePositionCONSTANT = 30; //this number is directly proportional to the speed of the ball

//the game score calculated in the number of collision between bar and ball
byte score = 0; //0 //MAX 255 for byte

bool gameStarted = true; //indicates if the game is started

//NUMBERS used in score stored in FLASH in order to reduce RAM size

const PROGMEM bool one[MATRIX_ROW][MATRIX_COL]  = {
  {0, 0, 0, 1, 0, 0},
  {0, 0, 1, 1, 0, 0},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 0, 1, 0, 0}
};

const PROGMEM bool two[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 1, 0, 0},
  {0, 0, 1, 0, 1, 0},
  {0, 0, 0, 0, 1, 0},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 1, 1, 1, 0}
};

const PROGMEM bool three[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 1, 1, 1, 0},
  {0, 0, 0, 0, 1, 0},
  {0, 0, 0, 1, 1, 0},
  {0, 0, 0, 0, 1, 0},
  {0, 0, 1, 1, 1, 0}
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
  {0, 0, 0, 1, 1, 0},
  {0, 0, 1, 0, 0, 0},
  {0, 0, 1, 1, 0, 0},
  {0, 0, 1, 0, 1, 0},
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
  {0, 1, 1, 1, 1, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 1, 1, 1, 0}
};



ISR(TIM1_OVF_vect) {  // timer1 overflow interrupt service routine
  cli(); //disable interrupt

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
        delayMicroseconds(400); //250
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

  // Set CS10 bit so timer runs at clock speed: (no prescaling)
  TCCR1B |= (1 << CS10);

  bitSet(GIMSK, PCIE0); //enable pingChange global interrupt

  //disabling all unnecessary peripherals to reduce power
  ADCSRA &= ~bit(ADEN); //disable ADC
  power_adc_disable(); // disable ADC converter
  power_usi_disable(); // disable USI
  // enable global interrupts:
  sei();
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
  goSleep(); //sleep to reduce power
  resetGame(); //reset game variables
}

void updateBallPosition() {
  //change ball position depending on the ball direction
  ballY = ballCurrentPosition[0] + ballDirection[0];
  ballX = ballCurrentPosition[1] + ballDirection[1];

  //checkCollision
  if (ballY >= MATRIX_ROW - 1) { // ball touched bottom
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
  else if (ballY <= 0) { //ball touch top
    ballY = 0;
    if (ballX >= MATRIX_COL - 1) {
      ballX = MATRIX_COL - 1;
    }
    if (ballX <= 0) {
      ballX = 0;
    }
    ballDirection[0] = 1;
    ballDirection[1] = random(-1, 2);
  }
  else if (ballX >= MATRIX_COL) { //ball touched right
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
    //Serial.println("ball touched left");
    ballX = 0;
    if (ballY >= MATRIX_ROW - 1) {
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
    delay(100);
    if (!digitalRead(BUTTON_B)) {
      barX1++;
      barX2++;
    }
  }

  if (!digitalRead(BUTTON_A)) {
    delay(100);
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
  //if the score number is less than 10
  // show this number
  if (scoreNumber < 10) {
    showNumber(scoreNumber);
  } else { //if the score number is higher than 9
    int scoreDecimal = scoreNumber / 10;
    showNumber(scoreDecimal); //show the first decimal number
    delay(1500);
    clearMatrix(); //clear all LEDs
    delay(100);
    int scoreSecond = scoreNumber - (scoreDecimal * 10);
    showNumber(scoreSecond); //show the second number
  }
  delay(1500);
}

//show the number according to the matrix number
void showNumber(byte number) {

  //according to the number to show we populate the matrixState
  // with the relative matrix, in this for-cycles.
  switch (number) {
    case 0:
      for (byte i = 0; i < MATRIX_ROW; i++) {
        for (byte j = 0; j < MATRIX_COL; j++) {
          matrixState[i][j] = (bool*)pgm_read_byte(&(zero[i][j])); //here we read the matrix from FLASH
        }
      }
      break;
    case 1:
      for (byte i = 0; i < MATRIX_ROW; i++) {
        for (byte j = 0; j < MATRIX_COL; j++) {
          matrixState[i][j] = (bool*)pgm_read_byte(&(one[i][j]));
        }
      }
      break;
    case 2:
      for (byte i = 0; i < MATRIX_ROW; i++) {
        for (byte j = 0; j < MATRIX_COL; j++) {
          matrixState[i][j] = (bool*)pgm_read_byte(&(two[i][j]));
        }
      }
      break;
    case 3:
      for (byte i = 0; i < MATRIX_ROW; i++) {
        for (byte j = 0; j < MATRIX_COL; j++) {
          matrixState[i][j] = (bool*)pgm_read_byte(&(three[i][j]));
        }
      }
      break;
    case 4:
      for (byte i = 0; i < MATRIX_ROW; i++) {
        for (byte j = 0; j < MATRIX_COL; j++) {
          matrixState[i][j] = (bool*)pgm_read_byte(&(four[i][j]));
        }
      }
      break;
    case 5:
      for (byte i = 0; i < MATRIX_ROW; i++) {
        for (byte j = 0; j < MATRIX_COL; j++) {
          matrixState[i][j] = (bool*)pgm_read_byte(&(five[i][j]));
        }
      }
      break;
    case 6:
      for (byte i = 0; i < MATRIX_ROW; i++) {
        for (byte j = 0; j < MATRIX_COL; j++) {
          matrixState[i][j] = (bool*)pgm_read_byte(&(six[i][j]));
        }
      }
      break;
    case 7:
      for (byte i = 0; i < MATRIX_ROW; i++) {
        for (byte j = 0; j < MATRIX_COL; j++) {
          matrixState[i][j] = (bool*)pgm_read_byte(&(seven[i][j]));
        }
      }
      break;
    case 8:
      for (byte i = 0; i < MATRIX_ROW; i++) {
        for (byte j = 0; j < MATRIX_COL; j++) {
          matrixState[i][j] = (bool*)pgm_read_byte(&(eight[i][j]));
        }
      }
      break;
    case 9:
      for (byte i = 0; i < MATRIX_ROW; i++) {
        for (byte j = 0; j < MATRIX_COL; j++) {
          matrixState[i][j] = (bool*)pgm_read_byte(&(nine[i][j]));
        }
      }
      break;
  }

}

void resetGame() {
  //reset all game variables to the start condition
  clearMatrix();
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

/*void showMatrix(bool newMatrix[MATRIX_ROW][MATRIX_COL]) {

  //show up the matrix by putting it to the matrixState
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = newMatrix[i][j];
    }
  }
}*/

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
  sleep_mode();
  //disable interrupt buttons after sleep
  bitClear(GIMSK, INT0); //disable interrupt pin 8 - button B - INT0
  bitClear(PCMSK0, PCINT6); //disable interrupt pin 6 - button A - PCINT6
  power_timer0_enable(); //enable Timer 0
  power_timer1_enable(); //enable Timer 1
}

