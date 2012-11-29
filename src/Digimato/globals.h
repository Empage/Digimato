/*
 * globals.h
 *
 *  Created on: Nov 29, 2012
 *      Author: matthis
 */

#ifndef GLOBALS_H_
#define GLOBALS_H_

/* defines */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define DCF_VALUE   (PINB & 0b00000001)
#define DBG_LED_ON  (PORTB |= 0b00000010)
#define DBG_LED_OFF (PORTB &= 0b11111101)
#define byte uint8_t
#define boolean byte
#define TRUE 1
#define true TRUE
#define FALSE 0
#define false FALSE
#define ERROR 1
#define SUCCESS 0

/* globals */
extern byte brightness;
extern boolean autoBrightness;
extern volatile byte cmp;		//Value to compare with for Softpwm
extern char* weekdays[];
extern byte state; 	// Count the states 0 to 6
extern byte states[];
extern byte data[7][17]; //Stores the actual Data, one Byte per LED

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
