/*************************************************************************
 * DICE FOR KEYCHAININO www.keychainino.com
 *
 * created by Hersh Burston with lots of help from Alessandro Matera
 * *************************************************************************/

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <Entropy.h>    //Random number generator library - seems better than Arduino random number generator

#define MATRIX_ROW 5
#define MATRIX_COL 6
#define PIN_NUMBER 7

#define BUTTON_A 8     //Player 1 button
#define BUTTON_B 6     //Player 2 button

const byte pins[PIN_NUMBER] = {0, 1, 2, 3, 7, 9, 10}; //the number of the pin used for the LEDs in ordered

const byte connectionMatrix[MATRIX_ROW][MATRIX_COL][2] =       //the matrix that shows the LEDs pin connections. First Value is the Anode, second is the Cathode
{
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

byte randNumberDiceOne;      // declare dice 1
byte randNumberDiceTwo;      // declare dice 2

byte rollTurn = 0;           // keep track of roll order

//KeyChainino Face stored in FLASH in order to reduce RAM size
const PROGMEM bool KeyChaininoFace[MATRIX_ROW][MATRIX_COL] =
{
  {0, 0, 0, 0, 0, 0},
  {0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {1, 0, 0, 0, 0, 1},
  {0, 1, 1, 1, 1, 0}
};

//KeyChainino Face Frowning stored in FLASH in order to reduce RAM size
const PROGMEM bool KeyChaininoFaceFrown[MATRIX_ROW][MATRIX_COL] =
{
  {0, 0, 0, 0, 0, 0},
  {0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 1, 1, 1, 1, 0},
  {1, 0, 0, 0, 0, 1}
};

//outside border stored in FLASH in order to reduce RAM size
const PROGMEM bool outsideBorder[MATRIX_ROW][MATRIX_COL] =
{
  {1, 1, 1, 1, 1, 1},
  {1, 0, 0, 0, 0, 1},
  {1, 0, 0, 0, 0, 1},
  {1, 0, 0, 0, 0, 1},
  {1, 1, 1, 1, 1, 1}
};

//inside border stored in FLASH in order to reduce RAM size
const PROGMEM bool insideBorder[MATRIX_ROW][MATRIX_COL] =
{
  {0, 0, 0, 0, 0, 0},
  {0, 1, 1, 1, 1, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 1, 1, 1, 0},
  {0, 0, 0, 0, 0, 0}
};

//last border stored in FLASH in order to reduce RAM size
const PROGMEM bool lastBorder[MATRIX_ROW][MATRIX_COL] =
{
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0}
};

//NUMBERS for dice 1 stored in FLASH in order to reduce RAM size

const PROGMEM bool oneDiceOne[MATRIX_ROW][MATRIX_COL]  =
{
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0}
};

const PROGMEM bool twoDiceOne[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 0, 0, 0},
  {0, 0, 1, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {1, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0}
};

const PROGMEM bool threeDiceOne[MATRIX_ROW][MATRIX_COL]  =
{
  {0, 0, 0, 0, 0, 0},
  {0, 0, 1, 0, 0, 0},
  {0, 1, 0, 0, 0, 0},
  {1, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0}
};

const PROGMEM bool fourDiceOne[MATRIX_ROW][MATRIX_COL] =
{
  {0, 0, 0, 0, 0, 0},
  {1, 0, 1, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {1, 0, 1, 0, 0, 0},
  {0, 0, 0, 0, 0, 0}
};

const PROGMEM bool fiveDiceOne[MATRIX_ROW][MATRIX_COL]  =
{
  {0, 0, 0, 0, 0, 0},
  {1, 0, 1, 0, 0, 0},
  {0, 1, 0, 0, 0, 0},
  {1, 0, 1, 0, 0, 0},
  {0, 0, 0, 0, 0, 0}
};

const PROGMEM bool sixDiceOne[MATRIX_ROW][MATRIX_COL] =
{
  {0, 0, 0, 0, 0, 0},
  {1, 0, 1, 0, 0, 0},
  {1, 0, 1, 0, 0, 0},
  {1, 0, 1, 0, 0, 0},
  {0, 0, 0, 0, 0, 0}
};

//NUMBERS for dice 2 used stored in FLASH in order to reduce RAM size

const PROGMEM bool oneDiceTwo[MATRIX_ROW][MATRIX_COL]  =
{
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 1, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0}
};

