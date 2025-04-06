#include <KeyChainino.h>
#include <LED_Font_1.h>

/****************************************************************************
  PONG (User vs CPU) for KeyChainino 144
  - CPU paddle has random "miss" chance
  - Both user & CPU move horizontally at ~45 ms intervals
  - BALL STARTS ON THE USER'S PADDLE AT GAME START

  Created by: [Your Name]

  Hardware:
   - KeyChainino 144 (12×12 matrix, ATmega328p)
   - Two push buttons:
       Button A on pin 3
       Button B on pin 2
  Libraries:
   - KeyChainino.cpp, KeyChainino.h
****************************************************************************/
//---------------------------------------------------------------------------
// GAME SETTINGS & CONSTANTS
//---------------------------------------------------------------------------
#define USER_PADDLE_LENGTH   3      // User paddle horizontal length
#define CPU_PADDLE_LENGTH    3      // CPU paddle horizontal length

#define USER_ROW   (MATRIX_ROW - 1) // Bottom row => 11 on a 12×12
#define CPU_ROW    0                // Top row    => 0  on a 12×12

#define POINTS_TO_WIN       5

// Ball speed parameters
#define INITIAL_BALL_SPEED  900
#define MIN_BALL_SPEED      350
#define BALL_SPEED_STEP     40

// CPU movement
#define CPU_MOVE_DELAY      45   // CPU moves horizontally once every 45 ms
#define CPU_MISS_PERCENT    20   // % chance that CPU “skips” a move randomly

// Timer1 reload
unsigned long timer1_count_;

//---------------------------------------------------------------------------
// PADDLES & BALL GLOBALS
//---------------------------------------------------------------------------
int userPaddleX;                
int userPaddleY = USER_ROW;  

int cpuPaddleX;
int cpuPaddleY = CPU_ROW;

int ballX;           // Current ball X
int ballY;           // Current ball Y
int ballDirX;        // -1=left, +1=right
int ballDirY;        // -1=up,   +1=down

int ballUpdateCounter = 0;      
int ballUpdateDelay   = INITIAL_BALL_SPEED;

int userScore = 0;
int cpuScore  = 0;

bool gameRunning = false;
bool userWon     = false;
bool cpuWon      = false;

//---------------------------------------------------------------------------
// INTERRUPTS
//---------------------------------------------------------------------------
ISR(TIMER1_OVF_vect) {
  // Disable Timer1 interrupts during update
  TIMSK1 &= ~(1 << TOIE1);

  if (gameRunning) {
    ballUpdateCounter++;
    if (ballUpdateCounter > ballUpdateDelay) {
      updateBall();
      ballUpdateCounter = 0;
    }
  }

  // Reload Timer1 & re-enable
  TCNT1 = timer1_count_;
  TIMSK1 |= (1 << TOIE1);
}

// Button interrupts (unused here; we do polling in loop)
ISR(INT1_vect) { }
ISR(INT0_vect) { }

//---------------------------------------------------------------------------
// SETUP
//---------------------------------------------------------------------------
void setup() {
  // Initialize random if desired
  randomSeed(analogRead(A0));

  KC.init();
  KC.clear();
  KC.display();

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);

  // Configure Timer1 for frequent overflow
  timer1_count_ = 64900; 
  noInterrupts();
  TCCR1A = 0; 
  TCCR1B = 0;
  TCNT1  = timer1_count_;
  TCCR1B |= (1 << CS10);   // prescaler=1
  TIMSK1 |= (1 << TOIE1);  // enable Timer1 overflow
  interrupts();

  // Sleep to save power until user presses a button
  KC.goSleep();
  resetGame();
}

//---------------------------------------------------------------------------
// MAIN LOOP
//---------------------------------------------------------------------------
void loop() {
  if (gameRunning) {
    // In-game: handle paddles, draw, etc.
    updateUserPaddle();
    updateCpuPaddle();

    drawEverything();
    KC.display();

    // Check for winner
    if (userScore >= POINTS_TO_WIN) {
      userWon     = true;
      gameRunning = false;
    }
    if (cpuScore >= POINTS_TO_WIN) {
      cpuWon      = true;
      gameRunning = false;
    }

  } else {
    // Not running => might be end of game or first start
    if (userWon) {
      userWinAnimation();
      endGame();
    } else if (cpuWon) {
      cpuWinAnimation();
      endGame();
    } else {
      KC.goSleep();
      resetGame();
    }
  }
}

