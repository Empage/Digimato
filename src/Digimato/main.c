 
#include <avr/io.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "fontMonoSpace.h"

#define DCF_VALUE   (PINB & 0b00000001)
// 1: unmoduliert
// 0: moduliert
#define DBG_LED_ON  (PORTB |= 0b00000010)
#define DBG_LED_OFF (PORTB &= 0b11111101)
#define byte uint8_t

//void sr_out(uint16_t cData);
/* Animations */
void clear_all(void);
void set_all(byte value);
void drawWithBrightness(void);
void draw(void);
void wild_noise(void);
void slow_noise(void);
void tick(void);
void time(void);
void time_new(void);
void vertical_time(void);
void get_dcf_time(void);
void diagonal(void);
byte conrad_get_dcf_data(byte* dcf_data);
byte conrad_check_parity(byte* dcf_data);
void conrad_calculate_time(byte* dcf_data);
void conrad_calculate_date(byte* dcf_data);

/* Help Function for time() */
void printByte(byte b);
void printByteBinary(byte b);
void placeNumber(uint8_t pos, uint8_t number);
void vertical_num(uint8_t posx, uint8_t posy, uint8_t number);
/*Character Functions*/
void place_mono_char(uint8_t pos,uint8_t zeichen);
void place_mono_char_checked(int16_t pos,uint8_t zeichen);
void running_letters(char* str, byte time);
void running_letters_simple(char* str);

/*If brightness!=0 the value in the dataarray is overitten by this brightness*/
byte brightness; 

volatile uint8_t cmp=0;		//Value to compare with for Softpwm

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

uint8_t state=0; 	// Count the states 0 to 6
uint8_t states[] ={	0b01111110,
					0b01111101,
					0b01111011,
					0b01110111,
					0b01101111,
					0b01011111,
					0b00111111};
					
uint8_t data[7][17];//Stores the actual Data, one Byte per LED

/*Time Variables */
byte hour=10;
byte min=15;
byte sec=1;

/* Date variables */
byte day_of_week = 1;
byte day = 12;
byte month = 5;
byte year = 91;

