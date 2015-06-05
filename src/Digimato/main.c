#include <stdlib.h>
#include <stdio.h>

#include "main.h"
#include "thermometer.h"
#ifdef USE_DCF
	#include "conrad_dcf.h"
#endif
#include "fontMonoSpace.h"

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
//static void vertical_time(void);
//static void vertical_num(uint8_t posx, uint8_t posy, uint8_t number);
static void tick(void);
static void getButtonStates(void);
static void handleButtons(void);
#ifndef MATTHIS
/* Tobis clock has a speaker built-in */
static void playAlarm(void);
#endif
static void displayTemperature(void);

int main(void) {
	initPorts();
	initSPI();
	initTimer0();
	initADC();
	initTimer1();
	initTimer2();

	/* zero the data array (representing the display) */
	clearAll();
	/* Set global interupts enabled*/
	sei();

	DBG_LED_ON();
	running_letters("On!",100);

//	char datestring[200];
//	snprintf(datestring, 199, "%s,der%02d.%02d.%02d", weekdays[day_of_week], day, month, year);

#ifdef USE_DCF
	conrad_init_time_measure();
#endif

	while (1) {
		if (showTemperature) {
			therm_initiate_temperature_read();
			_delay_ms(750);
			therm_get_temperature((char*)temperature);
			//running_letters((char*)temperature, 100);
			displayTemperature();
			showTemperature = false;
		}

		/* Brightness measurement only once a sec; flag is set in ISR1 */
		if(getBrightness){
			/* start ADC brightness measurement */
			ADCSRA |= (1<<ADSC);
			/* wait for the conversion to finish */
			while (ADCSRA & (1<<ADSC))
				;
			byte brightnessMeasurement = ADCH;
			/* with daylight, the value is ~ 100 => full brightness if lower */
			if (brightnessMeasurement <= 100) {
				brightness = 255;
			} else {
				brightness = 255 - brightnessMeasurement + 100;
			}
			getBrightness = false;

			if (showBrightness) {
				T0_DISABLE_INTR();
				clearAll();
				place_mono_char_checked(0, '0' + brightnessMeasurement / 100);
				place_mono_char_checked(6, '0' + (brightnessMeasurement % 100) / 10);
				place_mono_char_checked(12, '0' + (brightnessMeasurement % 10));
				T0_ENABLE_INTR();
			}
		}

		/* test for pressed buttons */
		handleButtons();

#ifdef USE_DCF
		/* if time got received, check it and take it*/
		if (got_time) {
			if (conrad_check_parity() == SUCCESS) {
				conrad_calculate_time();
				conrad_calculate_date();
				got_time = false;
			} else {
				conrad_init_time_measure();
			}
		}
#endif
	}

	return 0;
}

static void initPorts() {
	/*
	 * PA0: Button black 1
	 * PA1: Button black 2
	 * PA2: Button red 1
	 * PA3: Button red 2
	 * PA4: Button blue 1
	 * PA5: Button blue 2
	 * PA6: temperature sensor
	 * PA7: LDR
	 * all are input => 0
	 */
	DDRA = 0;
	/* activate internal pullup for buttons */
	PORTA |= 0b00111111;
	/*
	 * PB0: nc
	 * PB1: debug LED
	 * PB2: DCF77 input inverse [TS]
	 * PB3: nc
	 * PB4: RCK (p2)
	 * PB5: SER_IN (p3)
	 * PB6: IR-Receiver
	 * PB7: SRCK (p13)
	 */
	DDRB = 0b11110011;
	/* activate pullup of PB2 */
	PORTB |= 0b00000100;
	/*
	 * 	PC0: speaker
	 * 	PC1: nc
	 * 	PC2: nc
	 * 	PC3: nc
	 * 	PC4: nc
	 * 	PC5: nc
	 * 	PC6: nc
	 * 	PC7: DCF77 input inverse [MH]
	 */
	DDRC = 0b00000001;
	/* [MH] no pull-up for DCF, because there is a Schmitt-NOT connected to the pin */
	PORTC &= 0b01111111;
	/*
	 * PD0: Mosfet purple
	 * PD1: Mosfet dark blue
	 * PD2: Mosfet green
	 * PD3: Mosfet yellow
	 * PD4: Mosfet orange
	 * PD5: Mosfet red
	 * PD6: Mosfet brown
	 * PD7: Mosfet light blue
	 */
	/* all output */
	DDRD = 0xFF;
}

