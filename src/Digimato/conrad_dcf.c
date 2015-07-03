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
	search_time = true;
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
	/* true, wenn eine neue Sekunde beginnt, damit auf moduliertes Signal nur dann gewartet wird */
	is_start_of_sec = true;
}

byte conrad_state_get_dcf_data() {
	/*
	 * Minutenstart erkennen
	 * Wenn es 2 Sekunden keine Modulation gibt, beginnt Minute
	 * eigentlich i >= 200; 155 aus Toleranz.
	 */
	if (i < 155) {
		/* DCF Signal unmoduliert (da es invertiert ist, ist es standartmaessig 1) */
		if (DCF_VALUE != 0) {
			i++;
			j = 0;
//			DBG_LED_OFF();
		/* Wenn es moduliert ist (logisch 0) */
		} else {
			j++;
			/*
			 * Wenn mehr als 70 ms moduliert ist, erkenne es als moduliert an (eig 100 oder 200 ms)
			 * damit beginnt die Minute hier noch nicht, also von vorne messen
			 */
			if (j > 7) {
				i = 0;
				j = 0;
//				DBG_LED_ON();
			}
		}
		/*
		 * returne fehlerfrei.
		 * Durch T2 wird diese Routine nach 10 ms wieder aufgerufen.
		 * Entspricht also quasi _delay_ms(10)
		 */
		return T2_WAIT;
	}
	/*
	 * Wenn wir hier sind, wurde Minutenanfang erkannt.
	 * Jetzt die Daten in jeder Sekunde auslesen.
	 * entweder 100ms (logisch 0) oder 200ms (logisch 1) moduliert.
	 */
	search_time = false;
	while (secs < 60) {
		if (is_start_of_sec) {
			/* Pausiere bis zum modulierten Signal */
			if (DCF_VALUE != 0) {
				return T2_WAIT;
			}
			is_start_of_sec = false;
		}
		/*
		 * Gehe 95 % der Sekunde durch
		 * (Rest ist Zeittoleranz, damit nächstes modulierte Signal nicht verpasst wird)
		 * Zähle dabei modulierte und unmodulierte Messungen
		 */
		if (k < 95) {
			if (DCF_VALUE != 0) {
				unmodulated++;
			} else {
				/*
				 * moduliertes Signal tritt nur am Anfang der Sekunde auf in den ersten 200 ms.
				 * 400 für Toleranz (300 war nicht genug, kA warum)
				 */
				if (k < 40) {
					modulated++;
				}
			}
			k++;
			/*
			 * returne fehlerfrei.
			 * Durch T2 wird diese Routine nach 10 ms wieder aufgerufen.
			 * Entspricht also quasi _delay_ms(10)
			 */
			return T2_WAIT;
		}
		/*
		 * Werte vergangene Sekunde aus:
		 * mindestens 500 ms und kleiner 1,4 s unmoduliert: Signal gültig, sonst ungültig und abbrechen
		 */
		if (unmodulated > 50 && unmodulated < 140) {
			/* Wenn moduliert zwischen 50 und 140 ms, liegt logisch 0 an */
			if (modulated > 5 && modulated < 14) {
				dcf_data[secs] = 0;
			/* Zwischen 150 ms und 240 ms, liegt logisch 1 an */
			} else if (modulated > 15 && modulated < 24) {
				dcf_data[secs] = 1;
			/* sonst ist es ungültig */
			} else {
				return ERROR;
			}
		} else {
			return ERROR;
		}
		/* Bereite die nächste Sekunde vor */
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
