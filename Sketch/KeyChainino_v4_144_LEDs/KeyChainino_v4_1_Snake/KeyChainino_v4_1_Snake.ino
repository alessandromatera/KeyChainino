/*************************************************************************
  SNAKE GAME FOR KEYCHAININO 144 WITH GAME OVER TEXT, SCORE, BLINKING,
  IDLE TIMEOUT (10 sec without button movement stops the game), AND SLEEP.
  (Based on the Arkanoid example, converted into Snake.)
  
  - Uses Timer1 to update the snake position.
  - Button A (pin 3, INT1) and Button B (pin 2, INT0) are polled for
    direction changes (the external ISRs remain empty so they can wake from sleep).
  - The snake “wraps” around the 12×12 display.
  - Each time the snake eats food its length increases and the score is incremented.
  - If the snake’s head collides with its body—or if no button movement occurs for ~10 sec—the game ends.
  - At game start and game over, the KeyChaininoFace is shown.
  - On game over, the screen blinks a few times, then a game over message with the score is scrolled, and finally the device goes to sleep.
  
  Created by [Your Name], adapted from the Arkanoid sketch.
**************************************************************************/

#include <KeyChainino.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdio.h>  // For sprintf

// Define the 12x12 matrix size
#define MATRIX_ROW 12
#define MATRIX_COL 12

// Button definitions (same pins as before)
#define BUTTON_A 3  // INT1: will be polled (turn left)
#define BUTTON_B 2  // INT0: will be polled (turn right)

// --- Idle timeout parameters ---
// Timer1 overflows roughly every 81 ms. For 10 sec, we need ~124 overflows.
volatile unsigned int idleCounter = 0;
const unsigned long int IDLE_THRESHOLD = 65000;  // Approximately 10 sec of inactivity

// --- Snake Game Global Variables ---

// Game state flags
volatile bool gameStarted = false;
volatile bool gameOver = false;

// Direction: 0 = UP, 1 = RIGHT, 2 = DOWN, 3 = LEFT
volatile byte currentDirection = 1;  // initially moving RIGHT

// The snake’s body (maximum length 144 = full board)
volatile byte snakeX[144];      // X positions (0..11)
volatile byte snakeY[144];      // Y positions (0..11)
volatile byte snakeLength = 0;  // current length of the snake

// Food position
volatile byte foodX = 0;
volatile byte foodY = 0;

// Score (number of foods eaten)
volatile unsigned int score = 0;

// Timer1–based snake movement update control
volatile unsigned int snakeUpdateCounter = 0;
// (Note: Adjust snakeUpdateConstant as needed for snake speed. Here it is set
// so that the snake updates every ~810 ms if using an 8 MHz internal oscillator.)
const unsigned int snakeUpdateConstant = 1000;  // (This value may need adjustment)

// Timer1 reload value (set for ~81 ms period on an 8 MHz internal oscillator)
unsigned int timer1_count_ = 64900;

// --- KeyChaininoFace ---
// This bitmap is shown at game start and game over.
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

// --- Function Prototypes ---
void resetGame();
void newFood();
void updateSnake();
void blinkScreen();

// --- Timer1 Overflow ISR ---
// This ISR is called approximately every 81 ms.
ISR(TIMER1_OVF_vect) {
  // Disable Timer1 interrupts temporarily
  TIMSK1 &= ~(1 << TOIE1);

  if (gameStarted) {
    // Update snake movement counter
    snakeUpdateCounter++;
    if (snakeUpdateCounter >= snakeUpdateConstant) {
      snakeUpdateCounter = 0;
      updateSnake();
    }
    // Also update the idle counter.
    idleCounter++;
    if (idleCounter >= IDLE_THRESHOLD) {
      // No button movement for about 10 sec → force game over.
      gameOver = true;
    }
  }
  
  // Reload Timer1 counter and re-enable Timer1 overflow interrupt
  TCNT1 = timer1_count_;
  TIMSK1 |= (1 << TOIE1);
}

// --- External Interrupts for Buttons ---
// These ISR functions are intentionally left empty so that they
// can be used solely to wake the device from sleep.
ISR(INT0_vect) {
  // Empty ISR for BUTTON_B (INT0)
}

ISR(INT1_vect) {
  // Empty ISR for BUTTON_A (INT1)
}

// --- newFood ---
// Select a new food location that is not on the snake.
void newFood() {
  bool valid = false;
  byte x, y;
  while (!valid) {
    x = random(0, MATRIX_COL);
    y = random(0, MATRIX_ROW);
    valid = true;
    // Ensure food does not appear on any snake segment.
    for (byte i = 0; i < snakeLength; i++) {
      if (snakeX[i] == x && snakeY[i] == y) {
        valid = false;
        break;
      }
    }
  }
  foodX = x;
  foodY = y;
}

