/**
* @file   DataProcessing.c
* @author Marc Khouri (marc@khouri.ca)
* @date   April 2014
* @brief  Data processing helpers
**/

#include "DataProcessing.h"

/**
*  @name parse_data
*  @brief a function that searches the first 1000 chars of data "input" for a specified string "keyword", then returns up to the next delimiter or "outputlength" characters into "output"
*/
void parse_data(char * inputData, const char * keyword, char * output, uint8_t outputlength){
	uint16_t i=1000; // search the first 1000 char
	uint8_t j=0;
	while(i && *inputData) {
		#ifdef DEBUG_VERBOSE
		Serial.print(*inputData);
		#endif
		if (j && keyword[j]=='\0') { //found a match
			i=0;
			while(*inputData && (i < outputlength) && 
			(*inputData != '\r') && 
			(*inputData != '\n') && 
			(*inputData != '}') && 
			(*inputData != ',') && 
			(*inputData != ';') && 
			(*inputData != ' ') && 
			(*inputData != '"')){
				output[i]=*inputData;
				inputData++;
				i++;
			}
			output[i]='\0';
			return;
		}
		if (*inputData==keyword[j]) {
			j++;
		} else {
			j=0;
		}
		inputData++;
		i--;
	}
}

/**
* @name    my_atoi
* @brief   Converts a string to a 32 bit integer
* 
* @param [in] s[] String to convert.
*/
uint32_t my_atoi(char *s) {
	uint32_t i = 0;
	while (*s) {
		i = i*10 + (*s) - '0';
		#ifdef DEBUG_VERBOSE
		PRINT_TOKEN(*s);
		PRINT_TOKEN(i);
		#endif
		s++;
  }
  return i;
}
