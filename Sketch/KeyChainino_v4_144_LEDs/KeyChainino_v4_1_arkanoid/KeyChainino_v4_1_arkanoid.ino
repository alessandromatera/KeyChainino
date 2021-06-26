/*************************************************************************
  ARKANOID GAME FOR KEYCHAININO 144
  https://www.keychainino.com/product/keychainino-144

  created by Alessandro Matera
* ************************************************************************
*/

/*
   TODO:
    - sistemare numeri score (più grandi)

*/


#include <KeyChainino.h>

#define InitBallSpeed 1000


unsigned long timer1_count_;


///BAR VARIABLES
byte barCurrentPosition[2][2] = { //the current bar position
  {11, 5},
  {11, 6}
};

byte barNewPosition[2][2] = { //the new bar position. initialized as current position
  {11, 5},
  {11, 6}
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
int ballUpdatePositionCONSTANT = InitBallSpeed; //this number is inversely proportional to the speed of the ball

//the game score calculated in the number of collision between bar and ball
int score = 0; //0 //MAX 255 for byte

byte lives = 3;
bool loosingLife = false;

bool gameStarted = false; //indicates if the game is started
bool win = false;

unsigned long previousMillis = 0;

//KeyChainino Face stored in FLASH in order to reduce RAM size
const byte KeyChaininoFace[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const byte KeyChaininoSadFace[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

byte the_wall[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
                  };

const byte WALLHEIGHT = 4;

ISR(TIMER1_OVF_vect) {
  /* Disable Timer1 interrupts */
  TIMSK1 &= ~(1 << TOIE1);
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

  TCNT1 = timer1_count_;         // Load counter for next interrupt
  TIMSK1 |= (1 << TOIE1);        // Enable timer overflow interrupt
}


ISR(INT1_vect) { //BUTTON A INTERRUPT
  //do nothing
}

ISR(INT0_vect) { //BUTTON B INTERRUPT
  //do nothing
}



void setup() {
  //configure LED pins

  // Initialize and clear display

  randomSeed(analogRead(A0));


  KC.init();
  KC.clear();
  KC.display();

  //configure Buttons pins
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);

  // initialize Timer1


  timer1_count_ =  64900;

  /* Initialize Timer 1 */
  noInterrupts();                                 // Disable interrupts
  TCCR1A = 0;                                     // Normal operation
  TCCR1B = 0;                                     // Clear prescaler
  TCNT1 = timer1_count_;                          // Load counter
  TCCR1B |=  (1 << CS10);  // Prescaler = 1024
  TIMSK1 |= (1 << TOIE1);                         // Enable timer overflow
  interrupts();                                   // Enable all interrupts

  KC.goSleep();
  resetGame();

}

void loop() {
  if (gameStarted) { //if game is started
    game(); //go to game function
  } else { //else end the game
    if (win) {
      youWin();
    } else {
      youLose();
    }
  }
}

void game() {
  if (loosingLife) {
    showRemainingLives();
  }
  updateBarPosition(); //update the bar position by checking buttons
  updateWall();
  KC.display();
}

void youLose() {
  //SHOW SCORE
  showScore(score); //shows the score number
  KC.clear();
  KC.drawBitmap(KeyChaininoSadFace);
  KC.display();
  delay(500);
  endGame();
}

void endGame() {
  KC.goSleep(); //sleep to reduce power
  resetGame(); //reset game variables
}


void updateBallPosition() {
  //change ball position depending on the ball direction
  ballY = ballCurrentPosition[0] + ballDirection[0];
  ballX = ballCurrentPosition[1] + ballDirection[1];

  // Bounce if we hit a block above the ball
  int i = ballY * ROW_SIZE + ballX;
  if ( the_wall[i] > 0 ) {
    the_wall[i]--;
    ballY = (i / ROW_SIZE) + abs((i / ROW_SIZE) - ballY);
    ballDirection[0] = 1;
    scoreUpdate();
  }

  // Bounce if we hit a block below the ball
  if ( the_wall[i] > 0 ) {
    the_wall[i]--;
    ballY = (i / ROW_SIZE) + abs((i / ROW_SIZE) - ballY);
    ballDirection[0] = -1;
    scoreUpdate();
  }

  // Bounce if we hit a block to the left the ball
  if ( the_wall[i] > 0 ) {
    the_wall[i]--;
    ballY = (i / ROW_SIZE) + abs((i / ROW_SIZE) - ballY);
    ballDirection[1] = 1;
    scoreUpdate();
  }

  // Bounce if we hit a block to the right the ball
  if ( the_wall[i] > 0 ) {
    the_wall[i]--;
    ballY = (i / ROW_SIZE) + abs((i / ROW_SIZE) - ballY);
    ballDirection[1] = -1;
    scoreUpdate();
  }

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
    } else { //ball touched bottom = END
      lives -= 1;
      if (lives > 0) {
        ballDirection[0] = -1;
        ballDirection[1] = 0;
        ballY = MATRIX_COL - 2;
        ballX = random(barCurrentPosition[0][1], barCurrentPosition[1][1] + 1);
        loosingLife = true;

      } else {
        ballDirection[0] = 0;
        ballDirection[1] = 0;
        gameStarted = false;
      }
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
  KC.pixel(ballCurrentPosition[1], ballCurrentPosition[0], 0);
  //set current bar position to new position
  ballCurrentPosition[0] = ballNewPosition[0];
  ballCurrentPosition[1] = ballNewPosition[1];
  //show new bar Position
  KC.pixel(ballNewPosition[1], ballNewPosition[0], 1);
  KC.display();
}

void scoreUpdate() {
  score++;
  if (score % 5 == 0) { //if wallCounter is egual to 10
    ballUpdatePositionCONSTANT -= 25; //we can increase the speed of the wall
    if (ballUpdatePositionCONSTANT < 600) { //by decreasing the ballUpdatePositionSpeed variable
      ballUpdatePositionCONSTANT = 600; //if this variable is under 600, we stay at this speed.
    }
  }

  //Check win
  if (score == 48) { //48
    win = true;
    gameStarted = false;
  }

}

void showRemainingLives() {
  //stop timer1 to not move ball
  TIMSK1 &= ~(1 << TOIE1);

  loosingLife = false;
  String str;
  char livesChar[15];
  if (lives > 1) {
    str = String(lives) + " lives left";
  } else {
    str = String(lives) + " life left";
  }
  str.toCharArray(livesChar, 15);
  KC.clear();
  KC.display();
  KC.scrollText(livesChar, 1);
  delay(4000);
  KC.clear();
  KC.display();
  TIMSK1 |= (1 << TOIE1);        // Enable timer overflow
}

void youWin() {
  KC.clear();
  KC.display();
  for (byte i = 0; i < 5; i++) {
    KC.full();
    KC.display();
    delay(100);
    KC.clear();
    KC.display();
    delay(100);
  }

  for (byte i = 0; i < 15; i++ ) {
    byte x = random(0, 11);
    byte y = random(0, 11);
    for (byte radius = 0; radius < 12; radius++ ) {
      KC.clear();
      KC.circle(x, y, radius);
      KC.circle(x, y, radius + 1);
      KC.display();
      delay(30);
    }
  }
  KC.clear();
  KC.display();
  KC.scrollText("YOU WIN", 1);
  delay(3000);
  KC.clear();
  KC.drawBitmap(KeyChaininoFace);
  KC.display();
  delay(500);
  endGame(); //to to end game
}


void updateBarPosition() {

  //depends on which button is pressed, change the bar position
  // to left (button A) or right (button B)

  if (!digitalRead(BUTTON_B)) {

    delay(80); //80
    if (!digitalRead(BUTTON_B)) {
      barX1++;
      barX2++;
    }
  }

  if (!digitalRead(BUTTON_A)) {
    delay(80);//80
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

    KC.pixel(barCurrentPosition[0][1], barCurrentPosition[0][0], 0);
    KC.pixel(barCurrentPosition[1][1], barCurrentPosition[1][0], 0);

  }

  //set current bar position to new position
  barCurrentPosition[0][0] = barNewPosition[0][0];
  barCurrentPosition[0][1] = barNewPosition[0][1];
  barCurrentPosition[1][0] = barNewPosition[1][0];
  barCurrentPosition[1][1] = barNewPosition[1][1];

  KC.pixel(barNewPosition[0][1], barNewPosition[0][0], 1);
  KC.pixel(barNewPosition[1][1], barNewPosition[1][0], 1);

}

void updateWall() {
  for (byte y = 0; y < ROW_SIZE; y++ ) {
    for (byte x = 0; x < WALLHEIGHT; x++ ) {
      if ( the_wall[(x * COL_SIZE) + y] > 0 ) {
        KC.pixel(y, x, 1);
      }
    }
  }
}

// Create a randomized ball launch angle

void showScore(byte scoreNumber) {
  //clearMatrix();
  KC.clear();
  char scoreChar[21];
  String str = "You lose. Score: " + String(scoreNumber);
  str.toCharArray(scoreChar, 21);
  KC.scrollText(scoreChar, 1);
  delay(5500);
}


void resetGame() {
  //reset all game variables to the start condition
  KC.clear();
  //clearMatrix();
  KC.drawBitmap(KeyChaininoFace);
  KC.display();
  delay(500);
  //clearMatrix();
  KC.clear();
  KC.display();
  //delay(300);

  barCurrentPosition[0][0] = 0;
  barCurrentPosition[0][1] = 0;
  barCurrentPosition[1][0] = 0;
  barCurrentPosition[1][1] = 0;

  barNewPosition[0][0] = 11;
  barNewPosition[0][1] = 5;
  barNewPosition[1][0] = 11;
  barNewPosition[1][1] = 6;

  barX1 = barNewPosition[0][1];
  barY1 = barNewPosition[0][0];
  barX2 = barNewPosition[1][1];
  barY2 = barNewPosition[1][0];


  ballCurrentPosition[0] = MATRIX_COL - 2;
  ballCurrentPosition[1] = random(barNewPosition[0][1], barNewPosition[1][1] + 1);

  ballX = ballCurrentPosition[1];
  ballY = ballCurrentPosition[0];

  ballDirection[0] = -1;
  ballDirection[1] = 0;

  ballUpdatePositionCounter = 0;

  score = 0;
  win = false;

  ballUpdatePositionCONSTANT = InitBallSpeed;

  for (byte i = 0; i < (MATRIX_COL * WALLHEIGHT); i++) {
    the_wall[i] = 1;
  }
  lives = 3;
  gameStarted = true;
}
