/*
 * defines.h
 *
 *  Created on: Nov 26, 2012
 *      Author: matthis
 */

#ifndef DEFINES_H_
#define DEFINES_H_

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

#endif /* DEFINES_H_ */