// --- updateSnake ---
// Update the snake’s position, check for food or self-collision, and redraw.
void updateSnake() {
  if (gameOver) return;  // Do nothing if game is over.

  // Get current head coordinates.
  byte headIndex = snakeLength - 1;
  int headX = snakeX[headIndex];
  int headY = snakeY[headIndex];

  // Determine new head coordinates based on currentDirection.
  int newX = headX;
  int newY = headY;
  switch (currentDirection) {
    case 0: newY = headY - 1; break;  // UP
    case 1: newX = headX + 1; break;  // RIGHT
    case 2: newY = headY + 1; break;  // DOWN
    case 3: newX = headX - 1; break;  // LEFT
  }

  // Wrap around the edges.
  if (newX < 0) newX = MATRIX_COL - 1;
  if (newX >= MATRIX_COL) newX = 0;
  if (newY < 0) newY = MATRIX_ROW - 1;
  if (newY >= MATRIX_ROW) newY = 0;

  // Check for collision with the snake’s body.
  for (byte i = 0; i < snakeLength; i++) {
    if (snakeX[i] == newX && snakeY[i] == newY) {
      gameOver = true;
      return;
    }
  }

  // Check if the snake eats the food.
  bool ateFood = (newX == foodX && newY == foodY);
  if (ateFood) {
    // Add new head segment (growing snake).
    if (snakeLength < 144) {
      snakeX[snakeLength] = newX;
      snakeY[snakeLength] = newY;
      snakeLength++;
    }
    score++;  // Increment score when food is eaten.
    newFood();
    // Force an immediate update on the next Timer1 overflow.
    snakeUpdateCounter = snakeUpdateConstant - 1;
  } else {
    // Move snake: shift segments forward (remove tail) and add new head.
    for (byte i = 0; i < snakeLength - 1; i++) {
      snakeX[i] = snakeX[i + 1];
      snakeY[i] = snakeY[i + 1];
    }
    snakeX[snakeLength - 1] = newX;
    snakeY[snakeLength - 1] = newY;
  }

  // Redraw snake and food.
  KC.clear();
  for (byte i = 0; i < snakeLength; i++) {
    KC.pixel(snakeX[i], snakeY[i], 1);
  }
  KC.pixel(foodX, foodY, 1);
  KC.display();
}

// --- blinkScreen ---
// Blink the screen a few times to indicate collision/game over.
void blinkScreen() {
  for (byte i = 0; i < 3; i++) {
    KC.full();      // Turn all LEDs on.
    KC.display();
    delay(200);
    KC.clear();
    KC.display();
    delay(200);
  }
}

// --- resetGame ---
// Show the KeyChaininoFace, then initialize game state.
void resetGame() {
  // Reset idle counter.
  idleCounter = 0;

  // Display the KeyChaininoFace for 1 second at game start.
  KC.clear();
  KC.drawBitmap(KeyChaininoFace);
  KC.display();
  delay(1000);

  // Initialize a snake of length 3 in the center.
  snakeLength = 3;
  byte startX = MATRIX_COL / 2;
  byte startY = MATRIX_ROW / 2;
  // The snake starts horizontal.
  snakeX[0] = startX - 2;
  snakeY[0] = startY;
  snakeX[1] = startX - 1;
  snakeY[1] = startY;
  snakeX[2] = startX;
  snakeY[2] = startY;

  currentDirection = 1;  // initially moving RIGHT
  score = 0;             // Reset score.
  newFood();

  gameOver = false;
  gameStarted = true;
  // Reset movement counters.
  snakeUpdateCounter = 0;
  idleCounter = 0;
}

// --- setup ---
void setup() {
  // Seed the random number generator.
  randomSeed(analogRead(A0));

  // Initialize the KeyChainino LED matrix.
  KC.init();
  KC.clear();
  KC.display();

  // Set up button pins.
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);

  // Initialize Timer1 (used for snake movement).
  noInterrupts();  // Disable interrupts during Timer1 setup.
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = timer1_count_;
  // Set Timer1 prescaler to 1 - no prescaling
  TCCR1B |= (1 << CS10);
  TIMSK1 |= (1 << TOIE1);  // Enable Timer1 overflow interrupt.
  interrupts();

  // Enable external interrupts for the buttons.
  // (Their ISR functions are empty so they only help wake the MCU from sleep.)
  EIMSK |= (1 << INT0) | (1 << INT1);

  // Optionally, go to sleep at startup.
  KC.goSleep();

  resetGame();
}

// --- loop ---
void loop() {
  // Poll the buttons for direction changes (only if the game is running).
  if (!gameOver) {
    // If BUTTON_B (turn right) is pressed.
    if (digitalRead(BUTTON_B) == LOW) {
      currentDirection = (currentDirection + 1) % 4;
      // Reset idle counter upon any button movement.
      idleCounter = 0;
      delay(150);  // Debounce delay.
    }
    // If BUTTON_A (turn left) is pressed.
    if (digitalRead(BUTTON_A) == LOW) {
      currentDirection = (currentDirection + 3) % 4;  // Equivalent to subtracting 1 modulo 4.
      // Reset idle counter upon any button movement.
      idleCounter = 0;
      delay(150);  // Debounce delay.
    }
  }

  // When gameOver is detected, perform a blink sequence, scroll game-over text with the score, and then go to sleep.
  if (gameOver) {
    blinkScreen();
    char msg[30];
    sprintf(msg, "GAME OVER. SCORE: %u", score);
    KC.clear();
    KC.scrollText(msg, 1);
    delay(6000);

    // Show KeyChaininoFace for a moment before sleeping.
    KC.clear();
    KC.drawBitmap(KeyChaininoFace);
    KC.display();
    delay(1000);

    // Go to sleep. The goSleep() function disables timers and waits for a wake-up
    // (by external interrupts, whose ISR bodies are empty).
    KC.goSleep();
    // When the device wakes up, reinitialize the game.
    resetGame();
  }

  // (Nothing else needs to be done in loop() since Timer1 ISR handles snake updates.)
}
