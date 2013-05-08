/* Microprocessor Applications - M07CDE          */
/* Practical Assignment                          */
/* Lecturer: Dr. Andrew Tickle                   */
/* Group:                                        */
/* Andre de Castro Hermuche - Student ID 4812575 */
/* Caio Amaral              - Student ID         */
/* Rafael Capelozza Mazon   - Student ID         */


// PIC configurations
#pragma config OSC = HS            // HS oscillator
#pragma config WDT = OFF           // Watcdog timer off
#pragma config MCLRE = ON          // Masterhttp://www.yworks.com/en/products_download.php clear reset On
#pragma config DEBUG = OFF         // Debug off
#pragma config LVP = OFF           // Low voltage programming off
#pragma interrupt interrupt_treat


// Definitions used in this code
#define SWITCH_KEY PORTBbits.RB0
#define PRESSED 0
#define DELAY_DEBOUNCE 1
#define DELAY_PRESS_COUNT 100 
#define DELAY_LED_UP 1000
#define DELAY_LED_UP1 100
#define DOT_MAX_COUNT 3
#define DASH_MAX_COUNT 10
#define DELAY_INIT 1000000
#define MAX_TIMER_COUNTER 300


// Header files
#include <p18f4520.h>    // Header files of PIC18F4520
#include <delays.h>      // Delays
#include <portb.h>       // Header files required for port B interrupts
#include <string.h>
#include <stdlib.h>
#include <usart.h>       // USART header file


// Prototypes of the functions created in this code
void interrup_treat (void);   // Treats the interruption
void INT0_ISR(void);          // Interruption Service Routine for INT0
void TMR0_ISR(void);          // Interruption Service Routine for TMR0 
void state_machine(void);     // State machine that rules the program
void flashLed(int);           // Flashes the LED bank
void exceptionHandler(int);   // Translates the Morse code into characters
void ledTest(void);           // Run a test in the LED bank
void manageWord(int);         // Creates a string containing dots, dashes and silence depending on the press of the button
int pushCounter(int);         // Counts the time while the button is pressed
char identifyClick(int);      // Differs dots, dashes and errors based on time count
short debounce(void);         // Debounce effect for the button
void soundDot(void);          // Generates the "dot" sound
void soundDash(void);         // Generates the "dash" sound
void usartCom(char);          // USART communication between the board and the computer


// Vectors for translating Morse code into characters
char alphabetVector[] = {'&','T','E','M','N','A','I','O','G','K','D','W','R','U','S','£','£','Q','Z','Y','C','X','B','J','P','£','L','£','F','V','H','0','9','$','8','£','£','$','7','$','£','$','£','$','£','£','6','1','£','£','£','$','.','£','$','2','£','$','£','3','£','4','5'};
int dotVector[] =  {2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62}; // Dot state vector
int dashVector[] =  {1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41,43,45,47,49,51,53,55,57,59,61}; // Dash state vector


// Global variables
int state = 0;			// State Variable
int count = 0;			// Timer Counter
char word[] = "******";         // Initializing the word string full of 'silences'
char dot = '.';			
char dash = '-';
char silence = '*'; 
int word_pointer = 0; 		// Pointer to actual position on vector word[]
char resultChar;		// result of conversion	
int stateConv;			// Used on conversion method
int nextState;			// Used on conversion method
int i;
	

/* Functions */

void interrupt_treat(void)
{
	if (INTCONbits.INT0IF == 1) // Was interrupt caused by INT0 ?
		INT0_ISR ();            // Yes , execute INT0 program
	if (INTCONbits.TMR0IF)	    // Check for TMR0 overflow
		TMR0_ISR();		        // Clear interrupt flag
}


void main (void)
{
	ADCON1 = 0x0F;              // Set Ports as Digital I/O rather than analogue
	TRISD = 0x00;               // Make Port D all output
	LATD = 0x00;                // Clear Port D
	TRISC = 0x00;
	TRISA = 0x00;
	LATA = 0x00;
	LATC = 0x00;
	TRISB = 0x01;
	INTCONbits.INT0IE = 1;      // Enable INT0 interrupts
	INTCONbits.INT0IF = 0;      // Clear INT0 Interupt flag
	T0CON = 0x47;               // Inicializate TMR0 (turned off)
	
	INTCONbits.GIE = 1;         // Enable global interrupts

	ledTest();
	count = 0;
	state = 0;                  // Inicializate on stand-by state

	while (1)                   // Repeat for ever until interrupted
	{
		state_machine();
		Delay10KTCYx(1);
	}
}

void ledTest(void){             // Flashes all the LEDs from the LED bank in a sequence using delays
	flashLed(6);
	Delay10KTCYx(DELAY_LED_UP);
	flashLed(5);
	Delay10KTCYx(DELAY_LED_UP);
	flashLed(4);
	Delay10KTCYx(DELAY_LED_UP);
	flashLed(3);
	Delay10KTCYx(DELAY_LED_UP);
	flashLed(2);
	Delay10KTCYx(DELAY_LED_UP);
	flashLed(1);
	Delay10KTCYx(DELAY_LED_UP);
	flashLed(0);
}