/* Tabele for Clock Numbers used in the time Funktion */
uint8_t numbers[]={ 
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
	


int main (void) { 

//PORTB  = Schieberegister
//PB.4 = RCK
//PB.5 = SER_IN
//PB.7 = SRCK

//PORTD = MOSFETs
//PD.0-PD.6 = 7 Mosfets

	DDRB=0b11111010;
	DDRD=0xFF;
	
	PORTD = 0b11111110; //Lila under voltage!
	
	srand(12);
	

	/*CTC Value = 128, fck/8 Timer1 Takt, CTC enabled , Interupt on*/
//	TCCR0 =(1<<CS00); 

	TCCR0 =(1<<CS01)|(1<<CS01); //|(1<<WGM01);
	TIMSK |= (1<<TOIE0);
//	OCR0 = 253;
	/* Set MOSI and SCK output, all others input */
	// DDR_SPI = (1<<DD_MOSI)|(1<<DD_SCK);


	/* Enable SPI, Master, set clock rate fck/4, LSB first */
	SPCR = (1<<SPE)|(1<<MSTR); // Clock / 128: |(1<<SPR0)|(1<<SPR1)
	SPSR = (1<<SPI2X);
	
	/*
	00  --> Externe Referenz
	1   --> 8 Hoechstwertige Bits in ADCH + 2 Niederweritge in ADCL
	00111--> ADC7 als input
	*/
	ADMUX=0b00100111;
	/*
	1   -->  ADC enabled
	0   -->  Startbit off
	0   -->  Freerunning off (automatisches neustart)
	0   -->  Interupt flag
	0   -->  Interupt off
	100 -->  Prescale 16 (125kHz bei20MHz Takt)
	*/
	ADCSRA=0b10000100;
	
	/* Set global interupts enabled*/
	sei();
	

	

	clear_all();
	running_letters("On!",100);
	// Pullup von PB0 aktiveren
	PORTB |=0b00000001;
	
	PORTB |=0b00000010;
	_delay_ms(500);
	PORTB &=~(0b00000010);	
	
	clear_all();
	
	byte dcf_data[60];
	
	/*
	while (conrad_get_dcf_data(dcf_data) || conrad_check_parity(dcf_data)) ;
	conrad_calculate_time(dcf_data);
	conrad_calculate_date(dcf_data);
	*/
	

//	data[0][16] = 255;
//	vertical_num(0,0,hour/10);
//	vertical_num(4,0,hour%10);
	// Alle Bits ausgeben
//	for (byte i = 0; i < 60; i++) {
//		if (dcf_data[i] == 1)
//			data[i/17][i%17] = 255;
//	}
//	data[4][8] = 255;
//	data[5][8] = 255;
//	data[6][8] = 255;
	
	
	
//	get_dcf_time();
	//clear_all();
	
	//uint8_t high = 0;
	//uint8_t low = 0;t
	//
	//uint8_t offset = 0;
	byte i = 0;
	char datestring[200];
	
	while(1){
		
		//Messung starten
		ADCSRA |= (1<<ADSC);
		
		//Auf abgeschlossene Messung 
		while((ADCSRA & (1<<ADSC)));
		
		printByteBinary(ADCH);
		brightness= 255 - ADCH;
		
		_delay_ms(100);
		/*
		byte n=ADCH;
		for(byte i = 0; i<7;i++){
			for(byte j = 0; j<17; j++){
				data[i][j]=n;
			}
		}	
		*/

		
		
		//Matthis Code
		/*
		vertical_time();
		tick();
		//vertical_time();
		//running_letters_simple("Dies ist ein seeeeehr langer String, den unsere kewle Funktion trotzdem anzeigen kann! :-)");
		_delay_ms(900);
		i++;
		if (i == 60) {
			i = 0;
			snprintf(datestring, 199, "%s, der %02d.%02d.%02d", weekdays[day_of_week], day, month, year);
			running_letters_simple(datestring);
			// TODO ungefaehre Zeitkorrektur ;)
			min++;
		}
		
		*/
		
	}
	return 0;

}

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
		// Wenn mindestens 600 ms unmoduliert waren, deute Signal als gültig, sonst ungültig und abbrechen
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
	
	// Parität Minuten
	parity = 0;
	for (i = 21; i <= 27; i++) {
		parity += dcf_data[i];
	}
	// Wenn die Parität ungerade ist (modulo = 1), muss auch das Paritätsbit 28 "eins" sein
	if (parity % 2 != dcf_data[28]) {
		goto error;
	}
	
	
	// Parität Stunden
	parity = 0;
	for (i = 29; i <= 34; i++) {
		parity += dcf_data[i];
	}
	// Wenn die Parität ungerade ist (modulo = 1), muss auch das Paritätsbit 28 "eins" sein
	if (parity % 2 != dcf_data[35]) {
		goto error;
	}
	
	
	// Parität Datum
	parity = 0;
	for (i = 36; i <= 57; i++) {
		parity += dcf_data[i];
	}
	// Wenn die Parität ungerade ist (modulo = 1), muss auch das Paritätsbit 28 "eins" sein
	if (parity % 2 != dcf_data[58]) {
		goto error;
	}
	
	// Wenn alle Checks okay waren, returne Erfolg (= 0)
	return 0;
	
error:
	// TODO tmp
	running_letters_simple("PARITY FAIL");
	clear_all();
	return 1;
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
	// TODO geringfügig falsch ;)
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

void vertical_time(void){
	clear_all();
	vertical_num(0,0,hour/10);
	vertical_num(4,0,hour%10);
	vertical_num(0,6,min/10);
	vertical_num(4,6,min%10);
	vertical_num(0,12,sec/10);
	vertical_num(4,12,sec%10);
}

void vertical_num(uint8_t posx, uint8_t posy, uint8_t number){
	uint8_t help=0;
	for(uint8_t k = 0; k<5;k++){
		if(k==2){help++;}
		if(k==4){help++;}
		uint8_t bitdata = numbers[number*7+help];  //gets 1 line(k) of the number from memory
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


void get_dcf_time(void){
		uint8_t t_hour=0;
		uint8_t t_min=0;
		uint8_t j = 0;
		while(!(PINB&0b00000100)){};
		j = 0;
		//Sucht minutenanfang (Pause groesser 1 sek)
		for(uint8_t i = 0 ; 1; i++){
			_delay_ms(10);
			
			if(PINB&0b00000100){
				PORTB |=0b00000010;
				clear_all();
				j = 0;
			}else{
				PORTB &=~(0b00000010);
				data[(j/17)%7][(j%17)]=255;
				j ++;	
			}
		
		}
		data[3][3]^=0xFF; // Toggle LED 3/3 wenn Minutenanfang gefunden!
		for(uint8_t i = 0 ; i<38; i++){
			while(!(PINB&0b00000100)){};
			//Signallänge messen (j * 10 ms)
			for(j = 0;(PINB&0b00000100);j++){
				_delay_ms(10);
			}
			for(uint8_t k=0; k<j;k++){
				data[i%7][k>16?16:k]=255;
			}
			if(j>11){
				data[3][3]^=0xFF;
				switch(i)
				{
				case 21:
					t_hour +=1;
					break;
				case 22:
					t_min +=2;
					break;
				case 23:
					t_min +=4;
					break;
				case 24:
					t_min +=8;
					break;
				case 25:
					t_min +=10;
					break;
				case 26:
					t_min +=20;
					break;
				case 27:
					t_min +=40;
					break;
				case 29:
					t_hour +=1;
					break;
				case 30:
					t_hour +=2;
					break;
				case 31:
					t_hour +=4;
					break;
				case 32:
					t_hour +=8;
					break;
				case 33:
					t_hour +=10;
					break;
				case 34:
					t_hour +=20;
					break;
				default:
					break;
				}
			}
			
		}
	sec = 36;
	hour= t_hour;
	min = t_min;
}

/* Create Diagonal line, starting with [0][0] */
void diagonal(void){
	for(uint8_t k = 0; k<17; k++){
		data[k%7][k]=255;
		_delay_ms(1000);
	}
	for(uint8_t k = 0; k<17; k++){
		data[k%7][k]=0;
		_delay_ms(1000);
	}
}
/* Clears the whole display */
void clear_all(void){
	for(uint8_t j = 0; j< 7; j++){
		for(uint8_t k = 0; k<17; k++){
			data[j][k]=0;
		}
	}
}
/* Set brightness for the whole display */
void set_all(byte value){
	for(uint8_t j = 0; j< 7; j++){
		for(uint8_t k = 0; k<17; k++){
			data[j][k]=value;
		}
	}
}
/* wild_noise with 200ms delay */
void slow_noise(void){
		wild_noise();
		_delay_ms(200);
}
/* 1/4 of LEDs glowing randomly */
void wild_noise(void){
	for(uint8_t j = 0; j< 7; j++){
		for(uint8_t k = 0; k<17; k++){
			if(rand()%4==0)
				data[j][k]=rand()%256;
			else
				data[j][k]=0;
		}
	}	
}

/* Displays a cool thing!*/
void plasma(void){
//TODO!
	
}

/* Displays Laufschrift */
void running_letters_simple(char* str) {
	running_letters(str,200);
}

/* Displays Laufschrift */
void running_letters(char* str, byte time) {
	for (int16_t i = 16; i >= (-6) * (int16_t)strlen(str); i--) {
		clear_all();
		for (uint8_t k = 0; k < strlen(str); k++) {
			place_mono_char_checked(i+k*6, str[k]);
		}
		_delay_ms(time);
	}	
}
 
/* Displays the Time */
void time(void){
	if(++sec==60){
		sec=0;
		if(++min==60){
			min=0;
			if(++hour==24)
			hour=0;
			placeNumber(0,10);
		}
	}
	uint8_t r=hour/10;
	if(r)				
		placeNumber(0,r);
	placeNumber(4,hour%10);
	placeNumber(10,min/10);
	placeNumber(14,min%10);
	if(sec%2){
		data[2][8]=255;
		data[4][8]=255;}
	else{
		data[2][8]=0;
		data[4][8]=0;}
}

void tick(void){
	if(++sec==60){
		sec=0;
		if(++min==60){
			min=0;
			if(++hour==24)
			hour=0;
		}
	}
}

void time_new(void){
	if(++min==60){
		min=0;
		if(++hour==24)
		hour=0;
		placeNumber(0,10);
	}
	
	uint8_t r=hour/10;
	if(r)				
		placeNumber(0,r);
	placeNumber(4,hour%10);
	placeNumber(10,min/10);
	placeNumber(14,min%10);

		data[2][8]=255;
		data[4][8]=255;

}
/*Places the value of one byte on the screen*/
void printByte(byte b){
	clear_all();
	placeNumber(0,b/100);
	placeNumber(4,(b%100)/10);
	placeNumber(8,b%10);
}

void printByteBinary(byte b){
	clear_all();
	for(byte j = 0; j< 17&&b>1; j++){
		for(byte k = 0; k<7&&b>1; k++){
			data[k][16-j]=b;
			b=b-2;
		}
	}
}

void placeNumber(uint8_t pos,uint8_t number){
	for(uint8_t k = 0; k<7;k++){
		uint8_t bitdata = numbers[number*7+k];  //gets 1 line(k) of the number from memory
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
/* places a character from ASCII 32-127 */
void place_mono_char(uint8_t pos,uint8_t zeichen){
	for(uint8_t k = 0; k<7;k++){
		uint8_t bitdata = font5x7[(zeichen-32)*7+k]; 	//gets 1 line(k) of the character from memory
		if(bitdata&0b00000001)							//when dot is set as "1" the Pixel is set as on 
			data[k][pos]=255;
		else
			data[k][pos]=0;							//else Pixel is set as off
		if(bitdata&0b00000010)
			data[k][pos+1]=255;
		else
			data[k][pos+1]=0;
		if(bitdata&0b00000100)
			data[k][pos+2]=255;
		else
			data[k][pos+2]=0;
		if(bitdata&0b00001000)
			data[k][pos+3]=255;
		else
			data[k][pos+3]=0;	
		if(bitdata&0b00010000)
			data[k][pos+4]=255;
		else
			data[k][pos+4]=0;	
	}
}

/* places a character from ASCII 32-127 with verifying correct pos value */
void place_mono_char_checked(int16_t pos,uint8_t zeichen){
	for(uint8_t k = 0; k<7;k++){
		uint8_t bitdata = font5x7[(zeichen-32)*7+k]; 	//gets 1 line(k) of the character from memory
		if (pos >= 0 && pos < 17) {
			if(bitdata&0b00000001)							//when dot is set as "1" the Pixel is set as on 
				data[k][pos]=255;
			else
				data[k][pos]=0;							//else Pixel is set as off
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

/*Fill shift registers*/


//void bbbblal(void){
//	/*Fill shift registers*/
//	uint8_t output=0;
//	/* The first output Byte */
//	for(i=0;i<8;i++){
//		output = output<<1;
//		if(data[state][i]>cmp)
//			output++;
//	}
//	/* output to SPI-Register */
//	SPDR = output;
//	output = 0;
//	/* Second output Byte */
//	for(;i<16;i++){
//		output = output<<1;
//		if(data[state][i]>cmp)
//			output++;
//	}
//	/* Wait for SPI transmission complete*/
//	while(!(SPSR & (1<<SPIF)));
//	/* output to SPI-Register */
//	SPDR = output;
//	output = 0;
//	/* First Bit direct on microcontroller pin */
//	if(data[state][i]>cmp)
//		output++;
//	/* Increment cmp-variable */
//	cmp+=8;	
//	
//	while(!(SPSR & (1<<SPIF)));	
//	
//	
//	
//	/* RCK auf null ziehen */
//	PORTB &= 0b11101111; 
//	/* Alle Mosfets aus, PD7 unbelegt */
//	PORTD = 0b10000000;
//	/*
//	
//	
//	Hier könnte ihr Code stehn 
//	17 Bit beackern bitte
//	
//	
//	
//	*/ 
//	/* RCK auf high */
//	PORTB |= 0b00010000;
//	/* Mosfet wieder an */
//	PORTD = states[state++];
//	if(state==7)
//		state=0;
//}
//

/*Draws all the pixel with the brigtness of the global brightness variable*/
void drawWithBrightness(void){
	uint8_t output=0;
	
	if(brightness>cmp){
		uint8_t i=0;
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
		
		output=128;

		/* Last Bit direct on microcontroller pin */
		if(data[state][i]>cmp){
			output=0;
		}
		while(!(SPSR & (1<<SPIF)));	
	}else{
		output=0;
		/* output to SPI-Register */
		SPDR = 0;
		/* Wait for SPI transmission complete*/
		while(!(SPSR & (1<<SPIF)));
		SPDR = 0;
		while(!(SPSR & (1<<SPIF)));	
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
/*Draws all the pixel with the brigtness given in the data Array*/
void draw(void){
	uint8_t output=0;
	uint8_t i=0;
	/* The first output Byte */
	for(i=0;i<8;i++){
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

ISR (TIMER0_OVF_vect){
	if(brightness!=0){
		drawWithBrightness();
	}else{
		draw();
	}
}



//void sr_out(uint16_t cData)
//{
///* Start transmission */
///* RCK = 0 */
//PORTB &= 0b11101111; 
//SPDR = (uint8_t)(cData>>8);
///* Wait for transmission complete */
//while(!(SPSR & (1<<SPIF)));
//SPDR = (uint8_t)(cData&0b0000000011111111);
//while(!(SPSR & (1<<SPIF)));
///* RCK = 1 */
//	PORTB |= 0b00010000;
//}


/* ROOM FOR IMPROVMENT:

- Zahlen für Uhr Platzsparend (10 Byte Reichen, 56 in Use)
- Buchstaben Platz sparend
- Variable Schriftbreite
- Glimmen der LEDs unterbinden
- 1 Bit Farbtiefenmodus für Schrift
- Anordnung ISR optimal?
	--> states entfernen

-Temperatursensor
-Audio Spektrometer(google FFT) http://elm-chan.org/works/akilcd/report_e.html
-PC Zeit synchronisieren






*/


/* Alte ISR für Timer 
ISR (TIMER0_OVF_vect){
	if(PORTD==0b10111111) // 10111111111
		PORTD = 0b11111110;
	else
		PORTD = (PORTD<<1)+1;
	
}*/

//    PORTB = PORTB | 0x04; /* besser: PORTB = PORTB | ( 1<<PB2 ) */
    /* vereinfacht durch Nutzung des |= Operators : */
//    PORTB |= (1<<PB2);
 
    /* auch mehrere "gleichzeitig": */
//    PORTB |= (1<<PB4) | (1<<PB5); /* Pins PB4 und PB5 "high" */