/* initialization for timer 0: responsible for the LED PWM */
static void initTimer0() {
	/* interrupt at overflow */
	T0_ENABLE_INTR();
	/* activate timer */
	T0_ACTIVATE();

}

/* ISR for timer 0: responsible for the LED PWM */
ISR (TIMER0_OVF_vect){
	drawWithBrightness();
}

/* Initialization of the 16-bit timer to generate an interrupt every 100 ms; used for time dependent tasks */
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

/* ISR for the 16-bit timer to generate an interrupt every 100 ms; used for time dependent tasks */
ISR (TIMER1_COMPA_vect) {
	/* do this every second */
	if(++decisec == 10){
		decisec = 0;
		/* if mode is on auto brightness, read out the brightness value in the main loop next time */
		if (autoBrightness) {
			getBrightness = true;
		}
		/* increment time by one second */
		tick();

		if (setTime) {
			/*
			 * deactivate timer 0 interrupt to prevent flickering
			 * then refresh time in the data array and reactivate t0 ints
			 */
			T0_DISABLE_INTR();
			horizontal_time();
			T0_ENABLE_INTR();
		}
		/* decrement alarmcounter */
		if (alarmSecs) {
			alarmSecs--;
		}
	}
	getButtonStates();
}

/* initialisation for timer2; used for alarm or dcf measurement */
void initTimer2() {
	/* Use prescaler 1024 */
	TCCR2 |= (1 << CS22)|(1 << CS21)|(1 << CS20);
	/* Enable CTC Mode */
	TCCR2 |= (1 << WGM21)|(0 << WGM20);
	/* CTC value: 147456 / 1024 / 100 = 144; -1 because interupt is cycle later */
	OCR2 = 144 - 1;
	/* Initialize counter */
	TCNT2 = 0;
}

#ifdef USE_DCF
/* ISR for timer2; used for alarm or dcf measurement */
ISR (TIMER2_COMP_vect) {
	byte ret;

	if (t2_purpose == ALARM) {
		SPEAKER_TOGGLE();
		DBG_LED_TOGGLE();
	} else {
		/* purpose == DCF */
		ret = conrad_state_get_dcf_data();
		if (ret == T2_WAIT) {
			/* reset counter to wait exactly 10 ms */
			TCNT2 = 0;
		} else if (ret == SUCCESS) {
			got_time = true;
			T2_DISABLE_INTR();
			t2_purpose = ALARM;
		} else {
			/* if sth did go wrong, reset and start again */
			conrad_state_init_dcf();
			search_time = true;
		}
	}
}
#endif

/* Initialize serial peripheral interface for sending led data to the shift registers*/
static void initSPI() {
	/* Enable SPI, Master, set clock rate fck/4, LSB first */
	SPCR = (1<<SPE)|(1<<MSTR); // Clock / 128: |(1<<SPR0)|(1<<SPR1)
	SPSR = (1<<SPI2X);
}

/* Initialize Analog-Digital-Converter */
static void initADC() {
	/*
	 * 00  --> external reference value
	 * 1   --> 8 most significant bits in ADCH + 2 least significant bit in ADCL
	 * 00111--> ADC7 as input
	 */
	ADMUX=0b00100111;
	/*
	 * 1   -->  ADC enabled
	 * 1   -->  Startbit on (ADSC)
	 * 0   -->  Freerunning off (automatic restart)
	 * 0   -->  Interupt flag
	 * 0   -->  Interupt off
	 * 111 -->  Prescale 128 (115.2 kHz at 14.7456 MHz clock)
	 */
	ADCSRA=0b11000111;
}