void flashLed(int id)           // Flash LED UP
{
	switch(id){
		case 0:
			LATD = 0x00;
			break;
		case 1:
			LATD = 0x01;
			break;
		case 2:
			LATD = 0x02;
			break;
		case 3:
			LATD = 0x04;
			break;
		case 4:
			LATD = 0x08;
			break;
		case 5:
			LATD = 0x10;
			break;
		case 6:
			LATD = 0x20;
			break;
		case 7:
			LATD = 0x40;
			break;
	}	
}

void TMR0_ISR(void){
	INTCONbits.TMR0IE = 0;           // Disable Interruptions while treating this interruptions
	INTCONbits.TMR0IF = 0;		

   	if(count<MAX_TIMER_COUNTER)	// TIMER UNDERFLOW
   	{
		count++;                     // Increment the acumulator Counter
	}
	if(count>=MAX_TIMER_COUNTER)	// TIMER OVERFLOW
	{
		exceptionHandler(1);	// TERMINATE CONVERSION
	}
	INTCONbits.TMR0IE = 1;
}


void INT0_ISR (void)                 // This is the interrupt service routine
{	
	short pressed = 0;               // Flag for the use of the debouncer
	int clicked_counter = 0;         
	char click = 'c';                // Iniciates type of click as error, for the worst case

	INTCONbits.INT0IE = 0;           // Disable Interruptions while treating this interruptions

	/* Debounce */
	pressed = debounce();
	if (pressed == 0){
		Delay10KTCYx(1);             // Delay to prevent multiple interruptions while clicking the button once
		INTCONbits.INT0IE = 1;       // Re-enable INT0 interrupts
		INTCONbits.INT0IF = 0;       // Clear INT0 flag before returning to main program
		return;                      // Means the button wasnt really pressed
	}
	/* -------- */

	T0CONbits.TMR0ON = 1;
	count = 0;                       // Cleaning the time interruption

	state = 1;			// Change the State to In-Conversion

	
	clicked_counter = pushCounter(1);
	click = identifyClick(clicked_counter);
	switch (click){                  // Here would concatenate dots or dashes on a string
		case 'a': 
			//Dot
			manageWord(0); 	// write a dot on vector word[]
			flashLed(4); 	// flag to user
			soundDot();	// sound to user
			break;
		case 'b':
			//Dash
			manageWord(1);	//write a dash on vector[]		
			flashLed(5);	// flag to user
			soundDash();	// sount to user
			break;
		case 'c':
			exceptionHandler(2);
			flashLed(1);        //Error
			break;
	}
	Delay10KTCYx(1);                 // Delay to prevent multiple interruptions while clicking the button once
	INTCONbits.INT0IE = 1;           // Re-enable INT0 interrupts
	INTCONbits.INT0IF = 0;           // Clear INT0 flag before returning to main program
}

void soundDot(){                     // Generates a specific sound for a dot, to be heard from the buzzer              
	int i;
	for(i=0;i<7;i++){                // The number of interactions "creates" the unique sound
		LATB = 0x04;
		Delay10KTCYx(10);
		
	}
	LATB = 0x00;
}

void soundDash(){                    // Generates a specific sound for a dash, to be heard from the buzzer
	int i;
	for(i=0;i<18;i++){               // The number of interactions "creates" the unique sound
		LATB = 0x04;
		Delay10KTCYx(10);
	}
	LATB = 0x00;
}

void usartCom(char answer){                        // The parameter here is the character translated from the Morse code
	int answerHex;
	answerHex = answer;                            // Here, the character is associated to a integer variable, in order to get its hex value
	TRISC = 0x00;                                  // PORT C will be used for the USART communication

	/* USART configuration for a 19200 baud rate */
	OpenUSART(USART_TX_INT_OFF & USART_RX_INT_OFF & USART_ASYNCH_MODE & USART_EIGHT_BIT & USART_CONT_RX & USART_BRGH_HIGH, 65);
	/* ----------------------------------------- */

	/* Sending serial data to the Hyperterminal in the pc */
	putrsUSART("The character typed was: \r\n");   // Here, a string is sent (this function only sends vectors)   
	WriteUSART(answerHex);                         // Here, the character is sent as a hex value as in the ASCII table (this function can send only hex values)
	putrsUSART("\r\n\n");
	Delay10KTCYx(200);
	/* -------------------------------------------------- */
}
	