const PROGMEM bool twoDiceTwo[MATRIX_ROW][MATRIX_COL] =
{
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 1},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 0, 0, 0, 0}
};

const PROGMEM bool threeDiceTwo[MATRIX_ROW][MATRIX_COL]  =
{
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 1},
  {0, 0, 0, 0, 1, 0},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 0, 0, 0, 0}
};

const PROGMEM bool fourDiceTwo[MATRIX_ROW][MATRIX_COL] =
{
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 0, 1},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 0, 1},
  {0, 0, 0, 0, 0, 0}
};

const PROGMEM bool fiveDiceTwo[MATRIX_ROW][MATRIX_COL]  =
{
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 0, 1},
  {0, 0, 0, 0, 1, 0},
  {0, 0, 0, 1, 0, 1},
  {0, 0, 0, 0, 0, 0}
};

const PROGMEM bool sixDiceTwo[MATRIX_ROW][MATRIX_COL] =
{
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 0, 1},
  {0, 0, 0, 1, 0, 1},
  {0, 0, 0, 1, 0, 1},
  {0, 0, 0, 0, 0, 0}
};

ISR(TIM1_OVF_vect) {  // timer1 overflow interrupt service routine

  TCNT1 = 65405;

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
        delayMicroseconds(500); //250
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


void setup()
{

  Entropy.initialize();        //Initialize the random number generator

  //configure LED pins
  for (byte i = 0; i < PIN_NUMBER; i++)
  {
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

  for (int i = 0; i < 5; i++)    // show fancy introduction
  {
    showOutsideBorder();
    showInsideBorder();
    showLastBorder();
    showLastBorder();
    showInsideBorder();
    showOutsideBorder();
    showOutsideBorder();
    showInsideBorder();
    showLastBorder();
    showLastBorder();
    showInsideBorder();
    showOutsideBorder();
  }
  showKeyChaininoFace(); //show KeyChainino smile face
  delay(1500);
  clearMatrix(); //clear the Matrix
}

void loop()
{

  if ((digitalRead(BUTTON_A) == LOW) && (digitalRead(BUTTON_B) == LOW))  // press both butons together to restart
  {
    setup();
  }

  if ((digitalRead(BUTTON_A) == LOW) && (rollTurn == 1))      // show frown face if it's not the correct turn
  {
    clearMatrix();
    showKeyChaininoFaceFrown();
    rollTurn = 1;
  }

  if ((digitalRead(BUTTON_A) == LOW) && (rollTurn == 0))      //first player roll the dice
  {

    randNumberDiceOne = Entropy.random(1, 7);        //use random(1, 7) for Arduino random number generator
    randNumberDiceTwo = Entropy.random(1, 7);        //use random(1, 7) for Arduino random number generator
    delay(200);
    clearMatrix();
    diceRoll();
    rollTurn = 1;
  }



  if ((digitalRead(BUTTON_B) == LOW) && (rollTurn == 0))    // show frown face if it's not the correct turn
  {
    clearMatrix();
    showKeyChaininoFaceFrown();
    rollTurn = 0;
  }

  if ((digitalRead(BUTTON_B) == LOW) && (rollTurn == 1))    //second player roll the dice
  {

    randNumberDiceOne = Entropy.random(1, 7);              //use random(1, 7) for Arduino random number generator
    randNumberDiceTwo = Entropy.random(1, 7);              //use random(1, 7) for Arduino random number generator
    delay(200);
    clearMatrix();
    diceRoll();
    rollTurn = 0;
  }

}


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

void showKeyChaininoFace() {
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(KeyChaininoFace[i][j])); //here we read the matrix from FLASH
    }
  }
}

