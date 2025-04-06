/*************************************************************************
  <SPACE INVADER> GAME FOR KEYCHAININO V3 (Improved)
  Created by Alessandro Matera  
  Modified by ChatGPT
  
  Improvements:
    - Up to 4 enemies can be active.
    - Enemy update period (and hence speed) increases (i.e. period decreases)
      after every SCORE_THRESHOLD points.
  
  NOTE:
    - This sketch uses Timer1 (do not use millis() here).
    - Always displays KeyChaininoFace at game start and game over.
    - After KC.scrollText(), the delay is set by the library (or you can add an
      additional delay proportionate to the text length if desired).
**************************************************************************/

#include <KeyChainino.h>

//---------------------------------------------------------------------
// Constants & Tuning Parameters
//---------------------------------------------------------------------

#define MATRIX_COL 12
#define MATRIX_ROW 12

// Enemy settings:
#define NUM_ENEMIES 4        // Increase the number of enemy slots
#define MIN_ENEMY_SPEED 600  // Minimum allowed enemy update period (faster)
#define SPEED_STEP 100       // Decrease enemy update period by this much per threshold
#define SCORE_THRESHOLD 10   // Every SCORE_THRESHOLD points, enemy speed increases

// Fire settings:
#define NUM_FIRES 6  // number of simultaneous fires

//---------------------------------------------------------------------
// Global Variables
//---------------------------------------------------------------------

// --- Shooter Bar ---
// Coordinates are in (x, y): x from 0 to MATRIX_COL-1, y from 0 to MATRIX_ROW-1
// The bar is made of 4 pixels arranged as:
/*
       [4]
    [1][2][3]
*/
int barPosition[4][2] = {
  { 3, 11 },
  { 4, 11 },
  { 5, 11 },
  { 4, 10 }
};

// --- Fire (projectiles) ---
int fire[NUM_FIRES][2];  // each fire: [row, col]
byte fireSpawned[NUM_FIRES] = { 0, 0, 0, 0, 0, 0 };

int firePositionCounter = 0;
int firePositionCounterCONSTANT = 2000;  // update period for fires

// --- Enemies ---
// Each enemy is defined by 5 LED positions arranged as:
/*
     [0]   [1]
        [2]
     [3]   [4]
*/
int enemy[NUM_ENEMIES][5][2];      // [enemy index][enemy part][coordinate: 0=x, 1=y]
byte enemyIsSpawned[NUM_ENEMIES];  // 1 if enemy is active
byte enemyIsGone[NUM_ENEMIES];     // 1 if enemy has been cleared

byte enemySpawnCounter = 0;
byte enemySpawnCounterCONSTANT = 3;  // spawn an enemy every few enemy updates

int enemyPositionCounter = 0;
int enemyPositionCounterCONSTANT = 2000;  // starting update period for enemies

// --- Game Score and Speed Tracking ---
int score = 0;             // increases when enemy is hit
bool gameStarted = false;  // game running flag
byte lastSpeedScore = 0;   // last score value at which enemy speed was increased

// --- Timer1 reload value ---
unsigned long timer1_count_;

// --- KeyChainino Face (for start & game over) ---
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

//---------------------------------------------------------------------
// Interrupt Service Routines (ISRs)
//---------------------------------------------------------------------

