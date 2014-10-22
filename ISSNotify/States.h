/**
* @file   States.h
* @author Marc Khouri (marc@khouri.ca)
* @date   April 2014
* @brief  state declarations for states that need to be accessed globally
**/

#ifndef __STATES_H_
#define __STATES_H_

//defines states for the ethernet callback function to notify when it is done processing data
typedef enum{
	STATE_CB_WAIT = 0,
	STATE_CB_READY = 1
} state_cb_t;

extern state_cb_t state_cb;
#endif