void exceptionHandler(int id){

	/* Turning off interruptions */
	INTCONbits.INT0IE = 0;
	T0CONbits.TMR0ON = 0;
	/* ------------------------- */

	switch(id){                                        // Here we translate the Morse code into characters
		case 1:                                        // Each character has its own state, according to the Morse code table provided
			count = 0;                                 // Basically, this function goes through the table based on actual state and next state variables
			stateConv = 0;                             // It is regulated by two vectors (dot and dash), which contains the states related to them
			nextState = 0;                             // At the end, the position of the state is compared to another vector
			for (i=0;i<=5;i++){                        // Means that we are analysing the whole string of dots, dashes and silence
				if(word[i]=='.'){                      // If it is a dot, go to the next state based on the dot vector
					nextState = dotVector[stateConv];
					flashLed(4);    		// Flag with LED Orange
					soundDot();			// Flag with Sound
					Delay10KTCYx(DELAY_LED_UP1);
					flashLed(0);
					Delay10KTCYx(DELAY_LED_UP1);
					
				}
				else if(word[i]=='-'){                 // If it is a dash, go to the next state based on the dash vector
					nextState = dashVector[stateConv];
					flashLed(5);			// Flag with LED Yellow
					soundDash();			// Flag with sound
					Delay10KTCYx(DELAY_LED_UP1);
					flashLed(0);
					Delay10KTCYx(DELAY_LED_UP1);

				}
				else if(word[i]=='*'){                 // If it is a silence, go to the next interaction only
					flashLed(6);
					Delay10KTCYx(DELAY_LED_UP1);
					flashLed(0);
					Delay10KTCYx(DELAY_LED_UP1);
				}
				stateConv = nextState;                 // At the end, saves the current state (desired character)
			}

			resultChar = alphabetVector[stateConv];    // This vector contais all the characters, and from the state we can reach the desired one

			if(resultChar == '£' || resultChar == '$'){	// This will flag to the user if the conversion made have no correspondent character
				flashLed(1);
				Delay10KTCYx(DELAY_LED_UP);
				flashLed(0);
			}
			else {                                     // Conversion Successfull!
				flashLed(2);
				Delay10KTCYx(DELAY_LED_UP);
				flashLed(0);
			}

			usartCom(resultChar);                       // Send the character typed to the Hiperterminal via USART
		
			state=0;				    // return to state Stand-By
			manageWord(3);                              // Clean string
			break;
		case 2:						// Means that there was an error while user pressed the buttons
			state=0;				// Return to stand-by state
			flashLed(1);				// Flag to the user
			Delay10KTCYx(DELAY_LED_UP);		
			flashLed(0);				// Turn off led		
			manageWord(3);				// Clean the vector word[]
			break;
	}
}

void manageWord(int id){
	int i;
	switch (id){

		case 0: // Add dot
			/* WRITE STRING word[] MANIPULATION TO ADD A DOT */
			if(word_pointer <=5){
				word[word_pointer] = '.'; 
				word_pointer++;
			}
			else exceptionHandler(1); 	// if it is full, means the conversion is done
			break;

		case 1: // Add dash
			/* WRITE SRING word[] MANIPULATION TO ADD A DASH */
			if(word_pointer <=5){
				word[word_pointer] = '-';
				word_pointer++;
			}
			else exceptionHandler(1);	// if it is full means the conversion is done
			break;

		case 2: // Fill the string with 'silences' (Not necessare on this solution, but implemented)
			while(word_pointer<6){
				word_pointer++;
				word[word_pointer] = '*';
			}
			word_pointer=0;		// return pointer to zero
			break;

		case 3: // Clean the vector word[] for a new conversion
			for (i=0;i<=5;i++)	{
				word[i] = '*';  // put silences on the vector
			}
			word_pointer=0; 	// return pointer to zero
			break;
	}
}


short debounce(void)                                             // Short function that does part of the debounce effect
{
	Delay10KTCYx(DELAY_DEBOUNCE);				// wait a delay
	if (SWITCH_KEY == PRESSED) return 1; 			// if button is still pressed, retun 1
	else return 0;						// if button is not pressed, means that the user didn't pressed the button, return 0
}

char identifyClick(int amount_of_time)                           // Here, the time pressing the button for dot, dash and error is determined
{
	if(amount_of_time <= DOT_MAX_COUNT)                          // Dot, if time is less than 3
		return 'a';
	if(amount_of_time >= DASH_MAX_COUNT)                         // Error, if time is greater than 10
		return 'c';
	else if(DOT_MAX_COUNT < amount_of_time < DASH_MAX_COUNT)     // Between 3 and 10, it is a dash
		return 'b';
}


int pushCounter(int id)
{
	int counter = 0;
	switch (id)
	{
		case 1: //Counter RB0
			while(SWITCH_KEY == PRESSED) // while the button is pressed, increment the counter
			{	
				counter++;
				if (counter>DASH_MAX_COUNT)  // if user pressed for a long time, more than a dash, sinalize an error
					flashLed(1); // ERROR
				Delay10KTCYx(DELAY_PRESS_COUNT); // Delay
			}
			break;	
		default:
			counter = 0;
			break;
	}
	return counter; // return the counter wich contains a measure of time pressed
}


void state_machine()
{
	switch (state){
		case 0:                          // Stand-by state: When no code generation in progress (no user interation)
			INTCONbits.INT0IE = 1;       // Enable INT0 interrupts
			INTCONbits.TMR0IE = 1;
			T0CONbits.TMR0ON = 0;        // Keep timer interrupt off
			flashLed(3);                 // Light a led to user know that it is ready for a conversion
			break;
		case 1:                          // In-conversion state: When a conversion is happening (The user have already pressed the first dot or dash, starting a code generatio)
			T0CONbits.TMR0ON = 1;        // Turn on, the time interrupt
			INTCONbits.TMR0IE = 1;	    // Enable Timer
			flashLed(0);                 
			break;		
	}
}