ISR(TIMER1_OVF_vect) {
  TIMSK1 &= ~(1 << TOIE1);  // disable Timer1 interrupts

  if (gameStarted) {
    // Update fire positions
    firePositionCounter++;
    if (firePositionCounter > firePositionCounterCONSTANT) {
      updateFirePosition();
      firePositionCounter = 0;
    }

    // Update enemy positions
    enemyPositionCounter++;
    if (enemyPositionCounter > enemyPositionCounterCONSTANT) {
      updateEnemyPosition();
      enemyPositionCounter = 0;

      // Spawn a new enemy if possible
      enemySpawnCounter++;
      if (enemySpawnCounter > enemySpawnCounterCONSTANT) {
        for (byte i = 0; i < NUM_ENEMIES; i++) {
          if (!enemyIsSpawned[i] || enemyIsGone[i]) {
            enemyIsSpawned[i] = 1;
            enemyIsGone[i] = 0;
            // Spawn enemy with a random x coordinate; y starts at 0
            enemy[i][0][0] = random(0, MATRIX_COL - 2);  // x
            enemy[i][0][1] = 0;                          // y (top)
            enemy[i][1][0] = enemy[i][0][0] + 2;         // top-right
            enemy[i][1][1] = enemy[i][0][1];
            enemy[i][2][0] = enemy[i][0][0] + 1;  // center
            enemy[i][2][1] = enemy[i][0][1] + 1;
            enemy[i][3][0] = enemy[i][0][0];  // bottom-left
            enemy[i][3][1] = enemy[i][0][1] + 2;
            enemy[i][4][0] = enemy[i][0][0] + 2;  // bottom-right
            enemy[i][4][1] = enemy[i][0][1] + 2;
            // Draw the new enemy on the display
            for (byte p = 0; p < 5; p++) {
              KC.pixel(enemy[i][p][0], enemy[i][p][1], 1);
            }
            break;  // spawn only one enemy per spawn event
          }
        }
        enemySpawnCounter = 0;
      }
    }
  }

  TCNT1 = timer1_count_;   // reload Timer1 counter
  TIMSK1 |= (1 << TOIE1);  // re-enable Timer1 interrupt
}

ISR(INT1_vect) {  // BUTTON A interrupt (unused)
  // Do nothing
}

ISR(INT0_vect) {  // BUTTON B interrupt (unused)
  // Do nothing
}

//---------------------------------------------------------------------
// Functions
//---------------------------------------------------------------------

// Spawn a fire projectile if one is free.
void spawnFire() {
  for (byte i = 0; i < NUM_FIRES; i++) {
    if (fireSpawned[i] == 0) {
      fireSpawned[i] = 1;
      fire[i][1] = 9;                  // column where fire starts
      fire[i][0] = barPosition[3][0];  // row from the shooter bar (using bar's 4th pixel)
      KC.pixel(fire[i][0], fire[i][1], 1);
      break;
    }
  }
}

// Update fire position: move left one column.
// Check the next position for a collision before moving.
void updateFirePosition() {
  for (byte f = 0; f < NUM_FIRES; f++) {
    if (fireSpawned[f]) {
      byte oldRow = fire[f][0];
      byte oldCol = fire[f][1];
      int nextCol = fire[f][1] - 1;

      // Check for collision with any enemy part at the next column.
      bool hit = false;
      byte hitEnemy = 0;
      for (byte e = 0; e < NUM_ENEMIES && !hit; e++) {
        if (enemyIsSpawned[e] && !enemyIsGone[e]) {
          for (byte p = 0; p < 5 && !hit; p++) {
            if (enemy[e][p][0] == fire[f][0] && enemy[e][p][1] == nextCol) {
              hit = true;
              hitEnemy = e;
            }
          }
        }
      }

      // Clear the current fire pixel.
      KC.pixel(oldRow, oldCol, 0);

      if (hit) {
        // Collision detected: clear the enemy.
        for (byte p = 0; p < 5; p++) {
          KC.pixel(enemy[hitEnemy][p][0], enemy[hitEnemy][p][1], 0);
          enemy[hitEnemy][p][0] = -1;
          enemy[hitEnemy][p][1] = -1;
        }
        enemyIsGone[hitEnemy] = 1;
        fireSpawned[f] = 0;
        score++;  // Increase score for the hit.
      } else {
        // No collision: update fire position if within bounds.
        if (nextCol >= 0) {
          fire[f][1] = nextCol;
          KC.pixel(fire[f][0], fire[f][1], 1);
        } else {
          // Fire has left the display.
          fireSpawned[f] = 0;
        }
      }
    }
  }
}

