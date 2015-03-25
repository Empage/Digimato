#include <stdlib.h>
#include <stdio.h>

#include "main.h"
#include "thermometer.h"
#include "conrad_dcf.h"
#include "fontMonoSpace.h"

#define MATTHIS 1

static void initPorts();
static void initTimer0();
static void initTimer1();
static void initTimer2();
static void initSPI();
static void initADC();
static void drawWithBrightness(void);
static void place_mono_char_checked(int16_t pos,uint8_t zeichen);
static void horizontal_time(void);
static void horizontal_num(byte pos, byte number);
static void vertical_time(void);
static void vertical_num(uint8_t posx, uint8_t posy, uint8_t number);
static void tick(void);
static void getButtonStates(void);
static void handleButtons(void);
static void playAlarm(void);

/* TODO Hardware
 * - 12. Spalte neu verlöten
 * - Acrylglas
 * - evtl. IR-Diode einbauen
 */

/* TODO Software
 * - versuchen mit State Machine und Timer2 DCF-Funktionen abbilden, sodass sie asynchron ablaufen
 * - bei Helligkeit nen Mittelwert nehmen oder so um leichtes Schwanken zu vermeiden
 */

int main(void) {
	initPorts();
	initSPI();
	initTimer0();
	initADC();
	initTimer1();
	initTimer2();

	/* setze das data-Array auf Null am Anfang */
	clearAll();
	/* Set global interupts enabled*/
	sei();

//	while (1) {
//		for(byte i = 0; i<7;i++){
//			for (byte j = 0; j<17; j++) {
//				data[i][j]=255;
//			}
//		}
//	}

	running_letters("On!",100);
#if MATTHIS == 1
	running_letters("Matthis stinkt!",200);
#endif
//	char datestring[200];
//	snprintf(datestring, 199, "%s,der%02d.%02d.%02d", weekdays[day_of_week], day, month, year);

	conrad_init_time_measure();

	while (1) {
		if (showTemperature) {
			therm_initiate_temperature_read();
			//TODO evtl. umändern um reaktiver auf Buttons zu sein
			_delay_ms(1000);
			therm_get_temperature((char*)temperature);
			running_letters((char*)temperature, 100);
			showTemperature = false;
		}

		/* Helligkeitsmessung nur einmal die Sekunde, wird von ISR1 gesetzt */
		if(getBrightness){
			/* ADC Helligkeitsmessung starten */
			ADCSRA |= (1<<ADSC);
			/* warte bis die Konvertierung abgeschlossen ist */
			while (ADCSRA & (1<<ADSC))
				;
			brightness = 255 - ADCH;
			getBrightness = false;
		}

		/* Tasterevents verarbeiten */
		handleButtons();

		//TODO alarm
//		if (alarmOn) {
//			TIMSK |= OCIE2;
//			playAlarm();
//		} else {
//			TIMSK &= ~OCIE2;
//		}

		if (got_time) {
			if (conrad_check_parity() == SUCCESS) {
				conrad_calculate_time();
				conrad_calculate_date();
				got_time = false;
			} else {
				conrad_init_time_measure();
			}
		}
	}

	return 0;
}

static void initPorts() {
	/*
	 * PA0: Taster schwarz 1
	 * PA1: Taster schwarz 2
	 * PA2: Taster rot 1
	 * PA3: Taster rot 2
	 * PA4: Taster blau 1
	 * PA5: Taster blau 2
	 * PA6: Temperatursensor
	 * PA7: Lichtsensor
	 * Alle sind Input, deshalb 0.
	 */
	DDRA = 0;
	/* Pullup für die Taster aktivieren */
	PORTA |= 0b00111111;
	/*
	 * PB0: nc
	 * PB1: rote Debug LED
	 * PB2: Conrad DC7 Eingang invers
	 * PB3: nc
	 * PB4: RCK (p2)
	 * PB5: SER_IN (p3)
	 * PB6: IR-Empfaenger
	 * PB7: SRCK (p13)
	 */
	DDRB = 0b11110011;
	/* Pullup von PB2 aktiveren */
	PORTB |= 0b00000100;
	/*
	 * 	PC0: Lautsprecher
	 * TODO Lagesensor
	 */
	DDRC = 0b00000001;
	/*
	 * PD0: Mosfet lila
	 * PD1: Mosfet dunkelblau
	 * PD2: Mosfet gruen
	 * PD3: Mosfet gelb
	 * PD4: Mosfet orange
	 * PD5: Mosfet rot
	 * PD6: Mosfet braun
	 * PD7: Mosfet hellblau
	 */
	DDRD = 0xFF;
}