void showKeyChaininoFaceFrown() {
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(KeyChaininoFaceFrown[i][j])); //here we read the matrix from FLASH
    }
  }
}

void showOutsideBorder() {
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(outsideBorder[i][j])); //here we read the matrix from FLASH
    }
  }
  delay(20);
}

void showInsideBorder() {
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(insideBorder[i][j])); //here we read the matrix from FLASH
    }
  }
  delay(20);
}

void showLastBorder() {
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(lastBorder[i][j])); //here we read the matrix from FLASH
    }
  }
  delay(20);
}



void showPlayerOneOneOne()
{

  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(oneDiceOne[i][j])) || (bool*)pgm_read_byte(&(oneDiceTwo[i][j]));

    }
  }

}

void showPlayerOneOneTwo()
{

  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(oneDiceOne[i][j])) || (bool*)pgm_read_byte(&(twoDiceTwo[i][j])); //here we read the matrix from FLASH
    }
  }
}

void showPlayerOneOneThree()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(oneDiceOne[i][j])) || (bool*)pgm_read_byte(&(threeDiceTwo[i][j])); //here we read the matrix from FLASH
    }
  }
}

void showPlayerOneOneFour()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(oneDiceOne[i][j])) || (bool*)pgm_read_byte(&(fourDiceTwo[i][j])); //here we read the matrix from FLASH
    }
  }
}

void showPlayerOneOneFive()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(oneDiceOne[i][j])) || (bool*)pgm_read_byte(&(fiveDiceTwo[i][j]));
    }
  }
}

void showPlayerOneOneSix()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(oneDiceOne[i][j])) || (bool*)pgm_read_byte(&(sixDiceTwo[i][j]));
    }
  }
}


void showPlayerOneTwoOne()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(twoDiceOne[i][j])) || (bool*)pgm_read_byte(&(oneDiceTwo[i][j]));
    }
  }
}

void showPlayerOneTwoTwo()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(twoDiceOne[i][j])) || (bool*)pgm_read_byte(&(twoDiceTwo[i][j]));
    }
  }
}

void showPlayerOneTwoThree()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(twoDiceOne[i][j])) || (bool*)pgm_read_byte(&(threeDiceTwo[i][j]));
    }
  }
}

void showPlayerOneTwoFour()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(twoDiceOne[i][j])) || (bool*)pgm_read_byte(&(fourDiceTwo[i][j]));
    }
  }
}

void showPlayerOneTwoFive()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(twoDiceOne[i][j])) || (bool*)pgm_read_byte(&(fiveDiceTwo[i][j]));
    }
  }
}

void showPlayerOneTwoSix()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(twoDiceOne[i][j])) || (bool*)pgm_read_byte(&(sixDiceTwo[i][j]));
    }
  }
}


void showPlayerOneThreeOne()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(threeDiceOne[i][j])) || (bool*)pgm_read_byte(&(oneDiceTwo[i][j]));
    }
  }
}

void showPlayerOneThreeTwo()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(threeDiceOne[i][j])) || (bool*)pgm_read_byte(&(twoDiceTwo[i][j]));
    }
  }
}

void showPlayerOneThreeThree()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(threeDiceOne[i][j])) || (bool*)pgm_read_byte(&(threeDiceTwo[i][j]));
    }
  }
}

void showPlayerOneThreeFour()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(threeDiceOne[i][j])) || (bool*)pgm_read_byte(&(fourDiceTwo[i][j]));
    }
  }
}

void showPlayerOneThreeFive()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(threeDiceOne[i][j])) || (bool*)pgm_read_byte(&(fiveDiceTwo[i][j]));
    }
  }
}

void showPlayerOneThreeSix()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(threeDiceOne[i][j])) || (bool*)pgm_read_byte(&(sixDiceTwo[i][j]));
    }
  }
}



void showPlayerOneFourOne()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(fourDiceOne[i][j])) || (bool*)pgm_read_byte(&(oneDiceTwo[i][j]));
    }
  }
}

