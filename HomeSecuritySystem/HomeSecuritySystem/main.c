/*
 * HomeSecuritySystem.c
 *
 * Created: 2023-03-08 12:36:50 PM
 * Author : nolan
 */ 

#include <avr/io.h>

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
	
}

void system_status(int status) {
	
}