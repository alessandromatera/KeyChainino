/***************************************************************************/
/** TAP IN TIME v1.2 - Sketch for KEYCHAININO -  www.keychainino.com *******/
/***************************************************************************/
/***************************************************************************/
/***** created by Riccardo Michele Licciardello - 10/01/2016****************/
/***************************************************************************/
/*The purpose of this game is push at time a button (left or right) when
   random falling notes arrive in the last line of the led matrix. When a button
   is correctly pushed, the score increase and a coloumn led lights.
   If the button isn't pushed or the button is pushed in wrong time, the game ends.

  /*Changelog TAP IN TIME v1.2
     -Improve gameplay
     -Minor optimizations in animation
*/

/*Changelog TAP IN TIME v1.1
   -Bug fix when a button is pressed, now the game is more accurate with falling note
   -New animations and texts
   -Added 3 life. Now you can wrong 3 times!
   -Added record, max point you have achieved in game. *NOTE: if you remove battery in keychainino, record will be erased!
   -More optimizations
*/


#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/power.h>

#define MATRIX_ROW 5
#define MATRIX_COL 6
#define PIN_NUMBER 7

#define BUTTON_A 6 //pin 6 - PCINT6
#define BUTTON_B 8 //pin 8 - INT0

byte punteggio = 0;     //variable score
byte record = 0;        //variable record, max point achieved in game
byte punteggioVEL = 0;  //variable used to control velocity of falling notes
byte YnotaSX = 0;       //Y position of left note
byte YnotaDX = 0;       //Y position of right note
byte velocitaNota = 10; //initial velocity of falling notes
byte timerNotaDX = 0;   //timer used to right notes
byte timerNotaSX = 0;   //timer used to left notes
byte NotaRandom;        //falling note (it's random: left or right)
byte vite = 2;          //variable life
boolean FlagNotaSX = 0; //flag that indicate left note is falling
boolean FlagNotaDX = 0; //flag that indicate right note is falling
boolean FlagFineNota = 0; //flag that indicate the end of falling note (when we have to push button)
boolean FlagFineGioco = 0; //flag end of game
boolean FlagPulsanteDX = 0; //flag that indicate the right button was pushed
boolean FlagPulsanteSX = 0; //flag that indicate the left button was pushed
boolean FlagAbilitaPunti = 0; //flag that indicate the possibility of update score

char titolo[] = "TAP IN TIME ";   //Scrolling text at start
char vite2[] = "2 LIFES ";        //Scrolling text when i lost first life
char vita1[] = "1 LIFE ";         //Scrolling text when i lost second life
char Punti[12];                   //Final text with score


const byte pins[PIN_NUMBER] = {0, 1, 2, 3, 7, 9, 10}; //the number of the pin used for the LEDs in ordered

const byte connectionMatrix[MATRIX_ROW][MATRIX_COL][2] = { //the matrix that shows the LEDs pin connections. First Value is the Anode, second is the Catode
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

//********************************************//
//*************LETTERS************************//
//********************************************//

const PROGMEM bool SPACE[MATRIX_ROW][MATRIX_COL]  = {
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0}
};

const PROGMEM bool T[MATRIX_ROW][MATRIX_COL] = {
  {0, 1, 1, 1, 1, 1},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 0, 1, 0, 0}
};

const PROGMEM bool P[MATRIX_ROW][MATRIX_COL] = {
  {0, 1, 1, 1, 0, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 1, 1, 0, 0},
  {0, 1, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0}
};

const PROGMEM bool M[MATRIX_ROW][MATRIX_COL] = {
  {0, 1, 0, 0, 0, 1},
  {0, 1, 1, 0, 1, 1},
  {0, 1, 0, 1, 0, 1},
  {0, 1, 0, 0, 0, 1},
  {0, 1, 0, 0, 0, 1}
};

const PROGMEM bool I[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 1, 0, 0},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 0, 1, 0, 0},
  {0, 0, 0, 1, 0, 0}
};

const PROGMEM bool F[MATRIX_ROW][MATRIX_COL] = {
  {0, 1, 1, 1, 1, 0},
  {0, 1, 0, 0, 0, 0},
  {0, 1, 1, 1, 0, 0},
  {0, 1, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0}
};

const PROGMEM bool L[MATRIX_ROW][MATRIX_COL] = {
  {0, 1, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0},
  {0, 1, 1, 1, 1, 0}
};

