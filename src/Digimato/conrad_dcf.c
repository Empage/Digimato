/*
 * conrad_dcf.c
 *
 *  Created on: Nov 26, 2012
 *      Author: matthis
 */

#include "conrad_dcf.h"
#include "main.h"

#ifdef USE_DCF

/* global variables only for the conrad module */
byte i, j, k, secs, unmodulated, modulated;
byte dcf_data[60];
boolean is_start_of_sec;

void conrad_init_time_measure() {
	conrad_state_init_dcf();
	t2_purpose = DCF;
	T2_ENABLE_INTR();
	got_time = false;
}

void conrad_state_init_dcf() {
	i = 0;
	j = 0;
	k = 0;
	secs = 0;
	unmodulated = 0;
	modulated = 0;
	memset(dcf_data, 0, 60);
	is_start_of_sec = true;
}

byte conrad_state_get_dcf_data() {
	/* Wait for start of a minute,
	 * which is when there is no modulation for 1800 or 1900 ms
	 * depending of the parity bit 58 */
	if (i < 175) {
		if (DCF_100) {
			i++;
		} else {
			j++;

			/* if there is more than one false positive, start again */
			if (j >= 2) {
				i = 0;
				j = 0;
			}
		}
		/* now wait for 10 ms realized with timer2 */
		return T2_WAIT;
	}
	/* If we reached this, we recognized the start of a minute.
	 * Now read the bit of every second:
	 * MODULATED 100 ms: bit is 0
	 * MODULATED 200 ms: bit is 1 */
	while (secs < 60) {
		if (is_start_of_sec) {
			/* Wait till we have a low amplitude */
			if (DCF_100) {
				return T2_WAIT;
			}
			is_start_of_sec = false;
		}

		/* sample the DCF value every 10 ms for this second */
		if (k < 99) {
			if (DCF_100) {
				unmodulated++;
			} else {
				/* only in the first 200ms, the signal can be modulated (i.e DCF_25) */
				if (k < 20) {
					modulated++;
				}
			}
			k++;

			/* now wait for 10 ms realized with timer2 */
			return T2_WAIT;
		}
		/* Check whether the bit is 0 or 1 for this second. Use some tolerance.
		 * Values for unmodulated: 80 - 90 => 800 or 900 ms of the second */
		if (unmodulated > 77 && unmodulated < 93) {
			/* the bit is 0 if the signal is 25 % for 100 ms, i.e modulated = 10 */
			if (modulated > 7 && modulated < 13) {
				dcf_data[secs] = 0;
			/* the bit is 1 if the signal is 25 % for 200 ms, i.e. modulated = 20 */
			} else if (modulated > 17 && modulated < 23) {
				dcf_data[secs] = 1;
			/* otherwise it is invalid */
			} else {
				return ERROR;
			}
		/* otherwise it is invalid */
		} else {
			return ERROR;
		}
		/* prepare for the next second */
		secs++;
		is_start_of_sec = true;
		k = 0;
		modulated = 0;
		unmodulated = 0;
	}
	return SUCCESS;
}

//byte conrad_get_dcf_data(byte* dcf_data) {
//	byte i = 0;
//	byte j = 0;
//	byte secs;
//	byte unmodulated;
//	byte modulated;
//	// Globale Interrupts verbieten fuer genauere Messung (die natuerlich immernoch ungenau ist ;))
//	cli();
//	// Minutenstart erkennen
//	while (i < 155) {
//		// DCF Signal unmoduliert
//		if (DCF_VALUE != 0) {
//			i++;
//			j = 0;
////			DBG_LED_OFF();
//		// DCF Signal moduliert
//		} else {
//			j++;
//			// Fehlertoleranz, wenn ein Signal kleiner 90 ms erkannt wird
//			if (j > 8) {
//				i = 0;
//				j = 0;
////				DBG_LED_ON();
//			}
//		}
//		_delay_ms(10);
//	}
//	// Minutenanfang erkannt
//	// Funkdaten auslesen
//	for (secs = 0; secs < 60; secs++) {
//		unmodulated = 0;
//		modulated = 0;
//		// Pausiere bis zum modulierten Signal
//		while (DCF_VALUE != 0) ;
//		// Gehe 90% der Sekunde durch (Rest ist Toleranz) und zaehle modulierte und unmodulierte Signale
//		for (j = 0; j < 90; j++) {
//			if (DCF_VALUE != 0) {
//				unmodulated++;
//			} else {
//				if (j < 40) {
//					modulated++;
//				}
//			}
//			_delay_ms(10);
//		}
//		// Wenn mindestens 600 ms unmoduliert waren, deute Signal als g�ltig, sonst ung�ltig und abbrechen
//		if (unmodulated > 60 && unmodulated < 130) {
////			DBG_LED_OFF();
//			// Wenn moduliertes zwischen 60 und 130 ms liegt, liegt logisch 0 an
//			if (modulated > 6 && modulated < 13) {
//				dcf_data[secs] = 0;
//			// Wenn moduliertes zwischen 160 und 230 ms liegt, liegt logisch 1 an
//			} else if (modulated > 16 && modulated < 23) {
//				dcf_data[secs] = 1;
//			}
//		} else {
//			goto error;
//		}
//	}
//
//	// Globale Interrupts wieder anschalten
//	sei();
//	return 0;
//
//error:
//	// Globale Interrupts wieder anschalten und mit Fehlerfall returnen
//	sei();
//	clearAll();
//	return 1;
//}

byte conrad_check_parity() {
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
	clearAll();
	return ERROR;
}

void conrad_calculate_time() {
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
	sec = 0;
}

void conrad_calculate_date() {
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

#endif
