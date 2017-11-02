/*************************************************************************
   SCROLLING YOUR NAME - FOR KEYCHAININO www.keychainino.com

   created by Alessandro Matera
 * ************************************************************************
*/

#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

#define MATRIX_ROW 5
#define MATRIX_COL 6
#define PIN_NUMBER 7

#define BUTTON_A 6 //pin 6 - PCINT6
#define BUTTON_B 8 //pin 8 - INT0

#define MATRIX_ROW 5
#define MATRIX_COL 6
#define PIN_NUMBER 7

#define BUTTON_A 2 // INT1
#define BUTTON_B 3 // INT0

unsigned long timer = 64900;

byte i_Charlie = 0;
byte j_Charlie = 0;

const byte pins[PIN_NUMBER] = {4, 5, 6, 7, 8, 9, 10}; //the number of the pin used for the LEDs in ordered

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


//KeyChainino Face stored in FLASH in order to reduce RAM size
const PROGMEM bool KeyChaininoFace[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 0, 0, 0},
  {0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {1, 0, 0, 0, 0, 1},
  {0, 1, 1, 1, 1, 0}
};

//CHARTERS used to make the word to be scrolling. stored in FLASH in order to reduce RAM size

const PROGMEM bool A[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 1, 1, 0, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 1, 1, 1, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 0, 0, 1, 0}
};

const PROGMEM bool B[MATRIX_ROW][MATRIX_COL] = {
  {0, 1, 1, 1, 0, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 1, 1, 0, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 1, 1, 0, 0}
};

const PROGMEM bool C[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 1, 1, 0},
  {0, 0, 1, 0, 0, 0},
  {0, 0, 1, 0, 0, 0},
  {0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 1, 0}
};

const PROGMEM bool D[MATRIX_ROW][MATRIX_COL] = {
  {0, 1, 1, 1, 0, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 1, 1, 0, 0}
};

const PROGMEM bool E[MATRIX_ROW][MATRIX_COL] = {
  {0, 1, 1, 1, 0, 0},
  {0, 1, 0, 0, 0, 0},
  {0, 1, 1, 1, 0, 0},
  {0, 1, 0, 0, 0, 0},
  {0, 1, 1, 1, 0, 0}
};

const PROGMEM bool F[MATRIX_ROW][MATRIX_COL] = {
  {0, 1, 1, 1, 1, 0},
  {0, 1, 0, 0, 0, 0},
  {0, 1, 1, 1, 0, 0},
  {0, 1, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0}
};

const PROGMEM bool G[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 1, 1, 1, 0},
  {0, 1, 0, 0, 0, 0},
  {0, 1, 0, 1, 1, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 0, 1, 1, 0, 0}
};

const PROGMEM bool H[MATRIX_ROW][MATRIX_COL] = {
  {0, 1, 0, 0, 1, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 1, 1, 1, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 0, 0, 1, 0}
};

const PROGMEM bool I[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 1, 0, 0},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 0, 1, 0, 0}
};

const PROGMEM bool L[MATRIX_ROW][MATRIX_COL] = {
  {0, 1, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0},
  {0, 1, 1, 1, 1, 0}
};

const PROGMEM bool M[MATRIX_ROW][MATRIX_COL] = {
  {0, 1, 0, 0, 0, 1},
  {0, 1, 1, 0, 1, 1},
  {0, 1, 0, 1, 0, 1},
  {0, 1, 0, 0, 0, 1},
  {0, 1, 0, 0, 0, 1}
};

const PROGMEM bool N[MATRIX_ROW][MATRIX_COL] = {
  {0, 1, 0, 0, 1, 0},
  {0, 1, 1, 0, 1, 0},
  {0, 1, 0, 1, 1, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 0, 0, 1, 0}
};

const PROGMEM bool O[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 1, 1, 0, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 0, 1, 1, 0, 0}
};

const PROGMEM bool P[MATRIX_ROW][MATRIX_COL] = {
  {0, 1, 1, 1, 0, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 1, 1, 0, 0},
  {0, 1, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0}
};

const PROGMEM bool Q[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 1, 1, 0, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 0, 1, 0, 0},
  {0, 0, 1, 0, 1, 0}
};

const PROGMEM bool R[MATRIX_ROW][MATRIX_COL] = {
  {0, 1, 1, 1, 0, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 1, 1, 0, 0},
  {0, 1, 0, 1, 0, 0},
  {0, 1, 0, 0, 1, 0}
};

const PROGMEM bool S[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 1, 1, 1, 0},
  {0, 1, 0, 0, 0, 0},
  {0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 1, 0},
  {0, 1, 1, 1, 0, 0}
};

const PROGMEM bool T[MATRIX_ROW][MATRIX_COL] = {
  {0, 1, 1, 1, 1, 1},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 0, 1, 0, 0}
};

const PROGMEM bool U[MATRIX_ROW][MATRIX_COL] = {
  {0, 1, 0, 0, 1, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 0, 1, 1, 0, 0}
};

