/*************************************************************************
DICE FOR KEYCHAININO www.keychainino.com

created by Hersh Burston with lots of help from Alessandro Matera

2019/01/08 : modified by Frederic Ivsic to :

1) Reduce size of the program by using one array of 6 elements of 3x3 matrix items

Program size: 3�894 bytes (used 48% of a 8�192 byte maximum) (5,81 secs)
Minimum Memory Usage: 197 bytes (38% of a 512 byte maximum)

2) Show players turn with flashing led
3) Animation showing rolling dice
4) Auto sleep after laps of inactivity
5) Restore last dices when wake up after sleep

* *************************************************************************/

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <Entropy.h>	 //Random number generator library - seems better than Arduino random number generator

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

const byte flashDelayCONSTANT = 24;	// flashing led frequency
byte flashDelayCounter = 0;
volatile byte ledFlash = 0;

const int autoSleepCONSTANT = 500;	// delay before sleep
volatile int autoSleepCounter = 0;

volatile boolean gameStarted = false;

const byte numberLedANIMATION = 9;

// last dice number stored in EEPROM setting struct before going to sleep
struct settings_t
{
	byte saveDiceOne;
	byte saveDiceTwo;
	byte saveRollTurn;
} settings;


//KeyChainino "smileFace" stored in FLASH in order to reduce RAM size
const PROGMEM bool KeyChaininoFace[MATRIX_ROW][MATRIX_COL] =
{
	{0, 0, 0, 0, 0, 0},
	{0, 0, 1, 1, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{1, 0, 0, 0, 0, 1},
	{0, 1, 1, 1, 1, 0}
};

//KeyChainino "switch Off" stored in FLASH in order to reduce RAM size
const PROGMEM bool KeyChaininoSleep[MATRIX_ROW][MATRIX_COL] =
{
	{0, 0, 1, 0, 0, 0},
	{0, 1, 1, 1, 0, 0},
	{1, 0, 1, 0, 1, 0},
	{1, 0, 0, 0, 1, 0},
	{0, 1, 1, 1, 0, 0}
};

// square animation ClockWise
const PROGMEM int8_t squareCW[numberLedANIMATION][2] =
{
	{0, 0}, {0, 1}, {0, 2}, {1, 2}, {2, 2}, {2, 1}, {2, 0}, {1, 0}, {1, 1}
};

// square animation ConterClockWise
const PROGMEM int8_t squareCCW[numberLedANIMATION][2] =
{
	{0, 0}, {1, 0}, {2, 0}, {2, 1}, {2, 2}, {1, 2}, {0, 2}, {0, 1}, {1, 1}
};


//NUMBERS for dice stored in FLASH in order to reduce RAM size
// One array of six small arrays of 3x3 bool
const PROGMEM bool diceTab[6][3][3] =
{
	{ // index 0, number One
		{0, 0, 0},
		{0, 1, 0},
		{0, 0, 0}
	},
	{ // index 1, number Two
		{0, 0, 1},
		{0, 0, 0},
		{1, 0, 0}
	},
	{ // index 2, number Three
		{0, 0, 1},
		{0, 1, 0},
		{1, 0, 0}
	},
	{ // index 3, number Four
		{1, 0, 1},
		{0, 0, 0},
		{1, 0, 1}
	},
	{ // index 4, number Five
		{1, 0, 1},
		{0, 1, 0},
		{1, 0, 1}
	},
	{ // index 5, number Six
		{1, 0, 1},
		{1, 0, 1},
		{1, 0, 1}
	}
};


ISR(TIM1_OVF_vect) {  // timer1 overflow interrupt service routine

	TCNT1 = 65405;

	// flashing led when game is started
	if (gameStarted) {
		flashDelayCounter++;
		if (flashDelayCounter > flashDelayCONSTANT) {
			ledFlash = 1 - ledFlash;
			flashDelayCounter = 0;
		}

		autoSleepCounter++;
		if (autoSleepCounter > autoSleepCONSTANT) {
			gameStarted = false;
			autoSleepCounter = 0;
		}
	}

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

ISR(PCINT0_vect) { //BUTTON A INTERRUPT
	//do nothing
}

ISR(INT0_vect) { //BUTTON B INTERRUPT
	//do nothing
}

void setup()
{
	setupCPU();
	showKeyChaininoFace(); //show KeyChainino smile face
	delay(1500);
	clearMatrix(); //clear the Matrix
	gameStarted = true;
}

void setupCPU(){
	Entropy.Initialize();        //Initialize the random number generator
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

}

void loop()
{
	if ((digitalRead(BUTTON_A) == LOW) && (digitalRead(BUTTON_B) == LOW))  // press both buttons together to restart
	{
    // nothing
	}

	// AutoSleep if no activity, cf ISR counter
	if (gameStarted == false)
	{
		showKeyChaininoSleep(); //show KeyChainino switch off
		delay(500);
		clearMatrix();		      // clear the Matrix
		saveDicesAndPlayer();		// keep last dices number and turn player in eeprom
		goSleep();
    // on wake up, load display with last dice number
    restoreDicesAndPlayer();
    clearMatrix();
    showDices();
    delay(200);
    gameStarted = true;
    autoSleepCounter = 0;
	}


	if ((digitalRead(BUTTON_A) == LOW) && (rollTurn == 0))      //first player roll the dice
	{
		autoSleepCounter = 0;
		delay(200);
		clearMatrix();
		rollSquareAnimation(rollTurn);	// animation rotation CW
		clearMatrix();
		computeDiceNumber();
		showDices();
		rollTurn = 1;
	}

	if ((digitalRead(BUTTON_B) == LOW) && (rollTurn == 1))    //second player roll the dice
	{
		autoSleepCounter = 0;
		delay(200);
		clearMatrix();
		rollSquareAnimation(rollTurn);  // animation rotation CCW
		clearMatrix();
		computeDiceNumber();
		showDices();
		rollTurn = 0;
	}

	if (rollTurn == 1) {              // show on bottom line which player has to roll the dice, with a flashing led
		matrixState[4][1] = ledFlash;
	}
	else {
		matrixState[4][4] = ledFlash;
	}
}


/************************************************************************/
/* launch randow generation                                            */
/************************************************************************/
void computeDiceNumber(){
	randNumberDiceOne = Entropy.random(1, 7);              //use random(1, 7) for Arduino random number generator
	randNumberDiceTwo = Entropy.random(1, 7);              //use random(1, 7) for Arduino random number generator
}


/************************************************************************/
/*  Animation based on a sequence of led switched on according to       */
/*  array squareCW or swuareCCW											                    */
/************************************************************************/
void rollSquareAnimation(byte cw) {
	byte x, y;
	for (byte i = 0; i < numberLedANIMATION; i++)
	{
		if (cw == 0)
		{
			x = (byte*)pgm_read_byte(&(squareCW[i][0]));
			y = (byte*)pgm_read_byte(&(squareCW[i][1]));
		}
		else
		{
			x = (byte*)pgm_read_byte(&(squareCCW[i][0]));
			y = (byte*)pgm_read_byte(&(squareCCW[i][1]));
		}
		matrixState[x][y] = 1;
		matrixState[x][y + 3] = 1;
		delay(50);
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


//going sleep to reduce power consuming after a period of non activity
void goSleep() {
	//enable interrupt buttons to allow wakeup from button interrupts
	bitSet(GIMSK, INT0); //enable interrupt pin 8 - button B - INT0
	bitSet(PCMSK0, PCINT6); //enable interrupt pin 6 - button A - PCINT6
	power_timer0_disable(); //disable Timer 0
	power_timer1_disable(); //disable Timer 1
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	while (digitalRead(BUTTON_B) || digitalRead(BUTTON_A)) { //until all the two buttons are pressed
		sleep_mode();
	}
	//disable interrupt buttons after sleep
	bitClear(GIMSK, INT0); //disable interrupt pin 8 - button B - INT0
	bitClear(PCMSK0, PCINT6); //disable interrupt pin 6 - button A - PCINT6
	power_timer0_enable(); //enable Timer 0
	power_timer1_enable(); //enable Timer 1
}

// smiling face
void showKeyChaininoFace() {
	for (byte i = 0; i < MATRIX_ROW; i++) {
		for (byte j = 0; j < MATRIX_COL; j++) {
			matrixState[i][j] = (bool*)pgm_read_byte(&(KeyChaininoFace[i][j])); //here we read the matrix from FLASH
		}
	}
}


// switch off drawing
void showKeyChaininoSleep() {
	for (byte i = 0; i < MATRIX_ROW; i++) {
		for (byte j = 0; j < MATRIX_COL; j++) {
			matrixState[i][j] = (bool*)pgm_read_byte(&(KeyChaininoSleep[i][j])); //here we read the matrix from FLASH
		}
	}
}
/************************************************************************/
/* show 2 dices on top of screen                                        */
/************************************************************************/
void showDices() {
	showOneDiceAt(0, 0, randNumberDiceOne);
	showOneDiceAt(0, 3, randNumberDiceTwo);
}

/************************************************************************/
/* show one dice number at position x,y                                 */
/************************************************************************/
void showOneDiceAt(byte row, byte col, byte n) {
	// n = dice random number (1 to 6)
	byte nx = n - 1;	// for array index 0 to 5

	for (byte i = 0; i < 3; i++)
	{
		for (byte j = 0; j < 3; j++)
		{
			matrixState[row + i][col + j] = (bool*)pgm_read_byte(&(diceTab[nx][i][j]));
		}
	}
}

/************************************************************************/
/* Save dices & player turn                                             */
/************************************************************************/
void saveDicesAndPlayer() {
	// save to eeprom
	settings.saveDiceOne = randNumberDiceOne;
	settings.saveDiceTwo = randNumberDiceTwo;
	settings.saveRollTurn = rollTurn;
	eeprom_write_block((const void*)&settings, (void*)0, sizeof(settings));
}

/************************************************************************/
/* restore previous dices & players                                     */
/************************************************************************/
void restoreDicesAndPlayer() {
	// load from eeprom
	eeprom_read_block((void*)&settings, (void*)0, sizeof(settings));
	// default value for settings if eeprom read return odd value
	if (settings.saveDiceOne < 1 || settings.saveDiceOne > 6) settings.saveDiceOne = 1;
	if (settings.saveDiceTwo < 1 || settings.saveDiceTwo > 6) settings.saveDiceTwo = 1;
	if (settings.saveRollTurn < 0 || settings.saveRollTurn > 1) settings.saveDiceTwo = 0;
	randNumberDiceOne = settings.saveDiceOne;
	randNumberDiceTwo = settings.saveDiceTwo;
	rollTurn = settings.saveRollTurn;
}
