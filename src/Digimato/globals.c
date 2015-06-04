/*
 * globals.c
 *
 *  Created on: Nov 29, 2012
 *      Author: matthis
 */

#include "globals.h"

/* determines the brightness of all LEDS: 255 is max, 0 is off, stepsize is 16 */
byte brightness = 255;
/* determines whether to measure the brightness value or to hand tune it */
boolean autoBrightness = true;
/* determines whether time or brightness value is shown */
boolean showBrightness = false;

/* is the alarm currently on? */
boolean alarmOn = false;
/* determine the duration of one tone of the alarm */
volatile byte alarmSecs = 0;
/* determines which tone playAlarm() currently plays */
byte alarmStep = 0;

/* determines whether ISR1 should write the time into the data array */
volatile boolean setTime = true;

/* if true: measure temperature and display it */
volatile boolean showTemperature = false;
/* if true: measure brightness and set variable 'brightness' accordingly */
volatile boolean getBrightness = false;
/* purpose of timer2 can be DCF or alarm */
volatile t2_purpose_t t2_purpose = DCF;
/* true if time was measured and parity check was successfull */
volatile boolean got_time = false;
/* true if DCF searches for the start of a new minute */
volatile boolean search_time = false;

/* compare value for the PWM */
volatile byte cmp = 0;

/* To debounce the buttons: remember the state of the button from last decisec */
volatile byte buttonState[6];
/* button locking: determines how many decisecs need to pass before another button event can be triggered */
volatile byte buttonsLocked = 0;
/*
 * To interrupt lengthy functions (like running_letters)
 * Currently used for button events
 */
volatile boolean interrupt = false;

/* to display the weekday */
char* weekdays[] = {
	"",
	"Montag",
	"Dienstag",
	"Mittwoch",
	"Donnerstag",
	"Freitag",
	"Samstag",
	"Sonntag"
};

/* determines currently drawn row */
byte row = 0;
/* helper array to access the correct row */
byte states[] = {
	0b01111110,
	0b01111101,
	0b01111011,
	0b01110111,
	0b01101111,
	0b01011111,
	0b00111111
};

/* pictures the LED matrix */
volatile byte data[7][17];

/* time variables */
volatile byte hour=10;
volatile byte min=15;
volatile byte sec=1;
volatile byte decisec=0;

/* date variables */
byte day_of_week = 1;
byte day = 12;
byte month = 5;
byte year = 91;

/* temperature string of the form "YYY.X C" */
volatile char temperature[9] = "undef";

/* defines the shape of the numbers used to display the time */
byte numbers[] = {
	/* 0: */
	0x0E,0x0A,0x0A,0x0A,0x0A,0x0A,0x0E,
	/* 1: */
	0x02,0x02,0x02,0x02,0x02,0x02,0x02,
	/* 2: */
	0x0E,0x02,0x02,0x0E,0x08,0x08,0x0E,
	/* 3: */
	0x0E,0x02,0x02,0x0E,0x02,0x02,0x0E,
	/* 4: */
	0x0A,0x0A,0x0A,0x0E,0x02,0x02,0x02,
	/* 5: */
	0x0E,0x08,0x08,0x0E,0x02,0x02,0x0E,
	/* 6: */
	0x0E,0x08,0x08,0x0E,0x0A,0x0A,0x0E,
	/* 7: */
	0x0E,0x02,0x02,0x02,0x02,0x02,0x02,
	/* 8: */
	0x0E,0x0A,0x0A,0x0E,0x0A,0x0A,0x0E,
	/* 9: */
	0x0E,0x0A,0x0A,0x0E,0x02,0x02,0x0E,
	/* Space: */
	0,0,0,0,0,0,0
};
