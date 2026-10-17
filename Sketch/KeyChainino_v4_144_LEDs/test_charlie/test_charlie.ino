/*************************************************************************
  CHARLIEPLEX MATRIX TEST FOR KEYCHAININO 144 www.keychainino.com

  created by Alessandro Matera

  Drives the 12x12 charlieplexed matrix directly (no library) with a
  static test pattern, to check LEDs and pin wiring. Not a game: the
  buttons do nothing.

* ************************************************************************
*/

#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

#define MATRIX_ROW 12
#define MATRIX_COL 12
#define PIN_NUMBER 13

#define BUTTON_A 3 // INT1
#define BUTTON_B 2 // INT0

unsigned long timer = 250;

byte i_Charlie = 0;
byte j_Charlie = 0;

byte ledRow = 0;


const byte pins[PIN_NUMBER] = {4, 5, 6, 7, 8, 9, 10, A0, A1, A2, A3, A4, A5}; //the number of the pin used for the LEDs in ordered

const byte connectionMatrix[MATRIX_ROW][MATRIX_COL][2] = { //the matrix that show the LEDs pin connections. Firs Value is the Anode, second is the Catode
  {{1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}, {8, 0}, {9, 0}, {10, 0}, {11, 0}, {12, 0}},
  {{0, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 1}, {7, 1}, {8, 1}, {9, 1}, {10, 1}, {11, 1}, {12, 1}},
  {{0, 2}, {1, 2}, {3, 2}, {4, 2}, {5, 2}, {6, 2}, {7, 2}, {8, 2}, {9, 2}, {10, 2}, {11, 2}, {12, 2}},
  {{0, 3}, {1, 3}, {2, 3}, {4, 3}, {5, 3}, {6, 3}, {7, 3}, {8, 3}, {9, 3}, {10, 3}, {11, 3}, {12, 3}},
  {{0, 4}, {1, 4}, {2, 4}, {3, 4}, {5, 4}, {6, 4}, {7, 4}, {8, 4}, {9, 4}, {10, 4}, {11, 4}, {12, 4}},
  {{0, 5}, {1, 5}, {2, 5}, {3, 5}, {4, 5}, {6, 5}, {7, 5}, {8, 5}, {9, 5}, {10, 5}, {11, 5}, {12, 5}},
  {{0, 6}, {1, 6}, {2, 6}, {3, 6}, {4, 6}, {5, 6}, {7, 6}, {8, 6}, {9, 6}, {10, 6}, {11, 6}, {12, 6}},
  {{0, 7}, {1, 7}, {2, 7}, {3, 7}, {4, 7}, {5, 7}, {6, 7}, {8, 7}, {9, 7}, {10, 7}, {11, 7}, {12, 7}},
  {{0, 8}, {1, 8}, {2, 8}, {3, 8}, {4, 8}, {5, 8}, {6, 8}, {7, 8}, {9, 8}, {10, 8}, {11, 8}, {12, 8}},
  {{0, 9}, {1, 9}, {2, 9}, {3, 9}, {4, 9}, {5, 9}, {6, 9}, {7, 9}, {8, 9}, {10, 9}, {11, 9}, {12, 9}},
  {{0, 10}, {1, 10}, {2, 10}, {3, 10}, {4, 10}, {5, 10}, {6, 10}, {7, 10}, {8, 10}, {9, 10}, {11, 10}, {12, 10}},
  {{0, 11}, {1, 11}, {2, 11}, {3, 11}, {4, 11}, {5, 11}, {6, 11}, {7, 11}, {8, 11}, {9, 11}, {10, 11}, {12, 11}},
};