static void initTimer0() {
	/* Interrupt bei Overflow */
	T0_ENABLE_INTR();
	/* Timer aktivieren */
	T0_ACTIVATE();

}

ISR (TIMER0_OVF_vect){
	drawWithBrightness();
}

/* Der 16-bit Timer zur Generierung eines Interrupts alle 100 ms fuer die main-Routine */
static void initTimer1() {
	/* Prescaler 1024 benutzen */
	TCCR1B |= (1<<CS12) | (1<<CS10);
	/* CTC Modus aktivieren */
	TCCR1B |= (1<<WGM12);
	/*
	 * CTC Wert: 14745600 (Takt) / 1024 (Prescaler) / 10 (CTCs pro Sekunde)
	 * -1 weil Interrupt erst 1 Timer Clock Cycle spaeter ausgefuehrt wird
	 */
	OCR1A = 1440 - 1;
	/* Compare Interrupts erlauben */
	TIMSK |= (1<<OCIE1A);
}

ISR (TIMER1_COMPA_vect) {
	/* folgendes jede volle Sekunde tun */
	if(++decisec == 10){
		decisec = 0;
		if (autoBrightness) {
			getBrightness = true;
		}
		/* Uhrzeit um eins erhoehen */
		tick();

		if (setTime) {
			/*
			 * Timer 0 Interrupts verbieten, damit es kein Flackern gibt
			 * dann Zeit im data-Array aktualisieren und T0 intrs reaktiveren
			 */
			T0_DISABLE_INTR();
			horizontal_time();
			T0_ENABLE_INTR();
		}
		/* Alarmcounter verringern */
		if (alarmSecs) {
			alarmSecs--;
		}
	}
	getButtonStates();
}

void initTimer2() {
	/* Use prescaler 1024 */
	TCCR2 |= (1 << CS22)|(1 << CS21)|(1 << CS20);
	/* Enable CTC Mode */
	TCCR2 |= (1 << WGM21)|(0 << WGM20);
	/* CTC Wert: 147456 / 1024 / 100 = 144; -1 weil Intr erst 1 Timer Clock cycle sp�ter ausgel�st wird */
	OCR2 = 144 - 1;
	/* Initialize counter */
	TCNT2 = 0;
}

ISR (TIMER2_COMP_vect) {
	byte ret;

	if (t2_purpose == ALARM) {
		SPEAKER_TOGGLE();
		DBG_LED_TOGGLE();
	} else {
		PORTB ^= 1;
		ret = conrad_state_get_dcf_data();
		if (ret == T2_WAIT) {
			/* reset counter to wait exactly 10 ms */
			TCNT2 = 0;
		} else if (ret == SUCCESS) {
			got_time = true;
			T2_DISABLE_INTR();
			t2_purpose = ALARM;
		} else {
			/* wenn was schief gegangen ist, resette und fange neu an beim nächsten Interrupt */
			conrad_state_init_dcf();
			search_time = true;
		}
	}
}

/* Initialisierung des Seriellen Peripheren Interfaces */
static void initSPI() {
	/* Enable SPI, Master, set clock rate fck/4, LSB first */
	SPCR = (1<<SPE)|(1<<MSTR); // Clock / 128: |(1<<SPR0)|(1<<SPR1)
	SPSR = (1<<SPI2X);
}

/* Initialisierung des Analog-Digital-Converters */
static void initADC() {
	/*
	 * 00  --> Externe Referenz
	 * 1   --> 8 Hoechstwertige Bits in ADCH + 2 Niederweritge in ADCL
	 * 00111--> ADC7 als input
	 */
	ADMUX=0b00100111;
	/*
	 * 1   -->  ADC enabled
	 * 0   -->  Startbit on (ADSC)
	 * 0   -->  Freerunning off (automatisches neustart)
	 * 0   -->  Interupt flag
	 * 0   -->  Interupt off
	 * 100 -->  Prescale 16 (125kHz bei 20MHz Takt)
	 */
	ADCSRA=0b11000100;
}

