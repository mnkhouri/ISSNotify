/**
* @file   Printing.cpp
* @author Marc Khouri (marc@khouri.ca)
* @date   April 2014
* @brief  Printing helper functions
**/

#include <avr/pgmspace.h>
#include <Arduino.h>
#include "Printing.h"

/**
* @name    PRINT_TOKEN
* @brief   Macro to print a variable name and its value
*/
//defined in header file

/**
* @name    print_p
* @brief   Prints a string stored in PROGMEM.
* 
* @param [in] str[] String stored in PROGMEM.
*/
void print_p(const prog_char str[]){
	char c;
	if(!str) return;
	while((c = pgm_read_byte(str++))){
		Serial.write(c);
	}
}