bool matrixState[MATRIX_ROW][MATRIX_COL] = { //the matrix that will be always used to turn ON or OFF the LEDs
  {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
  {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
  {0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
  {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
};


//BAR VARIABLES

/*
     [4]
  [1][2][3]
*/
byte barPosition[4][2] = {
  {11, 3},
  {11, 4},
  {11, 5},
  {10, 4}
};


byte fire[9][2];
byte fireSpawned[6] = {
  0, 0, 0, 0, 0, 0
};

int firePositionCounter = 0;
int firePositionCounterCONSTANT = 2000; //2000



//enemy

/*
   [0][1]
   [2][3]
*/


/*
   [0]   [1]
      [2]
   [3]   [4]
*/

int enemy[2][5][2]; //two enemies
byte enemyIsSpawned[4] = {
  1, 0,
};

byte enemyIsGone[4] = { //used when enemy disappear because of collision or screen size
  1, 1,
};


byte enemySpawnCounter = 0;
byte enemySpawnCounterCONSTANT = 3;

int enemyPositionCounter = 0;
int enemyPositionCounterCONSTANT = 3000;

//the game score calculated in the number of collision between bar and ball
int score = 0; //0 //MAX 255 for byte

bool gameStarted = false; //indicates if the game is started

//KeyChainino Face stored in FLASH in order to reduce RAM size
const PROGMEM bool KeyChaininoFace[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

//NUMBERS used in score stored in FLASH in order to reduce RAM size

const PROGMEM bool one[MATRIX_ROW][MATRIX_COL]  = {
  {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
};


const PROGMEM bool two[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
};

const PROGMEM bool three[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0},
  {0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
};

const PROGMEM bool four[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0},
  {0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0},
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
};

const PROGMEM bool five[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
  {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
};

const PROGMEM bool six[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0},
  {0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
  {0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
  {0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
};

const PROGMEM bool seven[MATRIX_ROW][MATRIX_COL] = {
  {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
  {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
};

const PROGMEM bool eight[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
  {0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
  {0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
};

const PROGMEM bool nine[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
  {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0},
  {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0},
};
const PROGMEM bool zero[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0},
  {0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0},
  {0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0},
  {0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
  {0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
};




ISR(TIMER2_OVF_vect) {  // timer1 overflow interrupt service routine
  cli(); //disable interrupt
  TCNT2 = timer;// 65405;



  // THIS PART IS USED TO UPDATE THE CHARLIEPLEXING LEDS MATRIX
  // YOU CAN JUST DON'T CARE ABOUT THIS PART
  // BECAUSE YOU CAN CODE LIKE A STANDARD MATRIX BY MANIPULATING THE
  // VALUE OF THE matrixState MATRIX


  /*pinMode(pins[connectionMatrix[i_Charlie][j_Charlie][0]], INPUT); //set both positive pole and negative pole
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
    }*/



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
  /*TCCR1A = 0;    // set entire TCCR1A register to 0
    TCCR1B = 0;    // set entire TCCR1A register to 0

    bitSet(TIMSK1, TOIE1); // enable Timer1 overflow interrupt:

    TCNT1 = timer;

    // set prescaler
    //bitSet(TCCR1B, CS10);
    TCCR1B |= (1 << CS12) | (1 << CS21) | (1 << CS10);*/

  TCCR2A = 0;                                     // Normal operation
  TCCR2B = 0;                                     // Clear prescaler
  TCNT2 = timer;                          // Load counter
  TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20);  // Prescaler = 1024
  TIMSK2 |= (1 << TOIE2);                         // Enable timer overflow

  //disabling all unnecessary peripherals to reduce power
  ADCSRA = 0;
  bitSet(ADCSRA, ADEN); //disable ADC
  bitSet(ACSR, ACD); // disable Analog comparator, saves 4 uA
  power_adc_disable(); // disable ADC converter
  power_spi_disable();
  power_usart0_disable();
  power_twi_disable();
  //power_timer2_disable();
  wdt_disable();


  // enable global interrupts:
  sei();

  //goSleep();
  //resetGame();

}

void loop() {

  for (byte i = 0; i < PIN_NUMBER; i++) {
    //pinMode(pins[i], INPUT);
    //delay(10);
  }


  pinMode(pins[ledRow], OUTPUT);
  digitalWrite(pins[ledRow], LOW);

  for (byte i = 0; i < PIN_NUMBER; i++) {
    if ((i != ledRow) && (matrixState[ledRow][i] == 1)) {
      pinMode(pins[i], OUTPUT);
      //delay(1100);
      digitalWrite(pins[i], HIGH);
    }
  }
  //delay(1000);
  delayMicroseconds(500);

  /*for (byte i = 0; i < MATRIX_ROW; i++) {
    //if (matrixState[ledRow][i] == 1) {
    pinMode(pins[connectionMatrix[ledRow][i][0]], INPUT);
    //digitalWrite(pins[connectionMatrix[ledRow][i][0]], LOW);
    delay(200);
    //}
    }*/

  if (++ledRow >= MATRIX_ROW) {
    ledRow = 0;
  }

  //delay(10);

}



void game() {
  //check fire-monster collisions
  checkFireEnemyCollisions();
  //check bar-monster collisions
  checkBarEnemyCollisions();
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

void spawnFire() {
  for (byte i = 0; i < 6; i++) {
    if (fireSpawned[i] == 0) {
      fireSpawned[i] = 1;
      fire[i][0] = 9;
      fire[i][1] = barPosition[3][1]; //fire follow shooter bar
      //spawn fire
      matrixState[fire[i][0]][fire[i][1]] = 1;
      break;
    }
  }

  //fireSpawned[fireToSpawn] = 1;
}

void updateFirePosition() {

  for (byte f = 0; f < 6; f++) {
    if (fireSpawned[f] == 1) {

      byte oldFirePosition[2];

      oldFirePosition[0] = fire[f][0];
      oldFirePosition[1] = fire[f][1];

      if (fire[f][0] > 0) { //fire is not outside matrix
        fire[f][0]--;
      } else {
        fireSpawned[f] = 0;
        //set fire position
        fire[f][0] = 11;
        fire[f][1] = 11;
      }

      matrixState[oldFirePosition[0]][oldFirePosition[1]] = 0;
      if (fireSpawned[f] == 1) {
        matrixState[fire[f][0]][fire[f][1]] = 1;
      }
    } else { //fire not spawned
      //clear fire
      //matrixState[fire[f][0]][fire[f][1]] = 0;
    }
  }
}



void updateEnemyPosition() {

  // enemy
  /*
     [0][1]
     [2][3]
  */

  for (byte whichEnemy = 0; whichEnemy < 2; whichEnemy++) { //check every monster
    if (enemyIsSpawned[whichEnemy]) {

      if (!enemyIsGone[whichEnemy] && (enemy[whichEnemy][2][0] < MATRIX_ROW - 1)) {
        //enemy is not gone and inside matrix

        //bring monster down
        byte oldEnemyPosition[4][2];
        for (byte i = 0; i < 4; i++) {
          for (byte j = 0; j < 2; j++) {
            oldEnemyPosition[i][j] = enemy[whichEnemy][i][j];
          }
        }
        enemy[whichEnemy][0][0]++;
        enemy[whichEnemy][1][0]++;
        enemy[whichEnemy][2][0]++;
        enemy[whichEnemy][3][0]++;

        for (byte i = 0; i < 4; i++) {
          //delete oldEnemyPosition
          matrixState[oldEnemyPosition[i][0]][oldEnemyPosition[i][1]] = 0;
        }

        for (byte i = 0; i < 4; i++) {
          //set newEnemyPosition
          matrixState[enemy[whichEnemy][i][0]][enemy[whichEnemy][i][1]] = 1;
        }

      } else { //enemy over matrix or killed




        //clear enemy matrix
        for (byte i = 0; i < 4; i++) {
          matrixState[enemy[whichEnemy][i][0]][enemy[whichEnemy][i][1]] = 0;
        }
        //create new enemy

        //check different position if is the same as the other monster
        byte otherEnemy;

        if (whichEnemy == 0) { //enemy 0
          otherEnemy = 1;
        } else { //whichEnemy == 1
          otherEnemy = 0;
        }

        if ((enemy[otherEnemy][0][0] >= 3) || enemyIsGone[otherEnemy]) {


          enemyIsGone[whichEnemy] = 0;

          enemy[whichEnemy][0][0] = 0; //ROW
          enemy[whichEnemy][0][1] = random(0, MATRIX_COL - 1); //COL
          enemy[whichEnemy][1][0] = enemy[whichEnemy][0][0];
          enemy[whichEnemy][1][1] = enemy[whichEnemy][0][1] + 1;
          enemy[whichEnemy][2][0] = enemy[whichEnemy][0][0] + 1;
          enemy[whichEnemy][2][1] = enemy[whichEnemy][0][1];
          enemy[whichEnemy][3][0] = enemy[whichEnemy][0][0] + 1;
          enemy[whichEnemy][3][1] = enemy[whichEnemy][0][1] + 1;

          for (byte i = 0; i < 4; i++) {
            //set newEnemyPosition
            matrixState[enemy[whichEnemy][i][0]][enemy[whichEnemy][i][1]] = 1;
          }
        }
      }
    }

  }
}



void updateBarPosition() {

  byte oldBarPosition[4][2];

  for (byte i = 0; i < 4; i++) {
    for (byte j = 0; j < 2; j++) {
      oldBarPosition[i][j] = barPosition[i][j];
    }
  }

  /* BAR

       [4]
    [1][2][3]

  */

  //depends on which button is pressed, change the bar position
  // to left (button A) or right (button B) or Togeter to fire

  if (!digitalRead(BUTTON_A) || !digitalRead(BUTTON_B)) {
    delay(80); //software unbounce
    if (!digitalRead(BUTTON_A) && !digitalRead(BUTTON_B)) {
      spawnFire(); //FIRE!!!
    } else if (!digitalRead(BUTTON_B)) {
      if (barPosition[3][1] < MATRIX_COL - 2) {
        //go right
        for (byte i = 0; i < 4; i++) {
          barPosition[i][1]++;
        }
      }
    } else if (!digitalRead(BUTTON_A)) {
      if (barPosition[1][1] > 1) {
        //go left
        for (byte i = 0; i < 4; i++) {
          barPosition[i][1]--;
        }
      }
    }
  }


  //only if the bar position is different
  // (means that the button was pressed)
  if (barPosition[0][1] != oldBarPosition[0][1]) {
    //delete current bar Position
    for (byte i = 0; i < 4; i++) {
      matrixState[oldBarPosition[i][0]][oldBarPosition[i][1]] = 0;
    }
  }

  //show new bar Position
  for (byte i = 0; i < 4; i++) {
    matrixState[barPosition[i][0]][barPosition[i][1]] = 1;
  }

}

void checkFireEnemyCollisions() {

  bool collisionDetected = false;
  byte enemyKilled = 0;
  byte fireInvolved = 0;

  for (byte i = 0; i < 6; i++) { //cycle all fires
    if (fireSpawned[i]) {
      for (byte j = 0; j < 2; j++) { //cycle all enemy
        for (byte z = 0; z < 4; z++) { //cycle all enemy components
          if (fire[i][0] == enemy[j][z][0] && fire[i][1] == enemy[j][z][1]) {
            //found collision - delete enemy
            collisionDetected = true;
            enemyIsGone[j] = 1;
            enemyKilled = j;
            fireInvolved = i;
            score++;
          }
        }
      }
    }
  }

  if (collisionDetected) {
    for (byte i = 0; i < 2; i++) {
      if (enemyKilled == i) {
        //clear enemy
        for (byte j = 0; j < 4; j++) {
          //clear enemy on matrix
          matrixState[enemy[i][j][0]][enemy[i][j][1]] = 0;
          //set enemy position to 20 out of matrix
          enemy[i][j][0] = 20;
          enemy[i][j][1] = 20;
        }
        //clear fire
        for (byte j = 0; j < 4; j++) {
          matrixState[fire[fireInvolved][0]][fire[fireInvolved][1]] = 0;
        }
        fireSpawned[fireInvolved] = 0;
      }
    }
  }
}


void checkBarEnemyCollisions() {

  for (byte i = 0; i < 4; i++) { //cycle all bar's elements
    for (byte j = 0; j < 2; j++) { //cycle all enemy
      if (enemyIsSpawned[j] && !enemyIsGone[j]) {
        for (byte z = 0; z < 4; z++) { //cycle all enemy components
          if (barPosition[i][0] == enemy[j][z][0] && barPosition[i][1] == enemy[j][z][1]) {
            //found collision - delete enemy
            gameStarted = false;
          }
        }
      }
    }
  }
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
      delay(70);
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

  barPosition[0][0] = 11;
  barPosition[0][1] = 3;
  barPosition[1][0] = 11;
  barPosition[1][1] = 4;
  barPosition[2][0] = 11;
  barPosition[2][1] = 5;
  barPosition[3][0] = 10;
  barPosition[3][1] = 4;

  for (byte i = 0; i < 6; i++) {
    fireSpawned[i] = 0;
  }

  enemyIsSpawned[0] = 1;
  enemyIsSpawned[1] = 0;

  enemyIsGone[0] = 1;
  enemyIsGone[1] = 1;

  for (byte j = 0; j < 2; j++) { //cycle all enemy
    for (byte z = 0; z < 4; z++) { //cycle all enemy components
      for (byte x = 0; x < 2; x++) {
        enemy[j][z][x] = 20; //outside matrix
      }
    }
  }


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

void setMatrixStateBit(byte i, byte j) {
  matrixState[i][j] = 1;
}
void clearMatrixStateBit(byte i, byte j) {
  matrixState[i][j] = 0;
}

void goSleep() {
  //going sleep to reduce power consuming

  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      pinMode(pins[connectionMatrix[i][j][0]], INPUT); //set both positive pole and negative pole
      pinMode(pins[connectionMatrix[i][j][1]], INPUT); // to INPUT in order to turn OFF the LED
    }
  }

  power_timer0_disable(); //disable Timer 0
  power_timer1_disable(); //disable Timer 1

  //clean the charlieplexing
  i_Charlie = 0;
  j_Charlie = 0;


  //enable interrupt buttons to allow wakeup from button interrupts
  bitSet(EIMSK, INT0); //enable interrupt button B - INT0
  bitSet(EIMSK, INT1); //enable interrupt button A - INT1

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  while (digitalRead(BUTTON_B) || digitalRead(BUTTON_A)) { //until all the two buttons are pressend
    sleep_mode();
  }

  //disable interrupt buttons after sleep
  bitClear(EIMSK, INT0); //enable interrupt button B - INT0
  bitClear(EIMSK, INT1); //enable interrupt button A - INT1

  power_timer0_enable(); //enable Timer 0
  power_timer1_enable(); //enable Timer 1

}
