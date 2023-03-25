/*
 * HomeSecuritySystem.c
 *
 * Created: 2023-03-08 12:36:50 PM
 * Author : nolan
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

//LCD pin definitions
#define RS PORTC0
#define CE PORTC1
#define DB4 PORTD4
#define DB5 PORTD5
#define DB6 PORTD6
#define DB7 PORTD7

//Status LED pin definitions
#define GREEN PORTC2
#define YELLOW PORTC3
#define RED PORTC4

//Sensor pin definitions
#define HE1 PORTD0
#define HE2 PORTD1
#define PIR PORTB5

//keypad pin definitions
#define C1 PORTD2
#define C2 PORTB0
#define C3 PORTB1
#define R1 PORTB2
#define R2 PORTB3
#define R3 PORTB4
#define R4 PORTC5

//keypad global variables
volatile int row;
volatile int col;
volatile char password[] = "1234";

//LCD functions
void LCD_init (void);
void LCD_command (char command);
void LCD_command_4bit (char command);
void LCD_Char (char AsciiChar);
void LCD_print(char *StringOfCharacters);
void LCD_clear (void);
void LCD_home(void);
void LCD_display(void);
void LCD_noDisplay(void);
void increment_cursor(int n);
void decrement_cursor(int n);
void disable_cursor(void);
void enable_cursor(void);

//sensor function
void poll_sensors(void);

//speaker functions
void speaker_init(void);
void siren_on(void);
void siren_off(void);

//keypad functions
char get_button(void);
char get_new_button(void);
void set_row_low(int row);
int col_pushed(void);
int read_password(void);
int read_bool(void);
void update_password(void);

//general system functions
void init_hardware(void);
void system_status(int status); //status 0 is standby, status 1 is armed, status 2 is triggered


int main(void)
{
	int PIRflag = 0;
	init_hardware();
	
	LCD_print("Welcome!");
	_delay_ms(2000);
	LCD_clear();

	if(PIRflag) {
		//wait for PIR
		LCD_print("System Startup...");
		_delay_ms(60000);
		LCD_clear();
	}
	

		system_status(0);
}

void init_hardware(void) {
	UCSR0B &= ~(1<<TXEN0 | 1<<RXEN0); //disable tx and rx on pin D0 and D1, allows these to be used as normal pins
	
	//display pins
	DDRC |= (1<<RS | 1<<CE | 1<<RED | 1<<GREEN | 1<<YELLOW);
	DDRD |= 0xF0;
	PORTD &= ~0xF0;
	PORTC &= ~(1<<RED | 1<<YELLOW);
	PORTC |= (1<<GREEN);
	
	//speaker pin
	DDRD |= (1<<PORTD3);
	
	//sensor pins
	DDRD &= ~(1<<HE1 | 1<<HE2);
	DDRB &= ~(1<<PIR);
	
	//keypad pins
	//columns as inputs
	DDRD &= ~(1<<C1);
	DDRB &= ~(1<<C2 | 1<<C3);
	
	//rows as outputs
	DDRB |= (1<<R1 | 1<<R2 | 1<<R3);
	DDRC |= (1<<R4);
	
	//set outputs low
	PORTB &= ~(1<<R1 | 1<<R2 | 1<<R3);
	PORTC &= ~(1<<R4);
	
	//enable pullup resistors on inputs
	PORTD |= (1<<C1);
	PORTB |= (1<<C2 | 1<<C3);
	
	LCD_init();
	speaker_init();
}

void system_status(int status) {
	
	LCD_clear();
	
	//system standby (option to arm system or change password)
	if(status == 0) {
		//set status LEDs
		PORTC |= (1<<GREEN);
		PORTC &= ~(1<<RED | 1<<YELLOW);
		
		//begin sequence
		LCD_print("Enter Password:");
		increment_cursor(25);
		
		if(read_password()) {
			LCD_clear();
			
			LCD_print("Arm System?");
			increment_cursor(29); //start new line
			
			if(read_bool()) {
				system_status(1); //system status armed
			}
			
			LCD_clear();
			
			LCD_print("Change Password?");
			increment_cursor(24); //start new line
			
			if(read_bool()) {
				update_password();
				system_status(0);
			}
			
			LCD_clear();
			LCD_print("System Standby");
			_delay_ms(2000);
			
			system_status(0);
		}
		else {
			LCD_clear();
			LCD_print("Wrong Password!");
			_delay_ms(2000);
			LCD_clear();
			system_status(0);
		}
	}
	
	//system armed (polling sensors)
	else if(status == 1) {
		//set status LEDs
		PORTC &= ~(1<<YELLOW);
		PORTC &= ~(1<<GREEN | 1<<RED);
		
		//set LCD
		disable_cursor();
		LCD_print("System Armed");
		
		//poll sensors
		while(1) {
			poll_sensors();
		}
	}
	
	//system triggered (alarm, reading for password)
	else if(status == 2) {
		//set status LEDs
		PORTC |= (1<<RED);
		PORTC &= ~(1<<GREEN | 1<<YELLOW);
		
		//set off alarm
		siren_on();		

		LCD_clear();
		LCD_print("Enter Password:");
		increment_cursor(25);
		
		//read for password
		while(1) {
			if(read_password()) {
				LCD_clear();
				siren_off();
				LCD_print("System Disarmed!");
				_delay_ms(2000);
				system_status(0);
			}
		}
	}
}

//LCD Functions
void LCD_init(void)
{
	_delay_ms(40); //LCD power on delay, needs to be greater than 15ms
	
	//Manual 4 bit initialization of LCD, not likely required, but doesn't harm to do it
	LCD_command_4bit(0x3);
	_delay_ms(5); //min 4.1ms
	LCD_command_4bit(0x3);
	_delay_ms(1);
	LCD_command_4bit(0x3);
	_delay_ms(1); //min of 100us
	
	//DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0    <== See table in Datasheet, * indicated not used
	//0    0   1   0  0*   0*   0*  0*
	
	LCD_command_4bit(0x2); //Function set to 4 bit

	LCD_command(0x28); //2 line, 5*8 dots, 4 bit mode
	LCD_command(0x08); //Display off, cursor off (0x0C for display ON, cursor Off)
	LCD_command(0x01); //Display clear
	LCD_command(0x06); //Entry Set mode, increment cursor, no shift
	LCD_command(0x0C); //No cursor
	
	_delay_ms(2);
}
void LCD_command (char command)
{
	//Basic function used in giving commands to the LCD
	char UpperHalf, LowerHalf;
	
	UpperHalf=command & 0xF0;	//Take upper 4 bits of command
	PORTD &= 0x0F; //Flushes upper half of PortC to 0, but keeps lower half
	PORTD |= UpperHalf;
	PORTC &=~(1<<RS); //Clear RS for command register
	PORTC |= (1<<CE); //Set CE
	_delay_us(1);
	PORTC &= ~(1<<CE); //Clear CE
	_delay_us(200);
	
	LowerHalf=(command<<4); //Lower 4 bits of the command
	PORTD &= 0x0F; //Flushes upper half of PortC to 0, but keeps lower half
	PORTD |= LowerHalf;
	PORTC |= (1<<CE); //Set CE
	_delay_us(1);
	PORTC &= ~(1<<CE); //clear CE
	_delay_ms(2);
	
}
void LCD_command_4bit (char command)
{
	//Basic function used in giving commands to the LCD
	char LowerHalf;
	
	LowerHalf=(command<<4); //Lower 4 bits of the command
	PORTD &= 0x0F; //Flushes upper half of PortC to 0, but keeps lower half
	PORTD |= LowerHalf;
	PORTC &=~(1<<RS); //Clear RS for command register
	PORTC |= (1<<CE); //Set CE
	_delay_us(1);
	PORTC &= ~(1<<CE); //clear CE
	_delay_ms(2);
	
}
void LCD_Char (char AsciiChar)
{
	char UpperHalf, LowerHalf;
	
	UpperHalf=AsciiChar & 0xF0; //Upper 4 bits of data
	PORTD &= 0x0F; //Flushes upper half of PortD to 0, but keeps lower half
	PORTD |= UpperHalf;
	PORTC |=(1<<RS); //Set RS for data register
	PORTC |= (1<<CE); //Set CE
	_delay_us(1);
	PORTC &= ~(1<<CE); //Clear CE
	_delay_us(200);
	
	LowerHalf=(AsciiChar<<4); //Lower 4 bits of the command
	PORTD &= 0x0F; //Flushes upper half of PortD to 0, but keeps lower half
	PORTD |= LowerHalf;
	PORTC |= (1<<CE); //Set CE
	_delay_us(1);
	PORTC &= ~(1<<CE);
	_delay_ms(2);
	
}
void LCD_print(char *StringOfCharacters)
{
	//Take a string input and displays it
	//Each character in the string is processed using LCD_Char which converts the character into the proper 8bit hex #
	//Max character in a string should be <16, after 16th character, everything will be ignored

	int i;
	for (i=0;StringOfCharacters[i]!=0;i++) //Send each character of string until the Null
	{
		LCD_Char(StringOfCharacters[i]);
	}
	
}
void LCD_clear (void)
{
	//Clears the screen
	//DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
	//0    0   0   0  0    0   0  1
	LCD_command(0x01); //Clear display
	_delay_ms(2);
	
	//Returns the cursor to (0,0) position
	//DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
	//0    0   0   0  1    0   0  0
	LCD_command(0x08); //cursor at home position
	LCD_command(0x0C); //cursor off
}
void LCD_home(void)
{
	//Returns the cursor to (0,0) position
	//DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
	//0    0   0   0  0    0   1  0
	LCD_command(0x02);
}
void LCD_display(void)
{
	//Display ON with cursor OFF
	//DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
	//0    0   0   0  1    1   0  0
	LCD_command(0x0C);
}
void LCD_noDisplay(void)
{
	//Display off
	//DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
	//0    0   0   0  1    0   0  0
	LCD_command(0x08);
}
void increment_cursor(int n)
{
	int i;
	
	for(i = 0; i<n; i++) {
		LCD_command(0x14);
	}
}
void decrement_cursor(int n) {
	for(int i = 0; i<n; i++) {
		LCD_command(0x10);
	}
}
void disable_cursor() {
	LCD_command(0x0C);
}
void enable_cursor() {
	LCD_command(0x0F);
}

//Sensor Function
void poll_sensors(void)
{
	//hall effects
	if(!((PIND & (1<<HE1)) == 0) || !((PIND & (1<<HE2)) == 0)) {
		system_status(2);
	}
	
	//PIR
	//if((PINB & (1<<PIR)) == 0) {
	//	system_status(2);
	//}
}

//Speaker Functions
void speaker_init(void)
{
	OCR2A =64; //OCR2A set the top value of the timer/counter2
	OCR2B =32; //OCR2B/OCR2A sets the PWM duty cycle to 50%

	TCCR2A |= (1<<COM2B1);
	TCCR2A &= ~(1<<COM2B0); //Set Clear OC2B on Compare Match, set OC2B at Bottom in non-inverting mode
	TCCR2A |= (1<<WGM21 | 1<<WGM20); //Set Fast PWM mode
	TCCR2B |= (1<<WGM22); //Set OCRA to represent the top count
	
	siren_off();
}
void siren_on(void)
{
	TCCR2B |= (1<<CS22 | 1<<CS20);
	TCCR2B &= ~(1<<CS21); //Set prescaler to 128, starts counter
}
void siren_off(void)
{
	TCCR2B &= ~(1<<CS22 | 1<<CS20);
	TCCR2B &= ~(1<<CS21); //Set prescaler to 0, stops counter;
}

//Keypad Functions
char get_button(void)
{
	int key_pressed=0;
	char b;
	char buttons[4][3]=
	{{'1','2','3'},
	{'4','5','6'},
	{'7','8','9'},
	{'*','0','#'}}; //Same as in Matrix_KeyPad_1.c

	//Check for button push
	//Cycle through the rows with the for loop
	for (row=0; row< 4 ; row++)
	{
		set_row_low (row);
		_delay_ms(20);
		
		col=col_pushed();

		if (col)
		{
			b = buttons[row][col-1]; //b= look-up table in matrix
			//printf("%d-%d: %c\n", row, col,b );
			_delay_ms(500);
			key_pressed=1; //A key was pressed
		}
	}
	if (key_pressed)
	{
		return b;
	}
	else
	{
		return 0;
	}
}
char get_new_button(void)
{
	static char last_button = 0;
	char b;
	
	b=get_button(); //Call get_button function
	if (b == last_button)
	{
		return 0;
	}

	last_button = b;
	return b;
}
void set_row_low(int row)
{
	//Hi-Z the rows (make inputs without pull-ups)
	PORTD |= (1<<C1);
	PORTB |= (1<<C2 | 1<<C3);

	//Drive the specified row low
	switch(row)
	{
		case 0: //set Row 1 low
		PORTB &= ~(1<<R1);
		PORTB |= (1<<R2);
		PORTB |= (1<<R3);
		PORTC |= (1<<R4);
		break;
		case 1: //set Row 2 low
		PORTB &= ~(1<<R2);
		PORTB |= (1<<R1);
		PORTB |= (1<<R3);
		PORTC |= (1<<R4);
		break;
		case 2: //set Row 3 low
		PORTB &= ~(1<<R3);
		PORTB |= (1<<R2);
		PORTB |= (1<<R1);
		PORTC |= (1<<R4);
		break;
		case 3: //set Row 4 low
		PORTC &= ~(1<<R4);
		PORTB |= (1<<R1);
		PORTB |= (1<<R2);
		PORTB |= (1<<R3);
		break;		
	}
}
int col_pushed(void)
{
	
	if ((PIND & (1<<C1) )==0) //check column 1
	{
		return 1;
	}
	else if ((PINB & (1<<C2))==0) //check column 2
	{
		return 2;
	}
	else if ((PINB & (1<<C3))==0) //check column 3
	{
		return 3;
	}
	else
	{
		return 0;
	}
	
}
int read_password(void) {
	char b=0;
	char pin[10]; //string as an array of chars
	int i=0;
	
	enable_cursor();

	while (1)
	{
		b=get_new_button();
	
		//Clear a digit
		if (b== '#')
		{
			pin[i] = '\b';
			i--;
			decrement_cursor(1);
			LCD_Char(' ');
			decrement_cursor(1);
			continue;//breaks one iteration
		}
		//Clear all digits
		if (b== '*')
		{
			pin[i] = '\b';
			for(int j = 0; j<i; j++) {
				decrement_cursor(1);
				LCD_Char(' ');
				decrement_cursor(1);
			}
			i=0; //use the backspace character to backspace the replace the character with a backspace
			continue;//breaks one iteration
		}

		if (b)
		{
			pin[i] = b;
			i++; //Store value pin array to "b"
			LCD_Char(b);

		}
		if (i>=4)
		{
			pin[4]='\0'; //Terminate the string with a null terminator...that makes it a string.
			if (strcmp(pin,password)) {
				return 0;
			}
			else {
				return 1;
			}
		}
	}
}
int read_bool(void) {
	char button;
	
	enable_cursor();

	while(1) {
		button = get_new_button();
		if(button) {
			if(button == '0') {
				return 0;
			}
			else if(button == '1') {
				return 1;
			}
		}
	}
}
void update_password(void) {
	char b=0;
	char pin[10]; //string as an array of chars
	int i=0;
	
	LCD_clear();
	enable_cursor();
	LCD_print("New Password:");
	increment_cursor(27);
	
	while (1)
	{
		b=get_new_button();
		
		//Clear a digit
		if (b== '#')
		{
			pin[i] = '\b';
			i--;
			decrement_cursor(1);
			LCD_Char(' ');
			decrement_cursor(1);
			continue;//breaks one iteration
		}
		//Clear all digits
		if (b== '*')
		{
			pin[i] = '\b';
			for(int j = 0; j<i; j++) {
				decrement_cursor(1);
				LCD_Char(' ');
				decrement_cursor(1);
			}
			i=0;
			continue;//breaks one iteration
		}

		if (b)
		{
			pin[i] = b;
			i++; //Store value pin array to "b"
			LCD_Char(b);

		}
		if (i>=4)
		{
			pin[4]='\0';
			for(int i = 0; i<=4; i++) {
				password[i] = pin[i];
			}
			LCD_clear();
			LCD_print("Password Updated!");
			_delay_ms(2000);
			return;
		}
	}
}