#ifdef MATTHIS
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

		output=128; //LED 16 off!
	}

	/* RCK on 0 */
	PORTB &= 0b11101111;
	/* set gates of all p-channel mosfets to 1 => drain is disconnected */
	/* PD7 is LED16, which is off at 1*/
	PORTD = 0xFF;//
	/* RCK on 1 */
	PORTB |= 0b00010000;
	/* reactivate mosfet of the current LED row (set it to 0) */
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

		output=128; //LED 16 off!
	}

	/* RCK on 0 */
	PORTB &= 0b11101111;
	/* set gates of all p-channel mosfets to 1 => drain is disconnected */
	/* PD7 is LED16, which is off at 1*/
	PORTD = 0xFF;//
	/* RCK on 1 */
	PORTB |= 0b00010000;
	/* reactivate mosfet of the current LED row (set it to 0) */
	PORTD = states[row++] + output;
	if(row == 7){
		row = 0;
		/* Increment cmp-variable */
		cmp+=16;
	}
}
#endif


void running_letters_simple(char* str) {
	running_letters(str,200);
}

/* Displays running letters */
void running_letters(char* str, byte time) {
	/* do not write time during running letters */
	setTime = false;
	for (int16_t i = 16; i >= (-6) * (int16_t)strlen(str); i--) {
		/* check for an interruption (not a real interrupt) */
		if (interrupt) {
			handleButtons();
		}
		/* do not draw whilst writing into the data array */
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

#ifdef USE_DCF
	/* save value of both dots for blinking (cause clearAll would erase them) */
	byte p1 = data[2][8];
	byte p2 = data[4][8];
#endif

	clearAll();
	if (tens) {
		horizontal_num(0, tens);
	}
	horizontal_num(4, hour % 10);
	horizontal_num(10, min / 10);
	horizontal_num(14, min % 10);

#ifdef USE_DCF
	/* whilst searching for the start of a minute in dcf, let the dots blink */
	if (search_time) {
		data[2][8] = p1 ^ 255;
		data[4][8] = p2 ^ 255;
	} else {
		data[2][8] = 255;
		data[4][8] = 255;
	}
#else
	/* always display dots */
	data[2][8] = 255;
	data[4][8] = 255;
#endif
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

//static void vertical_time(void) {
//	clearAll();
//	vertical_num(0,  0, hour / 10);
//	vertical_num(4,  0, hour % 10);
//	vertical_num(0,  6,  min / 10);
//	vertical_num(4,  6,  min % 10);
//	vertical_num(0, 12,  sec / 10);
//	vertical_num(4, 12,  sec % 10);
//}
//
//static void vertical_num(byte posx, byte posy, byte number){
//	byte help=0;
//	for(byte k = 0; k<5;k++){
//		if(k==2){help++;}
//		if(k==4){help++;}
//		byte bitdata = numbers[number*7+help];  //gets 1 line(k) of the number from memory
//		if(bitdata&0b00001000)					//when dot is set as "1" the Pixel is set high
//			data[6-posx][posy+k]=255;
//		else
//			data[6-posx][posy+k]=0;
//		if(bitdata&0b00000100)
//			data[6-(posx+1)][posy+k]=255;
//		else
//			data[6-(posx+1)][posy+k]=0;
//		if(bitdata&0b00000010)
//			data[6-(posx+2)][posy+k]=255;
//		else
//			data[6-(posx+2)][posy+k]=0;
//		help++;
//	}
//}

static void tick(void) {
	if (++sec >= 60) {
		sec = 0;
		if (++min >= 60) {
			min = 0;
			if (++hour >= 24) {
				hour = 0;
			}
#ifdef USE_DCF
			/* measure time once every hour */
			conrad_init_time_measure();
#endif
		}
		/* measure and display temperature once every minute */
		//showTemperature = true;
	}
}

static inline void getButtonStates(void) {
	if (buttonsLocked) {
		buttonsLocked--;
		return;
	}
	for (byte button = BUT_BLACK_1; button <= BUT_BLUE_2; button++) {
		/* new way: buttons only need to be pressed one cycle,
		 * 'buttonsLocked' prevents multiple triggering due to bouncing */
		if (pressed(button)) {
			buttonState[button] = BUT_ON;
		}
//		if (pressed(button)) {
//			/* Button has to be pressed two clyces for debouncing */
//			if (buttonState[button] != BUT_OFF) {
//				buttonState[button] = BUT_ON;
//				interrupt = true;
//			} else {
//				buttonState[button] = BUT_PENDING;
//			}
//		} else {
//			/* if only pressed one clycle, deactivate it */
//			if (buttonState[button] == BUT_PENDING) {
//				buttonState[button] = BUT_OFF;
//			}
//		}
	}
}

static void handleButtons(void) {
	interrupt = false;

	if (buttonState[BUT_BLACK_1] == BUT_ON) {
		buttonState[BUT_BLACK_1] = BUT_OFF;
		buttonsLocked = 5; /* deciseconds */
		if (autoBrightness == false) {
			brightness += 16;
		} else {
			/* measure and display temperature */
			/* button assignment not ideal, but works for now */
			showTemperature = true;
		}
	}
	if (buttonState[BUT_BLACK_2] == BUT_ON) {
		buttonState[BUT_BLACK_2] = BUT_OFF;
		/* currently use this button to switch from time view to brightness view
		 * in order to find a good autobrightness function */
		if (showBrightness == false) {
			buttonsLocked = 5; /* deciseconds */
			showBrightness = true;
			setTime = false;
		} else {
			buttonsLocked = 5; /* deciseconds */
			showBrightness = false;
			setTime = true;
		}
//		if (autoBrightness == false) {
//			buttonsLocked = 15; /* deciseconds */
//			autoBrightness = true;
//			running_letters("AB ON", 100);
//		} else {
//			buttonsLocked = 10; /* deciseconds */
//			autoBrightness = false;
//			running_letters("AB OFF", 100);
//		}
	}
	if (buttonState[BUT_RED_1] == BUT_ON) {
		buttonState[BUT_RED_1] = BUT_OFF;
		buttonsLocked = 5; /* deciseconds */
		sec = 0;
		if(++min >= 60){
			min = 0;
		}
	}
	if (buttonState[BUT_RED_2] == BUT_ON) {
		buttonState[BUT_RED_2] = BUT_OFF;
		buttonsLocked = 5; /* deciseconds */
		sec = 0;
		if (min-- == 0) {
			min = 59;
		}
	}
	if (buttonState[BUT_BLUE_1] == BUT_ON) {
		buttonState[BUT_BLUE_1] = BUT_OFF;
		buttonsLocked = 5; /* deciseconds */
		sec = 0;
		if(++hour >= 24){
			hour = 0;
		}
	}
	if (buttonState[BUT_BLUE_2] == BUT_ON) {
		buttonState[BUT_BLUE_2] = BUT_OFF;
		buttonsLocked = 5; /* deciseconds */
		sec = 0;
		if (hour-- == 0) {
			hour = 23;
		}
	}
}

#ifndef MATTHIS
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
#endif

/* displays the temperature on the led matrix (only below 100°) */
static void displayTemperature(void) {
	/* do not write time during temperature display */
	setTime = false;

	/* do not draw whilst writing into the data array */
	T0_DISABLE_INTR();

	/* clear the data array */
	clearAll();

	if (temperature[0] == '+') {
		/* draw a plus sign */
		data[3][0] = 255;
		data[3][1] = 255;
		data[3][2] = 255;
		data[2][1] = 255;
		data[4][1] = 255;
	} else {
		/* draw a minus sign */
		data[3][0] = 255;
		data[3][1] = 255;
		data[3][2] = 255;
	}
	horizontal_num(4, temperature[1] - '0');
	horizontal_num(8, temperature[2] - '0');
	/* the dot */
	data[6][12] = 255;
	horizontal_num(14, temperature[4] - '0');

	T0_ENABLE_INTR();
	_delay_ms(3000);
	setTime = true;
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
