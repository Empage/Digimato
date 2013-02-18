/*
 * globals.h
 *
 *  Created on: Nov 29, 2012
 *      Author: matthis
 */

#ifndef GLOBALS_H_
#define GLOBALS_H_

#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

/* general defines */
#define byte uint8_t
#define boolean byte
#define TRUE 1
#define true TRUE
#define FALSE 0
#define false FALSE
#define ERROR 1
#define SUCCESS 0

/* Nullt das gesamte data-Array */
#define clearAll() memset(data, 0, 7*17)

/* Enthält den zurzeit gültigen DCF-Wert */
#define DCF_VALUE (PINB & 0b00000100)

/* rote Debug LED */
#define DBG_LED_ON()  (PORTB |= 0b00000010)
#define DBG_LED_OFF() (PORTB &= 0b11111101)
#define DBG_LED_TOGGLE() (PORTB ^= 0b00000010)

/* Defines für die 6 Taster */
enum {
	BUT_BLACK_1 = 0,
	BUT_BLACK_2,
	BUT_RED_1,
	BUT_RED_2,
	BUT_BLUE_1,
	BUT_BLUE_2
};
#define BUT_OFF 0
#define BUT_PENDING 1
#define BUT_ON 2
#define pressed(x) ((PINA & (1 << x)) == 0)

/* Timer0 (Zeichenroutine) defines */
#define T0_PRESCALER (1<<CS01) /* Prescaler 8 */
#define T0_ACTIVATE() TCCR0 |= T0_PRESCALER
#define T0_DEACTIVATE() TCCR0 &= ~T0_PRESCALER
#define T0_ENABLE_INTR() TIMSK |= (1<<TOIE0)
#define T0_DISABLE_INTR() TIMSK &= ~(1<<TOIE0)

/* Timer2 (Lautsprecher & DCF) defines */
#define T2_ENABLE_INTR() TIMSK |= (1<<OCIE2)
#define T2_DISABLE_INTR() TIMSK &= ~(1<<OCIE2)
#define SPEAKER_TOGGLE() (PORTC ^= (1 << PC0))
#define T2_WAIT 2
typedef enum {
	DCF, ALARM
} t2_purpose_t;

/*
 * global variables
 * for explanation please look in globals.c
 */
extern byte brightness;
extern boolean autoBrightness;
extern boolean alarmOn;
extern volatile byte alarmSecs;
extern byte alarmStep;
extern volatile boolean setTime;
extern volatile boolean showTemperature;
extern volatile boolean getBrightness;
extern volatile t2_purpose_t t2_purpose;
extern volatile boolean got_time;
extern volatile byte cmp;
extern volatile byte buttonState[6];
extern volatile byte buttonsLocked;
extern volatile boolean interrupt;
extern char* weekdays[];
extern byte row;
extern byte states[];
extern volatile byte data[7][17];
extern volatile byte hour;
extern volatile byte min;
extern volatile byte sec;
extern volatile byte decisec;
extern byte day_of_week;
extern byte day;
extern byte month;
extern byte year;
extern volatile char temperature[9];
extern byte numbers[];

#endif /* GLOBALS_H_ */