// Update enemy positions by moving each enemy down one row.
void updateEnemyPosition() {
  for (byte i = 0; i < NUM_ENEMIES; i++) {
    if (enemyIsSpawned[i]) {
      // If enemy is active and still within display bounds:
      if (!enemyIsGone[i] && (enemy[i][0][1] < MATRIX_ROW - 1)) {
        byte oldPos[5][2];
        for (byte p = 0; p < 5; p++) {
          oldPos[p][0] = enemy[i][p][0];
          oldPos[p][1] = enemy[i][p][1];
        }
        // Move enemy down one row (increment y coordinate)
        for (byte p = 0; p < 5; p++) {
          enemy[i][p][1]++;
        }
        // Clear old enemy pixels.
        for (byte p = 0; p < 5; p++) {
          KC.pixel(oldPos[p][0], oldPos[p][1], 0);
        }
        // Draw enemy in new position.
        for (byte p = 0; p < 5; p++) {
          KC.pixel(enemy[i][p][0], enemy[i][p][1], 1);
        }
      } else {
        // Enemy has either reached the bottom or been hit.
        for (byte p = 0; p < 5; p++) {
          KC.pixel(enemy[i][p][0], enemy[i][p][1], 0);
          enemy[i][p][0] = -1;
          enemy[i][p][1] = -1;
        }
        enemyIsGone[i] = 1;
      }
    }
  }
}

// Update the shooter bar’s position based on button input.
// Also spawns fire if both buttons are pressed.
void updateBarPosition() {
  byte oldBarPosition[4][2];
  for (byte i = 0; i < 4; i++) {
    oldBarPosition[i][0] = barPosition[i][0];
    oldBarPosition[i][1] = barPosition[i][1];
  }

  // Check buttons: if both are pressed, fire; if only one is pressed, move bar.
  if (!digitalRead(BUTTON_A) || !digitalRead(BUTTON_B)) {
    delay(80);  // Software debounce
    if (!digitalRead(BUTTON_A) && !digitalRead(BUTTON_B)) {
      spawnFire();
    } else if (!digitalRead(BUTTON_B)) {
      if (barPosition[3][0] < MATRIX_COL - 2) {  // Move right
        for (byte i = 0; i < 4; i++) {
          barPosition[i][0]++;
        }
      }
    } else if (!digitalRead(BUTTON_A)) {
      if (barPosition[1][0] > 1) {  // Move left
        for (byte i = 0; i < 4; i++) {
          barPosition[i][0]--;
        }
      }
    }
  }

  // Clear old bar position if changed.
  if (barPosition[0][0] != oldBarPosition[0][0]) {
    for (byte i = 0; i < 4; i++) {
      KC.pixel(oldBarPosition[i][0], oldBarPosition[i][1], 0);
    }
  }
  // Draw new bar position.
  for (byte i = 0; i < 4; i++) {
    KC.pixel(barPosition[i][0], barPosition[i][1], 1);
  }
}

// Check for collisions between fires and enemies.
void checkFireEnemyCollisions() {
  for (byte f = 0; f < NUM_FIRES; f++) {
    if (fireSpawned[f]) {
      bool collisionDetected = false;
      byte enemyIndex = 0;
      for (byte e = 0; e < NUM_ENEMIES && !collisionDetected; e++) {
        for (byte p = 0; p < 5 && !collisionDetected; p++) {
          if (fire[f][0] == enemy[e][p][0] && fire[f][1] == enemy[e][p][1]) {
            collisionDetected = true;
            enemyIndex = e;
            score++;  // Increase score for the hit
          }
        }
      }
      if (collisionDetected) {
        // Clear the entire enemy.
        for (byte p = 0; p < 5; p++) {
          KC.pixel(enemy[enemyIndex][p][0], enemy[enemyIndex][p][1], 0);
          enemy[enemyIndex][p][0] = -1;
          enemy[enemyIndex][p][1] = -1;
        }
        enemyIsGone[enemyIndex] = 1;
        KC.pixel(fire[f][0], fire[f][1], 0);
        fireSpawned[f] = 0;
      }
    }
  }
}

// Check for collisions between the bar and any enemy part.
void checkBarEnemyCollisions() {
  for (byte i = 0; i < 4; i++) {  // loop over bar pixels
    for (byte e = 0; e < NUM_ENEMIES; e++) {
      if (enemyIsSpawned[e] && !enemyIsGone[e]) {
        for (byte p = 0; p < 5; p++) {
          if (barPosition[i][0] == enemy[e][p][0] && barPosition[i][1] == enemy[e][p][1]) {
            // Collision with the bar ends the game.
            gameStarted = false;
          }
        }
      }
    }
  }
}

