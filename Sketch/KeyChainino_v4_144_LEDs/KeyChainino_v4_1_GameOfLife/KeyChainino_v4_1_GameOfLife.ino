/*
  GAME OF LIFE FOR KEYCHAININO 144 (12x12)
  Ported to 12x12 using KeyChainino library
*/

#include <KeyChainino.h>


// Dimensions
#define MATRIX_ROWS 12
#define MATRIX_COLS 12

// Timing
#define TURN_DELAY 150       // ms between generations
#define TURNS_MAX 250        // max generations before reset
#define NO_CHANGES_RESET 10  // reset after this many static generations

ISR(INT1_vect) {  //BUTTON A INTERRUPT
  //do nothing
}

ISR(INT0_vect) {  //BUTTON B INTERRUPT
  //do nothing
}


// Displayed faces stored in FLASH
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

// State arrays
bool grid[MATRIX_ROWS][MATRIX_COLS];
bool new_grid[MATRIX_ROWS][MATRIX_COLS];

// Counters
byte turns = 0;
byte no_changes = 0;
bool gameStarted = false;


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

  KC.goSleep();
  resetGame();
}


void loop() {
  if (gameStarted) {
    playGoL();
    delay(TURN_DELAY);
  } else {
    // Show face, sleep, then restart
    KC.clear();
    KC.drawBitmap(KeyChaininoFace);
    KC.display();
    delay(500);
    KC.goSleep();  // power down until button press
    resetGame();
  }
}


// Count live neighbours around (y,x)
byte count_neighbors(byte y, byte x) {
  byte cnt = 0;
  for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <= 1; dx++) {
      if (dy == 0 && dx == 0) continue;
      int ny = y + dy;
      int nx = x + dx;
      if (ny >= 0 && ny < MATRIX_ROWS && nx >= 0 && nx < MATRIX_COLS) {
        if (grid[ny][nx]) cnt++;
      }
    }
  }
  return cnt;
}

// Initialize display and random grid
void resetGame() {
  // Show welcome face
  KC.clear();
  KC.drawBitmap(KeyChaininoFace);
  KC.display();
  delay(500);

  // Reset counters
  no_changes = 0;
  turns = 0;

  // Randomize grid
  for (byte y = 0; y < MATRIX_ROWS; y++) {
    for (byte x = 0; x < MATRIX_COLS; x++) {
      grid[y][x] = random(0, 2);
    }
  }

  // Draw initial grid
  KC.clear();
  for (byte y = 0; y < MATRIX_ROWS; y++) {
    for (byte x = 0; x < MATRIX_COLS; x++) {
      if (grid[y][x]) KC.pixel(x, y, 1);
    }
  }
  KC.display();
  gameStarted = true;
}

// Run one Game of Life step
void playGoL() {
  byte changes = 0;
  byte alive = 0;
  // Compute new state
  for (byte y = 0; y < MATRIX_ROWS; y++) {
    for (byte x = 0; x < MATRIX_COLS; x++) {
      byte n = count_neighbors(y, x);
      if (grid[y][x]) {
        new_grid[y][x] = (n == 2 || n == 3);
      } else {
        new_grid[y][x] = (n == 3);
      }
      if (new_grid[y][x] != grid[y][x]) changes++;
      if (new_grid[y][x]) alive++;
    }
  }
  // Swap and display
  memcpy(grid, new_grid, sizeof(grid));
  KC.clear();
  for (byte y = 0; y < MATRIX_ROWS; y++) {
    for (byte x = 0; x < MATRIX_COLS; x++) {
      if (grid[y][x]) KC.pixel(x, y, 1);
    }
  }
  KC.display();

  // Update counters
  turns++;
  if (changes == 0) no_changes++;
  else no_changes = 0;

  // Check end conditions
  if (alive == 0 || no_changes > NO_CHANGES_RESET || turns > TURNS_MAX) {
    gameStarted = false;
  }
}
