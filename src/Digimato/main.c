#include <stdlib.h>
#include <stdio.h>

#include "main.h"
#include "thermometer.h"
#include "conrad_dcf.h"
#include "fontMonoSpace.h"

static void initPorts();
static void initTimer0();
static void initTimer1();
static void initSPI();
static void initADC();

/* TODO Hardware
 * - neuen Stecker löten für Datenkabel
 * - Rueckwand verschraubbar machen
 * - evtl. IR-Diode einbauen
 */

/* TODO Software
 * - erstmal DCF wieder zum Laufen bringen
 * - versuchen mit State Machine und Timer2 DCF-Funktionen abbilden, sodass sie asynchron ablaufen
 * - irgendwann hängt sich das komplette Ding auf, kA wieso
 * - Tasterunterstützung
 */

int main(void) {
	initPorts();
	initSPI();
	initTimer0();
	initADC();
	initTimer1();

	/* setze das data-Array auf Null am Anfang */
	clearAll();
	/* Set global interupts enabled*/
	sei();
	running_letters("On!",100);

	byte dcf_data[60];
	while (conrad_get_dcf_data(dcf_data) || conrad_check_parity(dcf_data)) ;
	conrad_calculate_time(dcf_data);
	conrad_calculate_date(dcf_data);
	char datestring[200];
	snprintf(datestring, 199, "%s,der%02d.%02d.%02d", weekdays[day_of_week], day, month, year);

	while (1) {
		if (showTemperature) {
			therm_initiate_temperature_read();
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
	 * PA6: nc
	 * PA7: Licht-Sensor
	 * Alle sind Input, deshalb 0.
	 */
	DDRA = 0;
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
	DDRB = 0b11110010;
	// Pullup von PB2 aktiveren
	PORTB |= 0b00000101;
	/*
	 * TODO Lagesensor
	 */
	//DDRC = 0;
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
//	if(brightness!=0){
		drawWithBrightness();
//	}else{
//		draw();
//	}
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
	//TODO mit dem Oszi verifizieren
	OCR1A = 1440 - 1;
	/* Compare Interrupts erlauben */
	TIMSK |= (1<<OCIE1A);
}

ISR (TIMER1_COMPA_vect) {
	/* folgendes jede volle Sekunde tun */
	if(++splitSecCount == 10){
		splitSecCount = 0;
		getBrightness = true;
		/* Uhrzeit um eins erhoehen */
		tick();

		if (setTime) {
			/*
			 * Timer 0 Interrupts verbieten, damit es kein Flackern gibt
			 * dann Zeit im data-Array aktualisieren und T0 intrs reaktiveren
			 */
			T0_DISABLE_INTR();
			vertical_time();
			T0_ENABLE_INTR();
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

/*Draws all the pixel with the brigtness given in the data Array*/
inline void draw(void){
	byte output=0;
	byte i=0;
	/* The first output Byte */
	for(i=0; i<8; i++){
		output = output<<1;
		if(data[state][i]>cmp){
			output++;
		}
	}
	/* output to SPI-Register */
	SPDR = output;
	output = 0;
	/* Second output Byte */
	for(;i<16;i++){
		output = output<<1;
		if(data[state][i]>cmp){
			output++;
		}
	}
	/* Wait for SPI transmission complete*/
	while(!(SPSR & (1<<SPIF)));
	/* output to SPI-Register */
	SPDR = output;

	output=128;

	/* Last Bit direct on microcontroller pin */
	if(data[state][i]>cmp){
		output=0;
	}
	/* Increment cmp-variable */


	while(!(SPSR & (1<<SPIF)));



	/* RCK auf null ziehen */
	PORTB &= 0b11101111;
	/* Alle Mosfets aus, PD7 unbelegt */
	PORTD = 0;//
	/* RCK auf high */
	PORTB |= 0b00010000;
	/* Mosfet wieder an */
	PORTD = states[state++]+output;
	if(state==7){
		state=0;
		cmp+=16;
	}

}

/*Draws all the pixel with the brigtness of the global brightness variable*/
void drawWithBrightness(void){
	byte output=0;

	if(brightness>cmp){
		byte i=0;
		/* The first output Byte */
		for(i=0;i<8;i++){
			output = output<<1;
			if(data[state][i]>0){
				output++;
			}
		}
		/* output to SPI-Register */
		SPDR = output;
		output = 0;
		/* Second output Byte */
		for(;i<16;i++){
			output = output<<1;
			if(data[state][i]>0){
				output++;
			}
		}
		/* Wait for SPI transmission complete*/
		while(!(SPSR & (1<<SPIF)));
		/* output to SPI-Register */
		SPDR = output;

		output = 128;

		/* Last Bit direct on microcontroller pin */
		if(data[state][i]>0){
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
	/* Alle Mosfets aus, PD7 unbelegt */
	PORTD = 0;//
	/* RCK auf high */
	PORTB |= 0b00010000;
	/* Mosfet wieder an */
	PORTD = states[state++]+output;
	if(state==7){
		state=0;
		/* Increment cmp-variable */
		cmp+=16;
	}
}

/* Displays Laufschrift */
void running_letters_simple(char* str) {
	running_letters(str,200);
}

/* Displays Laufschrift */
void running_letters(char* str, byte time) {
	/* Waehrend der Laufschrift darf die Zeit nicht ins data-Array geschrieben werden */
	setTime = false;
	for (int16_t i = 16; i >= (-6) * (int16_t)strlen(str); i--) {
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
void place_mono_char_checked(int16_t pos,byte zeichen){
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

void vertical_time(void) {
	clearAll();
	vertical_num(0,  0, hour / 10);
	vertical_num(4,  0, hour % 10);
	vertical_num(0,  6,  min / 10);
	vertical_num(4,  6,  min % 10);
	vertical_num(0, 12,  sec / 10);
	vertical_num(4, 12,  sec % 10);
}

void vertical_num(byte posx, byte posy, byte number){
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

void tick(void) {
	if(++sec == 60){
		/* Einmal die Minute die Temperatur anzeigen */
		showTemperature = true;
		sec = 0;
		if(++min == 60){
			min = 0;
			if(++hour == 24)
			hour = 0;
		}
	}
}

/****************************************************************************/
//
//void init_timer2() {
//	/* Use prescaler 1024 */
//	TCCR2 |= (1 << CS22)|(1 << CS21)|(1 << CS20);
//	/* Enable CTC Mode */
//	TCCR2 |= (1 << WGM21)|(0 << WGM20);
//	/* CTC Wert: 147456 / 1024 / 100 = 144; -1 weil Intr erst 1 Timer Clock cycle sp�ter ausgel�st wird */
//	OCR2 = 144 - 1;
//	/* Initialize counter */
//	TCNT2 = 0;
//	/* Compare Interrups erlauben */
//	TIMSK |= (1<<OCIE2);
//
//	data[0][0] = 0;
//}
//// TODO do we need that?
///* Displays the Time */
//void time(void){
//	if(++sec==60){
//		sec=0;
//		if(++min==60){
//			min=0;
//			if(++hour==24)
//			hour=0;
//			placeNumber(0,10);
//		}
//	}
//	byte r=hour/10;
//	if(r)
//		placeNumber(0,r);
//	placeNumber(4,hour%10);
//	placeNumber(10,min/10);
//	placeNumber(14,min%10);
//	if(sec%2){
//		data[2][8]=255;
//		data[4][8]=255;}
//	else{
//		data[2][8]=0;
//		data[4][8]=0;}
//}
//
////TODO do we need that?
//void time_new(void){
//	if(++min==60){
//		min=0;
//		if(++hour==24)
//		hour=0;
//		placeNumber(0,10);
//	}
//
//	byte r=hour/10;
//	if(r)
//		placeNumber(0,r);
//	placeNumber(4,hour%10);
//	placeNumber(10,min/10);
//	placeNumber(14,min%10);
//
//		data[2][8]=255;
//		data[4][8]=255;
//
//}
//
//void placeNumber(byte pos,byte number){
//	for(byte k = 0; k<7;k++){
//		byte bitdata = numbers[number*7+k];  //gets 1 line(k) of the number from memory
//		if(bitdata&0b00001000)					//when dot is set as "1" the Pixel is set high
//			data[k][pos]=255;
//		else
//			data[k][pos]=0;
//		if(bitdata&0b00000100)
//			data[k][pos+1]=255;
//		else
//			data[k][pos+1]=0;
//		if(bitdata&0b00000010)
//			data[k][pos+2]=255;
//		else
//			data[k][pos+2]=0;
//	}
//}
//
////TODO do we need this?
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