void showPlayerOneFourTwo()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(fourDiceOne[i][j])) || (bool*)pgm_read_byte(&(twoDiceTwo[i][j]));
    }
  }
}

void showPlayerOneFourThree()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(fourDiceOne[i][j])) || (bool*)pgm_read_byte(&(threeDiceTwo[i][j]));
    }
  }
}

void showPlayerOneFourFour()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(fourDiceOne[i][j])) || (bool*)pgm_read_byte(&(fourDiceTwo[i][j]));
    }
  }
}

void showPlayerOneFourFive()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(fourDiceOne[i][j])) || (bool*)pgm_read_byte(&(fiveDiceTwo[i][j]));
    }
  }
}

void showPlayerOneFourSix()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(fourDiceOne[i][j])) || (bool*)pgm_read_byte(&(sixDiceTwo[i][j]));
    }
  }
}


void showPlayerOneFiveOne()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(fiveDiceOne[i][j])) || (bool*)pgm_read_byte(&(oneDiceTwo[i][j]));
    }
  }
}

void showPlayerOneFiveTwo()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(fiveDiceOne[i][j])) || (bool*)pgm_read_byte(&(twoDiceTwo[i][j]));
    }
  }
}

void showPlayerOneFiveThree()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(fiveDiceOne[i][j])) || (bool*)pgm_read_byte(&(threeDiceTwo[i][j]));
    }
  }
}

void showPlayerOneFiveFour()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(fiveDiceOne[i][j])) || (bool*)pgm_read_byte(&(fourDiceTwo[i][j]));
    }
  }
}

void showPlayerOneFiveFive()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(fiveDiceOne[i][j])) || (bool*)pgm_read_byte(&(fiveDiceTwo[i][j]));
    }
  }
}

void showPlayerOneFiveSix()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(fiveDiceOne[i][j])) || (bool*)pgm_read_byte(&(sixDiceTwo[i][j]));
    }
  }
}



void showPlayerOneSixOne()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(sixDiceOne[i][j])) || (bool*)pgm_read_byte(&(oneDiceTwo[i][j]));
    }
  }
}

void showPlayerOneSixTwo()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(sixDiceOne[i][j])) || (bool*)pgm_read_byte(&(twoDiceTwo[i][j]));
    }
  }
}

void showPlayerOneSixThree()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(sixDiceOne[i][j])) || (bool*)pgm_read_byte(&(threeDiceTwo[i][j]));
    }
  }
}

void showPlayerOneSixFour()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(sixDiceOne[i][j])) || (bool*)pgm_read_byte(&(fourDiceTwo[i][j]));
    }
  }
}

void showPlayerOneSixFive()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(sixDiceOne[i][j])) || (bool*)pgm_read_byte(&(fiveDiceTwo[i][j]));
    }
  }
}

void showPlayerOneSixSix()
{
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(sixDiceOne[i][j])) || (bool*)pgm_read_byte(&(sixDiceTwo[i][j]));
    }
  }
}

