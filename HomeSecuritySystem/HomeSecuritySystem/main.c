/*
 * HomeSecuritySystem.c
 *
 * Created: 2023-03-08 12:36:50 PM
 * Author : nolan
 */ 

#include <avr/io.h>
#include <util/delay.h>

//LCD pin definitions
#define RS PORTC0
#define CE PORTC1
#define DB4 PORTD4
#define DB5 PORTD5
#define DB6 PORTD6
#define DB7 PORTD7

//LCD functions
void LCD_init (void);
void LCD_command (char command);
void LCD_command_4bit (char command);
void LCD_Char (char AsciiChar);
void LCD_Send_A_String(char *StringOfCharacters);
void LCD_clearScreen (void);
void LCD_home(void);
void LCD_display(void);
void LCD_noDisplay(void);
void increment_cursor(void);

//general system functions
void init_hardware(void);
void system_status(int status); //status 0 is standby, status 1 is armed, status 2 is triggered

int main(void)
{
	init_hardware();
	
    while (1) 
    {
		
    }
}

void init_hardware(void) {
	LCD_init();
}

void system_status(int status) {
	if(status == 0) {
		
	}
	else if(status == 1) {
		
	}
	else if(status == 2) {
		
	}
}

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
void LCD_Send_A_String(char *StringOfCharacters)
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
void LCD_clearScreen (void)
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
void increment_cursor(void)
{
	LCD_command(0x14);
}