const PROGMEM bool N[MATRIX_ROW][MATRIX_COL] = {
  {0, 1, 0, 0, 1, 0},
  {0, 1, 1, 0, 1, 0},
  {0, 1, 0, 1, 1, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 0, 0, 1, 0}
};

const PROGMEM bool A[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 1, 1, 0, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 1, 1, 1, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 0, 0, 1, 0}
};

const PROGMEM bool E[MATRIX_ROW][MATRIX_COL] = {
  {0, 1, 1, 1, 0, 0},
  {0, 1, 0, 0, 0, 0},
  {0, 1, 1, 1, 0, 0},
  {0, 1, 0, 0, 0, 0},
  {0, 1, 1, 1, 0, 0}
};

const PROGMEM bool S[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 1, 1, 1, 0},
  {0, 1, 0, 0, 0, 0},
  {0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 1, 0},
  {0, 1, 1, 1, 0, 0}
};

const PROGMEM bool O[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 1, 1, 0, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 0, 1, 1, 0, 0}
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

const PROGMEM bool R[MATRIX_ROW][MATRIX_COL] = {
  {0, 1, 1, 1, 0, 0},
  {0, 1, 0, 0, 1, 0},
  {0, 1, 1, 1, 0, 0},
  {0, 1, 0, 1, 0, 0},
  {0, 1, 0, 0, 1, 0}
};

const PROGMEM bool duepunti[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0}
};
/********************************************/
/************SAD FACE************************/
/********************************************/

const PROGMEM bool KeyChaininoFacciaTriste[MATRIX_ROW][MATRIX_COL] = {
  {0, 0, 0, 0, 0, 0},
  {0, 0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 1, 1, 1, 1, 0},
  {1, 0, 0, 0, 0, 1}
};

/**************************************************************/
/****************INTERRUPT SERVICE ROUTINE*********************/
/**************************************************************/

ISR(TIM1_OVF_vect) {  // timer1 overflow interrupt service routine
  cli(); //disable interrupt
  TCNT1 = 65406;

  if (FlagFineGioco == 0) //if game isn't finished
  {
    DiscesaNote();      //start generate falling notes
    ControlloPulsanti();    //routine that control if the button is pushed
  }
  // THIS PART IS USED TO UPDATE THE CHARLIEPLEXING LEDS MATRIX
  // YOU CAN JUST DON'T CARE ABOUT THIS PART
  // BECAUSE YOU CAN CODE LIKE A STANDARD MATRIX BY MANIPULATING THE
  // VALUE OF THE matrixState MATRIX

  //check from matrixState which LED to turn ON or OFF
  for (byte i = 0; i < MATRIX_ROW; i++)
  {
    for (byte j = 0; j < MATRIX_COL; j++)
    {
      if (matrixState[i][j] == 1)
      { //turn on LED with 1 in matrixState
        pinMode(pins[connectionMatrix[i][j][0]], OUTPUT); //set positive pole to OUTPUT
        pinMode(pins[connectionMatrix[i][j][1]], OUTPUT); //set negative pole to OUTPUT
        digitalWrite(pins[connectionMatrix[i][j][0]], HIGH); //set positive pole to HIGH
        digitalWrite(pins[connectionMatrix[i][j][1]], LOW); //set negative pole to LOW
        delayMicroseconds(250);
        pinMode(pins[connectionMatrix[i][j][0]], INPUT); //set both positive pole and negative pole
        pinMode(pins[connectionMatrix[i][j][1]], INPUT); // to INPUT in order to turn OFF the LED
      }
      if (matrixState[i][j] == 0)  //turn off LED with 0 in matrixState
      {
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
  TCCR1B = 0;    // set entire TCCR1B register to 0

  // enable Timer1 overflow interrupt:
  bitSet(TIMSK1, TOIE1);

  // preload timer 65536 - (8000000 / 1024 / 60)
  TCNT1 = 65406;

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
  FlagFineNota = 0;
  FlagFineGioco = 1;
  MostraTesto(titolo, 80);    //At start, a scrolling text appear (title of game)
  clearMatrix();
  AnimazioneAvvio();    //A little animation before the start of game
  FlagFineGioco = 0;  //Game started
}

//Loop routine, test if the game ended and if the buttons are pushed
void loop()
{
  PremiPulsanti();    //test the buttons
  if  (FlagFineGioco == 1)
    FineGioco();
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

//here we set or clear a single bit on the matrixState. We use this funciton in order
//to really set or clear the matrix's bit when an interrupt occours. To do that we disable the
//interrupt -> set or clear the bit -> enable interrupt

void setMatrixStateBit(byte i, byte j) {
  cli();
  matrixState[i][j] = 1;
  sei();
}
void clearMatrixStateBit(byte i, byte j) {
  cli();
  matrixState[i][j] = 0;
  sei();
}

//show the number and letters according to the matrix number and matrix text
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
  else if (charterToShow == 'I') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(I[j][i - col]));
  }
  else if (charterToShow == 'P') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(P[j][i - col]));
  }
  else if (charterToShow == 'M') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(M[j][i - col]));
  }
  else if (charterToShow == 'N') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(N[j][i - col]));
  }
  else if (charterToShow == 'E') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(E[j][i - col]));
  }
  else if (charterToShow == 'T') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(T[j][i - col]));
  }
  else if (charterToShow == 'A') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(A[j][i - col]));
  }
  else if (charterToShow == 'L') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(L[j][i - col]));
  }
  else if (charterToShow == 'F') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(F[j][i - col]));
  }
  else if (charterToShow == 'S') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(S[j][i - col]));
  }
  else if (charterToShow == 'O') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(O[j][i - col]));
  }
  else if (charterToShow == 'R') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(R[j][i - col]));
  }
  else if (charterToShow == 'C') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(C[j][i - col]));
  }
  else if (charterToShow == 'D') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(D[j][i - col]));
  }
  else if (charterToShow == ':') {
    matrixState[j][i] = (bool*)pgm_read_byte(&(duepunti[j][i - col]));
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
  while (digitalRead(BUTTON_B) || digitalRead(BUTTON_A)) { //until all the two buttons are pressend
    sleep_mode();
  }
  //disable interrupt buttons after sleep
  bitClear(GIMSK, INT0); //disable interrupt pin 8 - button B - INT0
  bitClear(PCMSK0, PCINT6); //disable interrupt pin 6 - button A - PCINT6
  power_timer0_enable(); //enable Timer 0
  power_timer1_enable(); //enable Timer 1
}