const PROGMEM bool V[MATRIX_ROW][MATRIX_COL] = {
  {1, 0, 0, 0, 1, 0},
  {1, 0, 0, 0, 1, 0},
  {1, 0, 0, 0, 1, 0},
  {0, 1, 0, 1, 0, 0},
  {0, 0, 1, 0, 0, 0}
};

const PROGMEM bool Z[MATRIX_ROW][MATRIX_COL] = {
  {0, 1, 1, 1, 1, 0},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 1, 0, 0, 0},
  {0, 1, 0, 0, 0, 0},
  {1, 1, 1, 1, 1, 0}
};

const PROGMEM bool Y[MATRIX_ROW][MATRIX_COL] = {
  {0, 1, 0, 0, 0, 1},
  {0, 0, 1, 0, 1, 0},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 1, 0, 0, 0},
  {0, 1, 0, 0, 0, 0}
};

const PROGMEM bool J[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 1, 0, 0},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 0, 1, 0, 0},
  {1, 0, 0, 1, 0, 0},
  {0, 1, 1, 0, 0, 0}
};

const PROGMEM bool K[MATRIX_ROW][MATRIX_COL]  = {
  {0, 0, 1, 0, 1, 0},
  {0, 0, 1, 1, 0, 0},
  {0, 0, 1, 0, 0, 0},
  {0, 0, 1, 1, 0, 0},
  {0, 0, 1, 0, 1, 0}
};

const PROGMEM bool X[MATRIX_ROW][MATRIX_COL]  = {
  {1, 0, 0, 0, 1, 0},
  {0, 1, 0, 1, 0, 0},
  {0, 0, 1, 0, 0, 0},
  {0, 1, 0, 1, 0, 0},
  {1, 0, 0, 0, 1, 0}
};


const PROGMEM bool W[MATRIX_ROW][MATRIX_COL]  = {
  {1, 0, 0, 0, 1, 0},
  {1, 0, 0, 0, 1, 0},
  {1, 0, 0, 0, 1, 0},
  {1, 0, 1, 0, 1, 0},
  {0, 1, 0, 1, 0, 0}
};

const PROGMEM bool SPACE[MATRIX_ROW][MATRIX_COL]  = {
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0}
};



ISR(TIMER1_OVF_vect) {  // timer1 overflow interrupt service routine
  cli(); //disable interrupt
  TCNT1 = timer;// 65405;

  // THIS PART IS USED TO UPDATE THE CHARLIEPLEXING LEDS MATRIX
  // YOU CAN JUST DON'T CARE ABOUT THIS PART
  // BECAUSE YOU CAN CODE LIKE A STANDARD MATRIX BY MANIPULATING THE
  // VALUE OF THE matrixState MATRIX

  //check from matrixState which LED to turn ON or OFF

  pinMode(pins[connectionMatrix[i_Charlie][j_Charlie][0]], INPUT); //set both positive pole and negative pole
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
  }

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
  TCCR1A = 0;    // set entire TCCR1A register to 0
  TCCR1B = 0;    // set entire TCCR1A register to 0

  bitSet(TIMSK1, TOIE1); // enable Timer1 overflow interrupt:

  TCNT1 = timer;

  // set prescaler
  bitSet(TCCR1B, CS10);

  //disabling all unnecessary peripherals to reduce power
  ADCSRA = 0;
  bitSet(ADCSRA, ADEN); //disable ADC
  bitSet(ACSR, ACD); // disable Analog comparator, saves 4 uA
  power_adc_disable(); // disable ADC converter
  power_spi_disable();
  power_usart0_disable();
  power_usart1_disable();
  power_twi_disable();
  power_timer2_disable();
  power_timer3_disable();
  bitSet(PRR1, 4); //disable timer 4 (bit 4 = PRTIM4)
  wdt_disable();


  // enable global interrupts:
  sei();

  showKeyChaininoFace(); //show KeyChainino smile face
  delay(500);
  clearMatrix(); //clear the Matrix

}

//WRITE HERE WHAT DO YOU WANT, BUT ONLY IN CAPITAL CHARTERS. DON'T USE SYMBOLS.
// ONLY THE SPACE CHARTER IS ALLOWED.
// IF YOU WANT OTHER SYMBOLS, WRITE YOUR OWN AND CREATE A NEW TABLE JUST LIKE THE
// OTHER CHARTERS
char phrase[] = "MADE IN ITALY WITH LOVE ";

void loop() {
  game(); //starting the game
}

void game() {
  for (char c = 0; phrase[c] != '\0'; c++) {
    for (int col = MATRIX_COL - 1; col >= 0; col--) { // we start to display the charter matrix from right to left
      for (byte i = 0; i < MATRIX_COL; i++) { //put the charter into the matrixState
        for (byte j = 0; j < MATRIX_ROW; j++) { //as usual
          if (i >= col) { //if the number of col(i) is higher than the scrolling col, we show the correct charter according to charterToShow var.
            writeCharter(phrase[c], i, j, col);
          } else { //else, if col (i) is less than col, we shift the matrixState
            matrixState[j][i] = matrixState[j][i + 1];
          }
        }

      }
      delay(100);
    }
  }

  delay(200);
  endGame(); //go to the end of the game

}

