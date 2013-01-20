///*
// * temporary.c
// *
// *  Created on: Nov 30, 2012
// *      Author: matthis
// */
//
//#include "globals.h"
//
///* Create Diagonal line, starting with [0][0] */
//void diagonal(void){
//	for(byte k = 0; k<17; k++){
//		data[k%7][k]=255;
//		_delay_ms(1000);
//	}
//	for(byte k = 0; k<17; k++){
//		data[k%7][k]=0;
//		_delay_ms(1000);
//	}
//}
//
///* 1/4 of LEDs glowing randomly */
//void wild_noise(void){
//	for(byte j = 0; j< 7; j++){
//		for(byte k = 0; k<17; k++){
//			if(rand()%4==0)
//				data[j][k]=rand()%256;
//			else
//				data[j][k]=0;
//		}
//	}
//}
//
///* wild_noise with 200ms delay */
//void slow_noise(void){
//		wild_noise();
//		_delay_ms(200);
//}
//
///* Displays a cool thing!*/
//void plasma(void){
////TODO!
//}
//
///*Places the value of one byte on the screen*/
//void printByte(byte b){
//	clearAll();
//	placeNumber(0,b/100);
//	placeNumber(4,(b%100)/10);
//	placeNumber(8,b%10);
//}
//
//void printByteBinary(byte b){
//	clearAll();
//	for(byte j = 0; j< 17&&b>1; j++){
//		for(byte k = 0; k<7&&b>1; k++){
//			data[k][16-j]=b;
//			b=b-2;
//		}
//	}
//}
//
///* old pollin get DCF time function */
////void get_dcf_time(void){
////		byte t_hour=0;
////		byte t_min=0;
////		byte j = 0;
////		while(!(PINB&0b00000100)){};
////		j = 0;
////		//Sucht minutenanfang (Pause groesser 1 sek)
////		for(byte i = 0 ; 1; i++){
////			_delay_ms(10);
////
////			if(PINB&0b00000100){
////				PORTB |=0b00000010;
////				clear_all();
////				j = 0;
////			}else{
////				PORTB &=~(0b00000010);
////				data[(j/17)%7][(j%17)]=255;
////				j ++;
////			}
////
////		}
////		data[3][3]^=0xFF; // Toggle LED 3/3 wenn Minutenanfang gefunden!
////		for(byte i = 0 ; i<38; i++){
////			while(!(PINB&0b00000100)){};
////			//Signall�nge messen (j * 10 ms)
////			for(j = 0;(PINB&0b00000100);j++){
////				_delay_ms(10);
////			}
////			for(byte k=0; k<j;k++){
////				data[i%7][k>16?16:k]=255;
////			}
////			if(j>11){
////				data[3][3]^=0xFF;
////				switch(i)
////				{
////				case 21:
////					t_hour +=1;
////					break;
////				case 22:
////					t_min +=2;
////					break;
////				case 23:
////					t_min +=4;
////					break;
////				case 24:
////					t_min +=8;
////					break;
////				case 25:
////					t_min +=10;
////					break;
////				case 26:
////					t_min +=20;
////					break;
////				case 27:
////					t_min +=40;
////					break;
////				case 29:
////					t_hour +=1;
////					break;
////				case 30:
////					t_hour +=2;
////					break;
////				case 31:
////					t_hour +=4;
////					break;
////				case 32:
////					t_hour +=8;
////					break;
////				case 33:
////					t_hour +=10;
////					break;
////				case 34:
////					t_hour +=20;
////					break;
////				default:
////					break;
////				}
////			}
////
////		}
////	sec = 36;
////	hour= t_hour;
////	min = t_min;
////}
//
///*Fill shift registers*/
////void bbbblal(void){
////	/*Fill shift registers*/
////	byte output=0;
////	/* The first output Byte */
////	for(i=0;i<8;i++){
////		output = output<<1;
////		if(data[state][i]>cmp)
////			output++;
////	}
////	/* output to SPI-Register */
////	SPDR = output;
////	output = 0;
////	/* Second output Byte */
////	for(;i<16;i++){
////		output = output<<1;
////		if(data[state][i]>cmp)
////			output++;
////	}
////	/* Wait for SPI transmission complete*/
////	while(!(SPSR & (1<<SPIF)));
////	/* output to SPI-Register */
////	SPDR = output;
////	output = 0;
////	/* First Bit direct on microcontroller pin */
////	if(data[state][i]>cmp)
////		output++;
////	/* Increment cmp-variable */
////	cmp+=8;
////
////	while(!(SPSR & (1<<SPIF)));
////
////
////
////	/* RCK auf null ziehen */
////	PORTB &= 0b11101111;
////	/* Alle Mosfets aus, PD7 unbelegt */
////	PORTD = 0b10000000;
////	/*
////
////
////	Hier k�nnte ihr Code stehn
////	17 Bit beackern bitte
////
////
////
////	*/
////	/* RCK auf high */
////	PORTB |= 0b00010000;
////	/* Mosfet wieder an */
////	PORTD = states[state++];
////	if(state==7)
////		state=0;
////}
////
//
////void sr_out(uint16_t cData)
////{
/////* Start transmission */
/////* RCK = 0 */
////PORTB &= 0b11101111;
////SPDR = (byte)(cData>>8);
/////* Wait for transmission complete */
////while(!(SPSR & (1<<SPIF)));
////SPDR = (byte)(cData&0b0000000011111111);
////while(!(SPSR & (1<<SPIF)));
/////* RCK = 1 */
////	PORTB |= 0b00010000;
////}
//
//
///* ROOM FOR IMPROVMENT:
//
//- Zahlen f�r Uhr Platzsparend (10 Byte Reichen, 56 in Use)
//- Buchstaben Platz sparend
//- Variable Schriftbreite
//- Glimmen der LEDs unterbinden
//- 1 Bit Farbtiefenmodus f�r Schrift
//- Anordnung ISR optimal?
//	--> states entfernen
//
//-Temperatursensor
//-Audio Spektrometer(google FFT) http://elm-chan.org/works/akilcd/report_e.html
//-PC Zeit synchronisieren
//
//
//
//
//
//
//*/
//
//
///* Alte ISR f�r Timer
//ISR (TIMER0_OVF_vect){
//	if(PORTD==0b10111111) // 10111111111
//		PORTD = 0b11111110;
//	else
//		PORTD = (PORTD<<1)+1;
//
//}*/
//
////    PORTB = PORTB | 0x04; /* besser: PORTB = PORTB | ( 1<<PB2 ) */
//    /* vereinfacht durch Nutzung des |= Operators : */
////    PORTB |= (1<<PB2);
//
//    /* auch mehrere "gleichzeitig": */
////    PORTB |= (1<<PB4) | (1<<PB5); /* Pins PB4 und PB5 "high" */
//