/***************************************************/
/**************ROUTINE THAT SHOW TEXTS**************/
/***************************************************/

void MostraTesto(char phrase[], int ritardo) //Show texts according the array and the "delay time" passed
{
  clearMatrix();
  for (char c = 0; phrase[c] != '\0'; c++)
  {
    for (int col = MATRIX_COL - 1; col >= 0; col--) // we start to display the charter matrix from right to left
    {
      for (byte i = 0; i < MATRIX_COL; i++) //put the charter into the matrixState
      {
        for (byte j = 0; j < MATRIX_ROW; j++)  //as usual
        {
          if (i >= col) //if the number of col(i) is higher than the scrolling col, we show the correct charter according to charterToShow var.
          {
            writeCharter(phrase[c], i, j, col);
          }
          else //else, if col (i) is less than col, we shift the matrixState
          {
            matrixState[j][i] = matrixState[j][i + 1];
          }
        }

      }
      delay(50);
    }
  }
  delay(ritardo);
}


void AnimazioneAvvio()  //Show little animation before the game starts
{
  delay(60);
  setMatrixStateBit(0, 2);
  setMatrixStateBit(0, 3);
  delay(70);
  setMatrixStateBit(0, 1);
  setMatrixStateBit(0, 4);
  delay(70);
  setMatrixStateBit(0, 0);
  setMatrixStateBit(0, 5);
  delay(70);
  setMatrixStateBit(1, 0);
  setMatrixStateBit(1, 5);
  delay(70);
  setMatrixStateBit(2, 0);
  setMatrixStateBit(2, 5);
  delay(90);
  setMatrixStateBit(3, 0);
  setMatrixStateBit(3, 5);
  delay(90);
  setMatrixStateBit(4, 0);
  setMatrixStateBit(4, 5);
  delay(90);
  clearMatrixStateBit(0, 2);
  clearMatrixStateBit(0, 3);
  setMatrixStateBit(4, 1);
  setMatrixStateBit(4, 4);
  delay(90);
  clearMatrixStateBit(0, 1);
  clearMatrixStateBit(0, 4);
  setMatrixStateBit(4, 2);
  setMatrixStateBit(4, 3);
  delay(300);
  clearMatrix();
}

/*************************************************/
/***********BUTTONS ROUTINES**********************/
/*************************************************/

