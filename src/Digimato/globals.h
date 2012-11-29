/*
 * globals.h
 *
 *  Created on: Nov 29, 2012
 *      Author: matthis
 */

#ifndef GLOBALS_H_
#define GLOBALS_H_

#include "defines.h"

extern byte brightness;
extern boolean autoBrightness;
extern volatile byte cmp;		//Value to compare with for Softpwm
extern char* weekdays[];
extern byte state; 	// Count the states 0 to 6
extern byte states[];
extern byte* data[];//Stores the actual Data, one Byte per LED

/*Time Variables */
extern volatile byte hour;
extern volatile byte min;
extern volatile byte sec;
extern volatile byte splitSecCount;

/* Date variables */
extern byte day_of_week;
extern byte day;
extern byte month;
extern byte year;

/* Tabele for Clock Numbers used in the time Funktion */
extern byte numbers[];

#endif /* GLOBALS_H_ */
