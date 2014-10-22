/**
* @file   Structs.h
* @author Marc Khouri (marc@khouri.ca)
* @date   April 2014
* @brief  struct declarations
**/

#ifndef __STRUCTS_H_
#define __STRUCTS_H_

#include <stdint.h>

typedef struct data_source_t{
	const prog_char website_name[]; //website name, stored in PROGMEM
	uint8_t         website_ip[4]; 
	const char      keyword[]; //keyword to look for when parsing data
	const uint8_t   return_len; //max length of data we are looking for
}

typedef struct 

#endif