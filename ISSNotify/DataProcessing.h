/**
* @file   DataProcessing.h
* @author Marc Khouri (marc@khouri.ca)
* @date   April 2014
* @brief  Data processing helpers
**/

#ifndef __DATAPROCESSING_H_
#define __DATAPROCESSING_H_

#include <stdint.h>

/**
* @name    my_atoi
* @brief   Converts a string to a 32 bit integer
* 
* @param [in] s[] String to convert.
*/
uint32_t my_atoi(char *s);

/**
*  @name parse_data
*  @brief a function that searches the first 1000 chars of string "inputData" for a specified string "keyword", then returns up to the next delimiter or "outputlength" characters into "output"
*/
void parse_data(char * inputData, const char * keyword, char * output, uint8_t outputlength);

#endif