/**
* @file   Printing.h
* @author Marc Khouri (marc@khouri.ca)
* @date   April 2014
* @brief  Printing helper functions
**/

#ifndef __PRINTING_H_
#define __PRINTING_H_

/**
* @name    PRINT_TOKEN
* @brief   Macro to print a variable name and its value
*/
#define         PRINT_TOKEN(token) \
				do {Serial.print(#token); \
					Serial.print(" is set to: "); \
					Serial.println(token); } \
				while(0)
				
/**
* @name    print_p
* @brief   Prints a string stored in PROGMEM.
* 
* @param [in] str[] String stored in PROGMEM.
*/
void print_p(const prog_char str[]);

#endif