//Test the button and set a flag when one button is pushed
void PremiPulsanti()
{
  if (!digitalRead(BUTTON_B)) //test right button
  {
    delay(50);
    if (!digitalRead(BUTTON_B))
      FlagPulsanteDX = 1; //flag right button is pushed
  }
  else
  {
    delay(20);
    CancellaDX(); //clear right coloumn of led
  }

  if (!digitalRead(BUTTON_A)) //test left button
  {
    delay(50);
    if (!digitalRead(BUTTON_A))
      FlagPulsanteSX = 1; //flag left button is pushed
  }
  else
  {
    delay(20);
    CancellaSX();    //clear left coloumn of led
  }
}

//test flags button for increase score or finish game
void ControlloPulsanti()
{

  if (FlagPulsanteDX == 1)
  {
    if (((YnotaDX == 5) || (YnotaDX == 0)) && (YnotaSX == 0)) //if Y position of right falling note is 5 or 0 (in the last line of matrix led)
    { //and left note is clear
      FlagPulsanteDX = 0;
      MostraRispostaDX(); //lights right coloumn of led
      AumentaPunteggio(); //update score
    }
    else
      FlagFineGioco = 1; //if button was pushed at wrong time, the game ends
  }

  if (FlagPulsanteSX == 1)
  {
    if (((YnotaSX == 5) || (YnotaSX == 0)) && (YnotaDX == 0)) //same things with left side
    {
      FlagPulsanteSX = 0;
      MostraRispostaSX();
      AumentaPunteggio();
    }
    else
      FlagFineGioco = 1;
  }
}

//routine that update score
void AumentaPunteggio()
{
  if (FlagAbilitaPunti == 1)
  {
    FlagFineNota = 0;
    FlagAbilitaPunti = 0; //disable possibility of updating again score
    punteggio++;    //increase score
    punteggioVEL++;   //increase temporaly score for boost velocity
  }
}

//Lights left coloumn of led
void MostraRispostaSX()
{
  byte visual;

  if (vite == 2)  //test how many life left. They will change animation
    visual = 0; //when a button is pressed
  else if (vite == 1)
    visual = 1;
  else if (vite == 0)
    visual = 3;

  for (byte i = visual; i < MATRIX_ROW; i++)
    setMatrixStateBit(i, 0);
}

//Lights right coloumn of led
void MostraRispostaDX()
{
  byte visual;

  if (vite == 2)  //test how many life left. They will change animation
    visual = 0;   //when a button is pressed
  else if (vite == 1)
    visual = 1;
  else if (vite == 0)
    visual = 3;

  for (byte i = visual; i < MATRIX_ROW; i++)
    setMatrixStateBit(i, 5);
}

//Clear right coloumn of led
void CancellaDX()
{
  for (byte i = 0; i < MATRIX_ROW; i++)
    clearMatrixStateBit(i, 5);
}

//Clear left coloumn of led
void CancellaSX()
{
  for (byte i = 0; i < MATRIX_ROW; i++)
    clearMatrixStateBit(i, 0);
}

/*************************************************/
/***********FALLING NOTES ROUTINES****************/
/*************************************************/

//Generate random falling notes and finish game if a button isn't correctly pressed
void DiscesaNote()
{
  if ((FlagNotaDX == 0) && (FlagNotaSX == 0)) //if no note was created, create left (0) or right (1) note
    NotaRandom = random(0, 2);

  if ((FlagFineNota == 1) && ((YnotaDX == 1) || (YnotaSX == 1))) //if the note has fallen and no button was pressed, the game ends
    FlagFineGioco = 1;

  if (NotaRandom == 0) //if NotaRandom=0, set flag left note
    FlagNotaSX = 1;
  else
    FlagNotaDX = 1; //if NotaRandom=1,set flag right note

  if (FlagNotaSX == 1) //if flag left note is 1, generate left note
    CompareNotaSX();

  if (FlagNotaDX == 1) //if flag right note is 1, generate right note
    CompareNotaDX();

}