//---------------------------------------------------------------------------
// GAME LOGIC
//---------------------------------------------------------------------------

/**
 * Reset the entire game state.
 * 
 * Now the ball starts on the user's paddle instead of the center.
 */
void resetGame() {
  KC.clear();
  KC.display();

  userScore = 0;
  cpuScore  = 0;
  userWon   = false;
  cpuWon    = false;

  // Place user paddle near bottom center
  userPaddleX = (MATRIX_COL / 2) - (USER_PADDLE_LENGTH / 2);

  // Place CPU paddle near top center
  cpuPaddleX  = (MATRIX_COL / 2) - (CPU_PADDLE_LENGTH / 2);

  // Instead of center: place the ball on the user's paddle
  ballX = userPaddleX + (USER_PADDLE_LENGTH / 2);  // horizontally center on the paddle
  ballY = userPaddleY - 1;                         // just above user’s paddle
  
  // We'll send it upwards. For variety, keep random horizontal direction:
  ballDirX = (random(0, 2) == 0) ? +1 : -1;
  ballDirY = -1;  // up

  ballUpdateCounter = 0;
  ballUpdateDelay   = INITIAL_BALL_SPEED;

  // Start the game
  gameRunning = true;
}

/**
 * Move the ball, check collisions, handle scoring.
 */
void updateBall() {
  // Erase old ball position
  KC.pixel(ballX, ballY, 0);

  int newX = ballX + ballDirX;
  int newY = ballY + ballDirY;

  // Left/right boundaries
  if (newX < 0) {
    newX     = 0;
    ballDirX = +1; 
  } else if (newX >= MATRIX_COL) {
    newX     = MATRIX_COL - 1;
    ballDirX = -1;
  }

  // Top/bottom scoring logic
  if (newY >= MATRIX_ROW) {
    if (!ballHitsUserPaddle()) {
      cpuScore++;
      resetBallAfterPoint(false);
      return;
    }
  }
  if (newY < 0) {
    if (!ballHitsCpuPaddle()) {
      userScore++;
      resetBallAfterPoint(true);
      return;
    }
  }

  // If ball hits exactly CPU row
  if (newY == cpuPaddleY) {
    if (ballDirY < 0 && ballHitsCpuPaddle()) {
      newY = cpuPaddleY;
      ballDirY = +1;
      speedUpBall();
    }
  }

  // If ball hits exactly user row
  if (newY == userPaddleY) {
    if (ballDirY > 0 && ballHitsUserPaddle()) {
      newY = userPaddleY;
      ballDirY = -1;
      speedUpBall();
    }
  }

  // Final position
  ballX = newX;
  ballY = newY;

  // Draw new ball position
  KC.pixel(ballX, ballY, 1);
}

/**
 * Speed up ball by decreasing its delay.
 */
void speedUpBall() {
  ballUpdateDelay -= BALL_SPEED_STEP;
  if (ballUpdateDelay < MIN_BALL_SPEED) {
    ballUpdateDelay = MIN_BALL_SPEED;
  }
}

/**
 * Reset ball after a point. 
 * If userGetsServe=true => CPU missed => user "serve" from near user paddle.
 */
void resetBallAfterPoint(bool userGetsServe) {
  KC.pixel(ballX, ballY, 0);

  if (userGetsServe) {
    // CPU missed => user serve
    ballX = userPaddleX + (USER_PADDLE_LENGTH / 2);
    ballY = userPaddleY - 1; // just above user’s paddle
    ballDirY = -1;
  } else {
    // user missed => CPU serve
    ballX = cpuPaddleX + (CPU_PADDLE_LENGTH / 2);
    ballY = cpuPaddleY + 1; // just below CPU’s paddle
    ballDirY = +1;
  }

  // Random horizontal direction
  ballDirX = (random(0,2) == 0) ? +1 : -1;

  ballUpdateCounter = 0;
  ballUpdateDelay   = INITIAL_BALL_SPEED;

  // Redraw the ball
  KC.pixel(ballX, ballY, 1);
}

/**
 * Check if the ball overlaps the user's paddle horizontally.
 */
