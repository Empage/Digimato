/*
 * globals.c
 *
 *  Created on: Nov 29, 2012
 *      Author: matthis
 */

#include "globals.h"

/* bestimmt wie hell (alle) LEDs sein sollen (255: max, 0: aus) in 16er Schritten */
byte brightness = 255;
/* bestimmt, ob der Helligkeitswert gemessen werden soll oder von Hand eingestellt */
boolean autoBrightness = true;

/* Alarm gerade zu hören */
boolean alarmOn = false;
/* bestimmt die Dauer eines Tons (atm in Sekunden) */
volatile byte alarmSecs = 0;
/* bestimmt, bei welchem Ton die Routine playAlarm ist */
byte alarmStep = 0;

/* bestimmt, ob ISR von Timer1 die Zeit auch ins data-Array schreiben soll */
volatile boolean setTime = true;

/* wenn true: Temperatur messen und anzeigen (atm 1mal die Minute) */
volatile boolean showTemperature = false;
/* wenn true: Helligkeit messen und umsetzen (atm 1mal die Sekunde) */
volatile boolean getBrightness = false;
/* bestimmt, ob Timer 2 Alarm oder DCF machen soll */
volatile t2_purpose_t t2_purpose = DCF;
/* true, wenn die Zeit gerade gemessen wurde und übernommen werden kann */
volatile boolean got_time = false;

/* Vergleichswert für die Soft-PWM */
volatile byte cmp = 0;

/* Zum Taster entprellen, merken, ob er vorherige "Runde" schon gedrückt war */
volatile byte buttonState[6];
/* Eine Tastersperre, dass nicht zu schnell hintereinander gedrückt wird */
volatile byte buttonsLocked = 0;
/*
 * To interrupt lengthy functions (like running_letters)
 * Currently used for button events
 */
volatile boolean interrupt = false;

/* Um den richtigen Wochentag im Datumsstring anzeigen zu können */
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

/* Bestimmt, welche Reihe gerade gezeichnet werden soll */
byte row = 0;
/* Hilfskonstrukt, um die richtige Reihe anzusteuern */
byte states[] = {
	0b01111110,
	0b01111101,
	0b01111011,
	0b01110111,
	0b01101111,
	0b01011111,
	0b00111111
};

/* Bestimmt, welche LEDs leuchten sollen */
volatile byte data[7][17];

/*Time Variables */
volatile byte hour=10;
volatile byte min=15;
volatile byte sec=1;
volatile byte decisec=0;

/* Date variables */
byte day_of_week = 1;
byte day = 12;
byte month = 5;
byte year = 91;

/* Temperature-String der Form "YYY.X C" */
volatile char temperature[9] = "undef";

/* Definiert, wie die Zahlen in den time-Funktionen (horizontal, vertical) gezeichnet werden sollen */
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