#if MATTHIS == 1
/* Draws all the pixel with the brightness of the global brightness variable */
static inline void drawWithBrightness(void){
	byte output=0;

	if(brightness>cmp){
		byte i;
		/* The first output Byte */
		for(i=16;i>8;i--){
			output = output<<1;
			if(data[row][i]>0){
				output++;
			}
		}
		/* output to SPI-Register */
		SPDR = output;
		output = 0;
		/* Second output Byte */
		for(;i>0;i--){
			output = output<<1;
			if(data[row][i]>0){
				output++;
			}
		}
		/* Wait for SPI transmission complete*/
		while(!(SPSR & (1<<SPIF)));
		/* output to SPI-Register */
		SPDR = output;

		output = 128;

		/* Last Bit direct on microcontroller pin */
		if(data[row][i]>0){
			output=0;
		}
		while(!(SPSR & (1<<SPIF)));
	}else{
		/* output to SPI-Register */
		SPDR = 0;
		/* Wait for SPI transmission complete*/
		while(!(SPSR & (1<<SPIF)));
		SPDR = 0;
		while(!(SPSR & (1<<SPIF)));

		output=128; //LED 16 aus!
	}

	/* RCK auf null ziehen */
	PORTB &= 0b11101111;
	/* Gates aller (p-Kanal) Mosfets auf 1, daher hängt Ausgang in der Luft */
	/* PD7 ist LED16, die ist aus bei high */
	PORTD = 0xFF;//
	/* RCK auf high */
	PORTB |= 0b00010000;
	/* Den Mosfet der aktuellen Reihe wieder an (also auf 0 runterziehen) */
	PORTD = states[row++] + output;
	if(row == 7){
		row = 0;
		/* Increment cmp-variable */
		cmp+=16;
	}
}

#else
/* Draws all the pixel with the brightness of the global brightness variable */
static inline void drawWithBrightness(void){
	byte output=0;

	if(brightness>cmp){
		byte i=0;
		/* The first output Byte */
		for(i=0;i<8;i++){
			output = output<<1;
			if(data[row][i]>0){
				output++;
			}
		}
		/* output to SPI-Register */
		SPDR = output;
		output = 0;
		/* Second output Byte */
		for(;i<16;i++){
			output = output<<1;
			if(data[row][i]>0){
				output++;
			}
		}
		/* Wait for SPI transmission complete*/
		while(!(SPSR & (1<<SPIF)));
		/* output to SPI-Register */
		SPDR = output;

		output = 128;

		/* Last Bit direct on microcontroller pin */
		if(data[row][i]>0){
			output=0;
		}
		while(!(SPSR & (1<<SPIF)));
	}else{
		/* output to SPI-Register */
		SPDR = 0;
		/* Wait for SPI transmission complete*/
		while(!(SPSR & (1<<SPIF)));
		SPDR = 0;
		while(!(SPSR & (1<<SPIF)));

		output=128; //LED 16 aus!
	}

	/* RCK auf null ziehen */
	PORTB &= 0b11101111;
	/* Gates aller (p-Kanal) Mosfets auf 1, daher hängt Ausgang in der Luft */
	/* PD7 ist LED16, die ist aus bei high */
	PORTD = 0xFF;//
	/* RCK auf high */
	PORTB |= 0b00010000;
	/* Den Mosfet der aktuellen Reihe wieder an (also auf 0 runterziehen) */
	PORTD = states[row++] + output;
	if(row == 7){
		row = 0;
		/* Increment cmp-variable */
		cmp+=16;
	}
}
#endif


/* Displays Laufschrift */
void running_letters_simple(char* str) {
	running_letters(str,200);
}

/* Displays Laufschrift */
void running_letters(char* str, byte time) {
	/* Waehrend der Laufschrift darf die Zeit nicht ins data-Array geschrieben werden */
	setTime = false;
	for (int16_t i = 16; i >= (-6) * (int16_t)strlen(str); i--) {
		/* check for an interruption */
		if (interrupt) {
			handleButtons();
		}
		/* Während Beschreiben vom data-Array darf es nicht angezeigt werden */
		T0_DISABLE_INTR();
		clearAll();
		for (byte k = 0; k < strlen(str); k++) {
			place_mono_char_checked(i+k*6, str[k]);
		}
		T0_ENABLE_INTR();
		_delay_ms(time);
	}
	setTime = true;
}