// Show the score using scrollText.
void showScore(byte scoreNumber) {
  KC.clear();
  char scoreChar[21];
  String str = "You lose. Score: " + String(scoreNumber);
  str.toCharArray(scoreChar, 21);
  KC.scrollText(scoreChar, 1, true);
}

// Reset all game variables and display the KeyChainino face.
void resetGame() {
  KC.clear();
  KC.drawBitmap(KeyChaininoFace);
  KC.display();
  delay(500);
  KC.clear();
  KC.display();

  // Reset shooter bar.
  barPosition[0][0] = 3;
  barPosition[0][1] = 11;
  barPosition[1][0] = 4;
  barPosition[1][1] = 11;
  barPosition[2][0] = 5;
  barPosition[2][1] = 11;
  barPosition[3][0] = 4;
  barPosition[3][1] = 10;

  // Reset fires.
  for (byte i = 0; i < NUM_FIRES; i++) {
    fireSpawned[i] = 0;
  }

  // Reset enemies.
  for (byte i = 0; i < NUM_ENEMIES; i++) {
    enemyIsSpawned[i] = 0;
    enemyIsGone[i] = 1;
    for (byte p = 0; p < 5; p++) {
      enemy[i][p][0] = -1;
      enemy[i][p][1] = -1;
    }
  }
  // Spawn the first enemy in slot 0.
  enemyIsSpawned[0] = 1;
  enemyIsGone[0] = 0;
  enemy[0][0][0] = random(0, MATRIX_COL - 2);
  enemy[0][0][1] = 0;
  enemy[0][1][0] = enemy[0][0][0] + 2;
  enemy[0][1][1] = enemy[0][0][1];
  enemy[0][2][0] = enemy[0][0][0] + 1;
  enemy[0][2][1] = enemy[0][0][1] + 1;
  enemy[0][3][0] = enemy[0][0][0];
  enemy[0][3][1] = enemy[0][0][1] + 2;
  enemy[0][4][0] = enemy[0][0][0] + 2;
  enemy[0][4][1] = enemy[0][0][1] + 2;
  for (byte p = 0; p < 5; p++) {
    KC.pixel(enemy[0][p][0], enemy[0][p][1], 1);
  }

  score = 0;
  enemyPositionCounterCONSTANT = 2000;  // reset to starting speed
  lastSpeedScore = 0;
  gameStarted = true;
}

// The main game loop. Also increases enemy speed every SCORE_THRESHOLD points.
void game() {
  // Increase enemy speed (i.e. decrease update period) every SCORE_THRESHOLD points.
  if (score >= lastSpeedScore + SCORE_THRESHOLD) {
    lastSpeedScore = score;
    if (enemyPositionCounterCONSTANT > MIN_ENEMY_SPEED) {
      enemyPositionCounterCONSTANT -= SPEED_STEP;
    }
  }

  checkFireEnemyCollisions();
  checkBarEnemyCollisions();
  updateBarPosition();
  KC.display();
}

// When the game ends, show the score, display the face, then reset.
void endGame() {
  showScore(score);
  KC.clear();
  KC.drawBitmap(KeyChaininoFace);
  KC.display();
  delay(500);
  KC.goSleep();
  resetGame();
}

//---------------------------------------------------------------------
// Arduino Setup and Loop
//---------------------------------------------------------------------

void setup() {
  randomSeed(analogRead(A0));  // for randomness

  KC.init();
  KC.clear();
  KC.display();

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);

  timer1_count_ = 64900;
  noInterrupts();  // disable interrupts for timer setup
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = timer1_count_;
  TCCR1B |= (1 << CS10);   // no prescaler
  TIMSK1 |= (1 << TOIE1);  // enable Timer1 overflow interrupt
  interrupts();            // re-enable interrupts

  KC.goSleep();
  resetGame();
}

void loop() {
  if (gameStarted) {
    game();
  } else {
    endGame();
  }
}
