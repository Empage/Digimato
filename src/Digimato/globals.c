/*
 * globals.c
 *
 *  Created on: Nov 29, 2012
 *      Author: matthis
 */

#include "globals.h"

/*If brightness!=0 the value in the dataarray is overitten by this brightness*/
byte brightness = 255;
boolean autoBrightness=false;

/* bestimmt, ob ISR von Timer1 die Zeit auch ins data-Array schreiben soll */
volatile boolean setTime = true;

volatile boolean showTemperature = false;
volatile boolean getBrightness = false;

volatile byte cmp=0;		//Value to compare with for Softpwm

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

byte state=0; 	// Count the states 0 to 6
byte states[] = {
	0b01111110,
	0b01111101,
	0b01111011,
	0b01110111,
	0b01101111,
	0b01011111,
	0b00111111
};

volatile byte data[7][17];//Stores the actual Data, one Byte per LED

/*Time Variables */
volatile byte hour=10;
volatile byte min=15;
volatile byte sec=1;
volatile byte splitSecCount=0;

/* Date variables */
byte day_of_week = 1;
byte day = 12;
byte month = 5;
byte year = 91;

volatile char temperature[9] = "undef";

/* Tabele for Clock Numbers used in the time Funktion */
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