bool ballHitsUserPaddle() {
  if (ballY >= MATRIX_ROW) {
    return false;
  }
  if (ballX >= userPaddleX && ballX < (userPaddleX + USER_PADDLE_LENGTH)) {
    return true;
  }
  return false;
}

/**
 * Check if the ball overlaps the CPU's paddle horizontally.
 */
bool ballHitsCpuPaddle() {
  if (ballY < 0) {
    return false;
  }
  if (ballX >= cpuPaddleX && ballX < (cpuPaddleX + CPU_PADDLE_LENGTH)) {
    return true;
  }
  return false;
}

//---------------------------------------------------------------------------
// PADDLE CONTROLS
//---------------------------------------------------------------------------

/**
 * Let the user move paddle with about 45 ms delay per press.
 * Button A => left, Button B => right
 */
void updateUserPaddle() {
  if (!digitalRead(BUTTON_A)) {
    delay(45);
    if (!digitalRead(BUTTON_A)) {
      userPaddleX--;
    }
  }
  if (!digitalRead(BUTTON_B)) {
    delay(45);
    if (!digitalRead(BUTTON_B)) {
      userPaddleX++;
    }
  }
  // Bounds
  if (userPaddleX < 0) {
    userPaddleX = 0;
  }
  if (userPaddleX + USER_PADDLE_LENGTH > MATRIX_COL) {
    userPaddleX = MATRIX_COL - USER_PADDLE_LENGTH;
  }
}

/**
 * CPU moves horizontally at the same ~45 ms rate, with random “miss.”
 */
void updateCpuPaddle() {
  static unsigned long lastMoveTime = 0;

  if (millis() - lastMoveTime >= CPU_MOVE_DELAY) {
    lastMoveTime = millis();

    // RANDOM SKIP: if random < CPU_MISS_PERCENT, the CPU does nothing
    if (random(0, 100) < CPU_MISS_PERCENT) {
      return; // skip movement => CPU makes a "mistake"
    }

    // Otherwise, track ball
    int cpuCenter = cpuPaddleX + (CPU_PADDLE_LENGTH / 2);

    if (ballX < cpuCenter) {
      cpuPaddleX--;
    } else if (ballX > cpuCenter) {
      cpuPaddleX++;
    }

    // Keep in bounds
    if (cpuPaddleX < 0) {
      cpuPaddleX = 0;
    }
    if (cpuPaddleX + CPU_PADDLE_LENGTH > MATRIX_COL) {
      cpuPaddleX = MATRIX_COL - CPU_PADDLE_LENGTH;
    }
  }
}

//---------------------------------------------------------------------------
// RENDERING & END-GAME ANIMATIONS
//---------------------------------------------------------------------------

/**
 * Draw ball, paddles, small scoreboard, etc.
 */
void drawEverything() {
  KC.clear();

  // Ball
  KC.pixel(ballX, ballY, 1);

  // User paddle
  for (int i = 0; i < USER_PADDLE_LENGTH; i++) {
    KC.pixel(userPaddleX + i, userPaddleY, 1);
  }

  // CPU paddle
  for (int i = 0; i < CPU_PADDLE_LENGTH; i++) {
    KC.pixel(cpuPaddleX + i, cpuPaddleY, 1);
  }

  // Minimal scoreboard
  for (int s = 0; s < userScore && s < POINTS_TO_WIN; s++) {
    KC.pixel(0, USER_ROW - s, 1);
  }
  for (int s = 0; s < cpuScore && s < POINTS_TO_WIN; s++) {
    KC.pixel(0, cpuPaddleY + s, 1);
  }
}

/**
 * User victory animation
 */
void userWinAnimation() {
  KC.clear();
  KC.display();
  for (int i = 0; i < 3; i++) {
    KC.full();
    KC.display();
    delay(200);
    KC.clear();
    KC.display();
    delay(200);
  }
  KC.scrollText("YOU WIN!", 1, true);
  delay(1000);
}

/**
 * CPU victory animation
 */
void cpuWinAnimation() {
  KC.clear();
  KC.display();
  for (int i = 0; i < 3; i++) {
    KC.full();
    KC.display();
    delay(200);
    KC.clear();
    KC.display();
    delay(200);
  }
  KC.scrollText("CPU WINS!", 1, true);
  delay(1000);
}

/**
 * End game: sleep & reset.
 */
void endGame() {
  KC.goSleep();
  resetGame();
}