void writeCharter(char charterToShow, byte i, byte j, byte col) {
  if (charterToShow == 'A') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(A[j][i - col]));
  }
  else if (charterToShow == 'B') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(B[j][i - col]));
  }
  else if (charterToShow == 'C') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(C[j][i - col]));
  }
  else if (charterToShow == 'D') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(D[j][i - col]));
  }
  else if (charterToShow == 'E') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(E[j][i - col]));
  }
  else if (charterToShow == 'F') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(F[j][i - col]));
  }
  else if (charterToShow == 'G') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(G[j][i - col]));
  }
  else if (charterToShow == 'H') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(H[j][i - col]));
  }
  else if (charterToShow == 'I') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(I[j][i - col]));
  }
  else if (charterToShow == 'L') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(L[j][i - col]));
  }
  else if (charterToShow == 'M') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(M[j][i - col]));
  }
  else if (charterToShow == 'N') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(N[j][i - col]));
  }
  else if (charterToShow == 'O') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(O[j][i - col]));
  }
  else if (charterToShow == 'P') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(P[j][i - col]));
  }
  else if (charterToShow == 'Q') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(Q[j][i - col]));
  }
  else if (charterToShow == 'R') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(R[j][i - col]));
  }
  else if (charterToShow == 'S') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(S[j][i - col]));
  }
  else if (charterToShow == 'T') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(T[j][i - col]));
  }
  else if (charterToShow == 'U') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(U[j][i - col]));
  }
  else if (charterToShow == 'V') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(V[j][i - col]));
  }
  else if (charterToShow == 'Z') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(Z[j][i - col]));
  }
  else if (charterToShow == 'K') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(K[j][i - col]));
  }
  else if (charterToShow == 'X') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(X[j][i - col]));
  }
  else if (charterToShow == 'Y') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(Y[j][i - col]));
  }
  else if (charterToShow == 'J') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(J[j][i - col]));
  }
  else if (charterToShow == 'W') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(W[j][i - col]));
  }
  else if (charterToShow == ' ') { //SYMBOLS FOR SPACE
    matrixState[j][i] = (bool*)pgm_read_byte(&(SPACE[j][i - col]));
  }

}

void endGame() {
  //reset all game variables to the start condition
  clearMatrix();
  showKeyChaininoFace();
  delay(500);
  clearMatrix();
  delay(300);

  //check for programming mode
  if (!digitalRead(BUTTON_B) && !digitalRead(BUTTON_A)) {
    //programming mode
    programmingMode();
  }

  goSleep(); //sleep to reduce power
  clearMatrix(); //clear all the LEDs
  showKeyChaininoFace(); //show KeyChaininoFace
  delay(500);
}
void programmingMode() {
  while (1) {
    fullMatrix();
    delay(300);
    clearMatrix();
    delay(300);
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


void goSleep() {
  //going sleep to reduce power consuming

  power_timer0_disable(); //disable Timer 0
  power_timer1_disable(); //disable Timer 1

  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      pinMode(pins[connectionMatrix[i][j][0]], INPUT); //set both positive pole and negative pole
      pinMode(pins[connectionMatrix[i][j][1]], INPUT); // to INPUT in order to turn OFF the LED
    }
  }


  //enable interrupt buttons to allow wakeup from button interrupts
  bitSet(EIMSK, INT0); //enable interrupt button B - INT0
  bitSet(EIMSK, INT1); //enable interrupt button A - INT1

  // Disable the USB interface
  bitClear(USBCON, USBE);

  // Disable the VBUS transition enable bit
  bitClear(USBCON, VBUSTE);

  // Disable the VUSB pad
  bitClear(USBCON, OTGPADE);

  // Freeze the USB clock
  bitClear(USBCON, FRZCLK);

  // Disable USB pad regulator
  bitClear(UHWCON, UVREGE);

  // Clear the IVBUS Transition Interrupt flag
  bitClear(USBINT, VBUSTI);

  // Physically detact USB (by disconnecting internal pull-ups on D+ and D-)
  bitSet(UDCON, DETACH);

  power_usb_disable();

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  bool falsePush = true;

  while (falsePush) {
    if (!digitalRead(BUTTON_B) && !digitalRead(BUTTON_A)) { //until all the two buttons are pressend
      power_timer0_enable(); //enable Timer 0 in order to enable delay() function
      delay(2000);
      if (!digitalRead(BUTTON_B) && !digitalRead(BUTTON_A)) {
        falsePush = false;
      } else {
        power_timer0_disable();
        sleep_mode();
      }
    } else {
      sleep_mode();
    }
  }

  //disable interrupt buttons after sleep
  bitClear(EIMSK, INT0); //enable interrupt button B - INT0
  bitClear(EIMSK, INT1); //enable interrupt button A - INT1

  power_timer0_enable(); //enable Timer 0
  power_timer1_enable(); //enable Timer 1

  power_usb_enable();

  USBDevice.attach();
  // delay(100);

}

