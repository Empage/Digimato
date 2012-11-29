/*
 * conrad_dcf.c
 *
 *  Created on: Nov 26, 2012
 *      Author: matthis
 */


#include "conrad_dcf.h"
#include "main.h"
#include "defines.h"
#include "globals.h"

//TODO auf Timer umstellen: entweder 10ms Timer oder Flankentimer des Signals
byte conrad_get_dcf_data(byte* dcf_data) {
	byte i = 0;
	byte j = 0;
	byte secs;
	byte unmodulated;
	byte modulated;
	// Globale Interrupts verbieten fuer genauere Messung (die natuerlich immernoch ungenau ist ;))
	cli();
	// Minutenstart erkennen
	while (i < 155) {
		// DCF Signal unmoduliert
		if (DCF_VALUE == 1) {
			i++;
			j = 0;
			DBG_LED_OFF;
		// DCF Signal moduliert
		} else {
			j++;
			// Fehlertoleranz, wenn ein Signal kleiner 90 ms erkannt wird
			if (j > 8) {
				i = 0;
				j = 0;
				DBG_LED_ON;
			}
		}
		_delay_ms(10);
	}
	// Minutenanfang erkannt
	data[0][0] = 255;
	// Funkdaten auslesen
	for (secs = 0; secs < 60; secs++) {
		unmodulated = 0;
		modulated = 0;
		// Pausiere bis zum modulierten Signal
		while (DCF_VALUE == 1) ;
		// Gehe 90% der Sekunde durch (Rest ist Toleranz) und zaehle modulierte und unmodulierte Signale
		for (j = 0; j < 90; j++) {
			if (DCF_VALUE == 1) {
				unmodulated++;
			} else {
				if (j < 40) {
					modulated++;
				}
			}
			_delay_ms(10);
		}
		// Wenn mindestens 600 ms unmoduliert waren, deute Signal als g�ltig, sonst ung�ltig und abbrechen
		if (unmodulated > 60 && unmodulated < 130) {
			DBG_LED_OFF;
			// Wenn moduliertes zwischen 60 und 130 ms liegt, liegt logisch 0 an
			if (modulated > 6 && modulated < 13) {
				dcf_data[secs] = 0;
			// Wenn moduliertes zwischen 160 und 230 ms liegt, liegt logisch 1 an
			} else if (modulated > 16 && modulated < 23) {
				dcf_data[secs] = 1;
			}
		} else {
			goto error;
		}
	}

	// Globale Interrupts wieder anschalten
	sei();
	return 0;

error:
	// Globale Interrupts wieder anschalten und mit Fehlerfall returnen
	sei();
	// TODO tmp
	running_letters_simple("EMPFANG FAIL");
	clear_all();
	return 1;
}

byte conrad_check_parity(byte* dcf_data) {
	byte i;
	byte parity;

	// Paritaet Minuten
	parity = 0;
	for (i = 21; i <= 27; i++) {
		parity += dcf_data[i];
	}
	// Wenn die Paritaet ungerade ist (modulo = 1), muss auch das Paritaetsbit 28 "eins" sein
	if (parity % 2 != dcf_data[28]) {
		goto error;
	}


	// Paritaet Stunden
	parity = 0;
	for (i = 29; i <= 34; i++) {
		parity += dcf_data[i];
	}
	// Wenn die Paritaet ungerade ist (modulo = 1), muss auch das Paritaetsbit 35 "eins" sein
	if (parity % 2 != dcf_data[35]) {
		goto error;
	}


	// Paritaet Datum
	parity = 0;
	for (i = 36; i <= 57; i++) {
		parity += dcf_data[i];
	}
	// Wenn die Paritaet ungerade ist (modulo = 1), muss auch das Paritaetsbit 58 "eins" sein
	if (parity % 2 != dcf_data[58]) {
		goto error;
	}

	// Wenn alle Checks okay waren, returne Erfolg
	return SUCCESS;

error:
	// TODO tmp
	running_letters_simple("PARITY FAIL");
	clear_all();
	return ERROR;
}

void conrad_calculate_time(byte* dcf_data) {
	hour =  dcf_data[29] +
			dcf_data[30] * 2 +
			dcf_data[31] * 4 +
			dcf_data[32] * 8 +
			dcf_data[33] * 10 +
			dcf_data[34] * 20;

	min  =  dcf_data[21] +
			dcf_data[22] * 2 +
			dcf_data[23] * 4 +
			dcf_data[24] * 8 +
			dcf_data[25] * 10 +
			dcf_data[26] * 20 +
			dcf_data[27] * 40;
	// TODO geringfuegig falsch ;)
	sec = 3;
}

void conrad_calculate_date(byte* dcf_data) {
	day  =  dcf_data[36] +
			dcf_data[37] * 2 +
			dcf_data[38] * 4 +
			dcf_data[39] * 8 +
			dcf_data[40] * 10 +
			dcf_data[41] * 20;

	day_of_week =
			dcf_data[42] +
			dcf_data[43] * 2 +
			dcf_data[44] * 4;

	month = dcf_data[45] +
			dcf_data[46] * 2 +
			dcf_data[47] * 4 +
			dcf_data[48] * 8 +
			dcf_data[49] * 10;

	year =  dcf_data[50] +
			dcf_data[51] * 2 +
			dcf_data[52] * 4 +
			dcf_data[53] * 8 +
			dcf_data[54] * 10 +
			dcf_data[55] * 20 +
			dcf_data[56] * 40 +
			dcf_data[57] * 80;
}