/* places a character from ASCII 32-127 with verifying correct pos value */
static void place_mono_char_checked(int16_t pos,byte zeichen){
	for(byte k = 0; k<7;k++){
		byte bitdata = font5x7[(zeichen-32)*7+k]; 	//gets 1 line(k) of the character from memory
		if (pos >= 0 && pos < 17) {
			if(bitdata&0b00000001)					//when dot is set as "1" the Pixel is set as on
				data[k][pos]=255;
			else
				data[k][pos]=0;						//else Pixel is set as off
		}
		if (pos + 1 >= 0 && pos + 1 < 17) {
			if(bitdata&0b00000010)
				data[k][pos+1]=255;
			else
				data[k][pos+1]=0;
		}
		if (pos + 2 >= 0 && pos + 2 < 17) {
			if(bitdata&0b00000100)
				data[k][pos+2]=255;
			else
				data[k][pos+2]=0;
		}
		if (pos + 3 >= 0 && pos + 3 < 17) {
			if(bitdata&0b00001000)
				data[k][pos+3]=255;
			else
				data[k][pos+3]=0;
		}
		if (pos + 4 >= 0 && pos + 4 < 17) {
			if(bitdata&0b00010000)
				data[k][pos+4]=255;
			else
				data[k][pos+4]=0;
		}
	}
}

static void horizontal_time(void) {
	byte tens = hour / 10;
	/* speichere den Wert der beiden Punkte, um zu blinken, weil clearAll alles löscht */
	byte p1 = data[2][8];
	byte p2 = data[4][8];

	clearAll();
	if (tens) {
		horizontal_num(0, tens);
	}
	horizontal_num(4, hour % 10);
	horizontal_num(10, min / 10);
	horizontal_num(14, min % 10);

	/* Wenn nach Minutenanfang gesucht wird, lasse die Punkte blinken */
	if (search_time) {
		data[2][8] = p1 ^ 255;
		data[4][8] = p2 ^ 255;
	} else {
		data[2][8] = 255;
		data[4][8] = 255;
	}
}

static void horizontal_num(byte pos, byte number) {
	for(byte k = 0; k<7;k++){
		byte bitdata = numbers[number*7+k];  //gets 1 line(k) of the number from memory
		if(bitdata&0b00001000)					//when dot is set as "1" the Pixel is set high
			data[k][pos]=255;
		else
			data[k][pos]=0;
		if(bitdata&0b00000100)
			data[k][pos+1]=255;
		else
			data[k][pos+1]=0;
		if(bitdata&0b00000010)
			data[k][pos+2]=255;
		else
			data[k][pos+2]=0;
	}
}

static void vertical_time(void) {
	clearAll();
	vertical_num(0,  0, hour / 10);
	vertical_num(4,  0, hour % 10);
	vertical_num(0,  6,  min / 10);
	vertical_num(4,  6,  min % 10);
	vertical_num(0, 12,  sec / 10);
	vertical_num(4, 12,  sec % 10);
}

static void vertical_num(byte posx, byte posy, byte number){
	byte help=0;
	for(byte k = 0; k<5;k++){
		if(k==2){help++;}
		if(k==4){help++;}
		byte bitdata = numbers[number*7+help];  //gets 1 line(k) of the number from memory
		if(bitdata&0b00001000)					//when dot is set as "1" the Pixel is set high
			data[6-posx][posy+k]=255;
		else
			data[6-posx][posy+k]=0;
		if(bitdata&0b00000100)
			data[6-(posx+1)][posy+k]=255;
		else
			data[6-(posx+1)][posy+k]=0;
		if(bitdata&0b00000010)
			data[6-(posx+2)][posy+k]=255;
		else
			data[6-(posx+2)][posy+k]=0;
		help++;
	}
}

static void tick(void) {
	if (++sec == 60) {
		sec = 0;
		if (++min == 60) {
			min = 0;
			if (++hour == 24) {
				//TODO Datumserhöhung
				hour = 0;
			}
			/* einmal die Stunde die Zeit neu messen */
			conrad_init_time_measure();
		}
		/* Einmal die Minute die Temperatur anzeigen */
		showTemperature = true;
	}
}

//TODO es muss wohl gar nicht zwei Zyklen lang sein, Entprellung ist ja 'fjeden schneller als 100 ms
static inline void getButtonStates(void) {
	if (buttonsLocked) {
		buttonsLocked--;
		return;
	}
	for (byte button = BUT_BLACK_1; button <= BUT_BLUE_2; button++) {
		if (pressed(button)) {
			/* Taster muss zwei Zyklen (atm 100 ms) aktiv sein */
			if (buttonState[button] != BUT_OFF) {
				buttonState[button] = BUT_ON;
				interrupt = true;
			} else {
				buttonState[button] = BUT_PENDING;
			}
		} else {
			/* Wenn er nur ein Zyklus gedrückt wurde, deaktivere ihn wieder */
			if (buttonState[button] == BUT_PENDING) {
				buttonState[button] = BUT_OFF;
			}
		}
	}
}

