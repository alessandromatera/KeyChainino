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

unsigned long timer1_count_;

// time to wait between turns
#define TURN_DELAY 150  //800
// how many turns per game before starting a new game
// you can also use the reset button on the board
#define TURNS_MAX 250
// how many turns to wait if there are no changes before starting a new game
#define NO_CHANGES_RESET 10

byte TURNS = 0;       // counter for turns
byte NO_CHANGES = 0;  // counter for turns without changes



bool gameStarted = false;  //indicates if the game is started
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


ISR(TIMER1_OVF_vect) {
  /* Disable Timer1 interrupts */
  TIMSK1 &= ~(1 << TOIE1);
  //THIS PART IS USED TO UPDATE THE BALL'S MOVIMENT IN THE GAME
  //if game is started change ball position
  

  TCNT1 = timer1_count_;   // Load counter for next interrupt
  TIMSK1 |= (1 << TOIE1);  // Enable timer overflow interrupt
}


ISR(INT1_vect) {  //BUTTON A INTERRUPT
  //do nothing
}

ISR(INT0_vect) {  //BUTTON B INTERRUPT
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


  timer1_count_ = 64900;

  /* Initialize Timer 1 */
  noInterrupts();          // Disable interrupts
  TCCR1A = 0;              // Normal operation
  TCCR1B = 0;              // Clear prescaler
  TCNT1 = timer1_count_;   // Load counter
  TCCR1B |= (1 << CS10);   // Prescaler = 1024
  TIMSK1 |= (1 << TOIE1);  // Enable timer overflow
  interrupts();            // Enable all interrupts

  KC.goSleep();
  resetGame();
}

void loop() {
  if (gameStarted) {  //if game is started
    game();           //go to game function
  } else {            //else end the game
    endGame();
  }
}

void game() {


  play_gol();
  KC.display();

  TURNS++;

  // reset the grid if no changes have occured recently
  // for when the game enters a static stable state
  if (NO_CHANGES > NO_CHANGES_RESET) {
    //reset_grid();
    gameStarted = false;
  }
  // reset the grid if the loop has been running a long time
  // for when the game cycles between a few stable states
  else if (TURNS > TURNS_MAX) {
    //reset_grid();
    gameStarted = false;
  } else {
    delay(TURN_DELAY);
  }
}



void play_gol() {
  /*
    1. Any live cell with fewer than two neighbours dies, as if by loneliness.
    2. Any live cell with more than three neighbours dies, as if by
    overcrowding.
    3. Any live cell with two or three neighbours lives, unchanged, to the next
    generation.
    4. Any dead cell with exactly three neighbours comes to life.
    */

  bool new_grid[MATRIX_ROW][MATRIX_COL] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 }
  };


  for (byte y = 0; y < MATRIX_COL; y++) {
    for (byte x = 0; x < MATRIX_ROW; x++) {
      byte neighboughs = count_neighboughs(y, x);
      if (frameBuffer[y][x] == 1) {
        if ((neighboughs == 2) || (neighboughs == 3)) {
          new_grid[y][x] = 1;
        } else {
          new_grid[y][x] = 0;
        }
      } else {
        if (neighboughs == 3) {
          new_grid[y][x] = 1;
        } else {
          new_grid[y][x] = 0;
        }
      }
    }
  }

  // update the current grid from the new grid and count how many changes
  // occured
  byte changes = 0;
  byte count = 0;
  for (byte y = 0; y < MATRIX_COL; y++) {
    for (byte x = 0; x < MATRIX_ROW; x++) {
      if (new_grid[y][x] != frameBuffer[y][x]) {
        changes++;
      }
      if (new_grid[y][x] == 1) {
        count++;
      }
      frameBuffer[y][x] = new_grid[y][x];
    }
  }

  if (count == 0) {
    //no life
    gameStarted = false;
  }

  // update global counter when no changes occured
  if (changes == 0) {
    NO_CHANGES++;
  }
}

// count the number of neighbough live cells for a given cell
byte count_neighboughs(byte y, byte x) {
  byte count = 0;

  // -- Row above us ---
  if (y > 0) {
    // above left
    if (x > 0) {
      count += frameBuffer[y - 1][x - 1];
    }
    // above
    count += frameBuffer[y - 1][x];
    // above right
    if ((x + 1) < 9) {
      count += frameBuffer[y - 1][x + 1];
    }
  }

  // -- Same row -------
  // left
  if (x > 0) {
    count += frameBuffer[y][x - 1];
  }
  // right
  if ((x + 1) < 9) {
    count += frameBuffer[y][x + 1];
  }

  // -- Row below us ---
  if ((y + 1) < 9) {
    // below left
    if (x > 0) {
      count += frameBuffer[y + 1][x - 1];
    }
    // below
    count += frameBuffer[y + 1][x];
    // below right
    if ((x + 1) < 9) {
      count += frameBuffer[y + 1][x + 1];
    }
  }

  return count;
}

// reset the grid
void resetGame() {
  clear();
  showKeyChaininoFace();
  display();
  delay(500);
  clear();
  display();

  NO_CHANGES = 0;
  TURNS = 0;
  for (byte y = 0; y < MATRIX_COL; y++) {
    for (byte x = 0; x < MATRIX_ROW; x++) {
      // create a random starting grid pattern
      frameBuffer[y][x] = random(0, 2);
    }
  }
  gameStarted = true;
}

void endGame() {
  clear();
  showKeyChaininoFace();
  display();
  delay(500);
  clear();
  display();

  goSleep();    //sleep to reduce power
  resetGame();  //reset game variables
}