void diceRoll()
{
  if ((randNumberDiceOne == 1) && (randNumberDiceTwo == 1))
  {
    showPlayerOneOneOne();
  }

  if  ((randNumberDiceOne == 1) && (randNumberDiceTwo == 2))
  {
    showPlayerOneOneTwo();
  }
  if ((randNumberDiceOne == 1) && (randNumberDiceTwo == 3))
  {
    showPlayerOneOneThree();
  }
  if ((randNumberDiceOne == 1) && (randNumberDiceTwo == 4))
  {
    showPlayerOneOneFour();
  }
  if ((randNumberDiceOne == 1) && (randNumberDiceTwo == 5))
  {
    showPlayerOneOneFive();
  }
  if ((randNumberDiceOne == 1) && (randNumberDiceTwo == 6))
  {
    showPlayerOneOneSix();
  }


  if ((randNumberDiceOne == 2) && (randNumberDiceTwo == 1))
  {
    showPlayerOneTwoOne();
  }

  if  ((randNumberDiceOne == 2) && (randNumberDiceTwo == 2))
  {
    showPlayerOneTwoTwo();
  }
  if ((randNumberDiceOne == 2) && (randNumberDiceTwo == 3))
  {
    showPlayerOneTwoThree();
  }
  if ((randNumberDiceOne == 2) && (randNumberDiceTwo == 4))
  {
    showPlayerOneTwoFour();
  }
  if ((randNumberDiceOne == 2) && (randNumberDiceTwo == 5))
  {
    showPlayerOneTwoFive();
  }
  if ((randNumberDiceOne == 2) && (randNumberDiceTwo == 6))
  {
    showPlayerOneTwoSix();
  }



  if ((randNumberDiceOne == 3) && (randNumberDiceTwo == 1))
  {
    showPlayerOneThreeOne();
  }

  if  ((randNumberDiceOne == 3) && (randNumberDiceTwo == 2))
  {
    showPlayerOneThreeTwo();
  }
  if ((randNumberDiceOne == 3) && (randNumberDiceTwo == 3))
  {
    showPlayerOneThreeThree();
  }
  if ((randNumberDiceOne == 3) && (randNumberDiceTwo == 4))
  {
    showPlayerOneThreeFour();
  }
  if ((randNumberDiceOne == 3) && (randNumberDiceTwo == 5))
  {
    showPlayerOneThreeFive();
  }
  if ((randNumberDiceOne == 3) && (randNumberDiceTwo == 6))
  {
    showPlayerOneThreeSix();
  }


  if ((randNumberDiceOne == 4) && (randNumberDiceTwo == 1))
  {
    showPlayerOneFourOne();
  }

  if  ((randNumberDiceOne == 4) && (randNumberDiceTwo == 2))
  {
    showPlayerOneFourTwo();
  }
  if ((randNumberDiceOne == 4) && (randNumberDiceTwo == 3))
  {
    showPlayerOneFourThree();
  }
  if ((randNumberDiceOne == 4) && (randNumberDiceTwo == 4))
  {
    showPlayerOneFourFour();
  }
  if ((randNumberDiceOne == 4) && (randNumberDiceTwo == 5))
  {
    showPlayerOneFourFive();
  }
  if ((randNumberDiceOne == 4) && (randNumberDiceTwo == 6))
  {
    showPlayerOneFourSix();
  }


  if ((randNumberDiceOne == 5) && (randNumberDiceTwo == 1))
  {
    showPlayerOneFiveOne();
  }

  if  ((randNumberDiceOne == 5) && (randNumberDiceTwo == 2))
  {
    showPlayerOneFiveTwo();
  }
  if ((randNumberDiceOne == 5) && (randNumberDiceTwo == 3))
  {
    showPlayerOneFiveThree();
  }
  if ((randNumberDiceOne == 5) && (randNumberDiceTwo == 4))
  {
    showPlayerOneFiveFour();
  }
  if ((randNumberDiceOne == 5) && (randNumberDiceTwo == 5))
  {
    showPlayerOneFiveFive();
  }
  if ((randNumberDiceOne == 5) && (randNumberDiceTwo == 6))
  {
    showPlayerOneFiveSix();
  }


  if ((randNumberDiceOne == 6) && (randNumberDiceTwo == 1))
  {
    showPlayerOneSixOne();
  }

  if  ((randNumberDiceOne == 6) && (randNumberDiceTwo == 2))
  {
    showPlayerOneSixTwo();
  }
  if ((randNumberDiceOne == 6) && (randNumberDiceTwo == 3))
  {
    showPlayerOneSixThree();
  }
  if ((randNumberDiceOne == 6) && (randNumberDiceTwo == 4))
  {
    showPlayerOneSixFour();
  }
  if ((randNumberDiceOne == 6) && (randNumberDiceTwo == 5))
  {
    showPlayerOneSixFive();
  }
  if ((randNumberDiceOne == 6) && (randNumberDiceTwo == 6))
  {
    showPlayerOneSixSix();
  }
}