static void handleButtons(void) {
	interrupt = false;

	if (buttonState[BUT_BLACK_1] == BUT_ON) {
		buttonState[BUT_BLACK_1] = BUT_OFF;
		if (autoBrightness == false) {
			buttonsLocked = 15; /* Dezisekunden */
			autoBrightness = true;
			running_letters("AB ON", 100);
		} else {
			buttonsLocked = 10; /* Dezisekunden */
			autoBrightness = false;
			running_letters("AB OFF", 100);
		}
	}
	if (buttonState[BUT_BLACK_2] == BUT_ON) {
		buttonState[BUT_BLACK_2] = BUT_OFF;
		buttonsLocked = 3; /* Dezisekunden */
		if (autoBrightness == false) {
			brightness += 16;
		}
	}
	if (buttonState[BUT_RED_1] == BUT_ON) {
		buttonState[BUT_RED_1] = BUT_OFF;
		buttonsLocked = 10; /* Dezisekunden */
		sec = 0;
		if(++min == 60){
			min = 0;
		}
	}
	if (buttonState[BUT_RED_2] == BUT_ON) {
		buttonState[BUT_RED_2] = BUT_OFF;
		buttonsLocked = 10; /* Dezisekunden */
		sec = 0;
		if (min-- == 0) {
			min = 59;
		}
	}
	if (buttonState[BUT_BLUE_1] == BUT_ON) {
		buttonState[BUT_BLUE_1] = BUT_OFF;
		buttonsLocked = 10; /* Dezisekunden */
		sec = 0;
		if(++hour == 24){
			hour = 0;
		}
	}
	if (buttonState[BUT_BLUE_2] == BUT_ON) {
		buttonState[BUT_BLUE_2] = BUT_OFF;
		buttonsLocked = 10; /* Dezisekunden */
		sec = 0;
		if (hour-- == 0) {
			hour = 23;
		}
	}
}

inline void playAlarm() {
	if (alarmSecs) {
		return;
	}

	switch (++alarmStep) {
	case 1:
		OCR2 = 9 - 1;
		TCNT2 = 0;
		alarmSecs = 2;
		return;
	case 2:
		OCR2 = 4 - 1;
		TCNT2 = 0;
		alarmSecs = 2;
		return;
	case 3:
		OCR2 = 9 - 1;
		TCNT2 = 0;
		alarmSecs = 2;
		return;
	case 4:
		OCR2 = 50 - 1;
		TCNT2 = 0;
		alarmSecs = 2;
		return;
	case 5:
		OCR2 = 9 - 1;
		TCNT2 = 0;
		alarmSecs = 2;
		return;
	case 6:
		OCR2 = 4 - 1;
		TCNT2 = 0;
		alarmSecs = 2;
		return;
	case 7:
		OCR2 = 18 - 1;
		TCNT2 = 0;
		alarmSecs = 4;
		return;
	case 8:
		OCR2 = 9 - 1;
		TCNT2 = 0;
		alarmSecs = 4;
		alarmStep = 0;
		return;
	default:
		return;
	}
}

///* places a character from ASCII 32-127 */
//void place_mono_char(byte pos,byte zeichen){
//	for(byte k = 0; k<7;k++){
//		byte bitdata = font5x7[(zeichen-32)*7+k]; 	//gets 1 line(k) of the character from memory
//		if(bitdata&0b00000001)							//when dot is set as "1" the Pixel is set as on
//			data[k][pos]=255;
//		else
//			data[k][pos]=0;							//else Pixel is set as off
//		if(bitdata&0b00000010)
//			data[k][pos+1]=255;
//		else
//			data[k][pos+1]=0;
//		if(bitdata&0b00000100)
//			data[k][pos+2]=255;
//		else
//			data[k][pos+2]=0;
//		if(bitdata&0b00001000)
//			data[k][pos+3]=255;
//		else
//			data[k][pos+3]=0;
//		if(bitdata&0b00010000)
//			data[k][pos+4]=255;
//		else
//			data[k][pos+4]=0;
//	}
//}
//