//Generate left note
void CompareNotaSX()
{
  if (timerNotaSX > velocitaNota)
  {
    timerNotaSX = 0;
    if (YnotaSX != 0)
    {
      ControlloVelocita();    //test if temporary score is 10 and boost velocity
      clearMatrixStateBit(YnotaSX - 1, 1);
      clearMatrixStateBit(YnotaSX - 1, 2);
    }
    if (YnotaSX < 5)
    {
      setMatrixStateBit(YnotaSX, 1);
      setMatrixStateBit(YnotaSX, 2);
      YnotaSX++;
      if (YnotaSX == 4) //when note is in the end of matrix, set flag end note
      {
        FlagFineNota = 1;
        FlagAbilitaPunti = 1; //and set the possibility of updating score
      }
    }
    else
    {
      YnotaSX = 0;
      clearMatrixStateBit(YnotaSX + 4, 1);
      clearMatrixStateBit(YnotaSX + 4, 2);
      FlagNotaSX = 0;
    }
  }
  else
    timerNotaSX++;
}

//Generate right note
void CompareNotaDX()
{
  if (timerNotaDX > velocitaNota)
  {
    timerNotaDX = 0;
    if (YnotaDX != 0)
    {
      ControlloVelocita();
      clearMatrixStateBit(YnotaDX - 1, 3);
      clearMatrixStateBit(YnotaDX - 1, 4);
    }

    if (YnotaDX < 5)
    {
      setMatrixStateBit(YnotaDX, 3);
      setMatrixStateBit(YnotaDX, 4);
      YnotaDX++;
      if (YnotaDX == 4)
      {
        FlagFineNota = 1;
        FlagAbilitaPunti = 1;
      }
    }
    else
    {
      YnotaDX = 0;
      clearMatrixStateBit(YnotaDX + 4, 3);
      clearMatrixStateBit(YnotaDX + 4, 4);
      FlagNotaDX = 0;
    }
  }
  else
    timerNotaDX++;
}


//Routine boost velocity
void ControlloVelocita()
{
  if (punteggioVEL == 10)
  {
    punteggioVEL = 0;
    if (velocitaNota > 3)
      velocitaNota--;     //To increase velocity of falling notes, variable "velocitaNota" has to decrease
  }
}

/*************************************************/
/************ENDS OF GAME ROUTINES****************/
/*************************************************/

//if flag end game is set, this routine will be performed
void FineGioco()
{
  if (vite > 0) //if you have life
  {
    fullMatrix();
    delay(100);
    if (vite == 2)  //show life remain
      MostraTesto(vite2, 50);
    else
      MostraTesto(vita1, 50);

    vite--;
    FlagFineGioco = 0;
    FlagAbilitaPunti = 0;
    FlagFineNota = 0;
    timerNotaDX = 0;
    timerNotaSX = 0;
    FlagPulsanteDX = 0;
    FlagPulsanteSX = 0;
    YnotaDX = 0;
    YnotaSX = 0;
  }
  else    //if you haven't life, end game
  {
    fullMatrix();
    delay(100);
    String temp = String(punteggio);
    String temp2 = "POINTS:" + temp + " ";
    temp2.toCharArray(Punti, 12);   //merge score number with text "POINTS"
    MostraTesto(Punti, 150);        //Show the result
    delay(100);
    MostraRecord();
    delay(100);
    MostraKeyChaininoFacciaTriste();    //show a sad face
    delay(700);
    goSleep();    //sleep Keychainino
    ResettaGioco();   //set variable to default values
  }
}

//Routine that shows record point and update it
void MostraRecord()
{
  if (punteggio > record)
    record = punteggio;
  String temp = String(record);
  String temp2 = "RECORD:" + temp + " ";
  temp2.toCharArray(Punti, 12);   //merge score number with text "POINTS"
  MostraTesto (Punti, 150);
}


//Show a sad face
void MostraKeyChaininoFacciaTriste() {
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = (bool*)pgm_read_byte(&(KeyChaininoFacciaTriste[i][j])); //here we read the matrix from FLASH
    }
  }
}


//Reset the game when Keychainino wake up. Set variable to default values
void ResettaGioco()
{
  cli();
  clearMatrix();
  delay(300);
  punteggio = 0;
  punteggioVEL = 0;
  YnotaSX = 0;
  YnotaDX = 0;
  velocitaNota = 10;
  timerNotaDX = 0;
  timerNotaSX = 0;
  FlagNotaSX = 0;
  FlagNotaDX = 0;
  FlagFineNota = 0;
  FlagPulsanteDX = 0;
  FlagPulsanteSX = 0;
  FlagAbilitaPunti = 0;
  vite = 2;
  sei();
  FlagFineGioco = 1;
  MostraTesto(titolo, 80);
  AnimazioneAvvio();
  FlagFineGioco = 0;
}

