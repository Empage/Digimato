/*
 * thermometer.c
 *
 *  Created on: Nov 30, 2012
 *      Author: matthis
 */

#include <stdio.h>
#include <stdint.h>

#include "globals.h"

#define LOOP_CYCLES 8 //Number of cycles that the loop takes
#define us(num) (num/(LOOP_CYCLES*(1/(F_CPU/1000000.0))))

/* Thermometer Connections (At your choice) */
#define THERM_PORT PORTC
#define THERM_DDR DDRC
#define THERM_PIN PINC
#define THERM_DQ PC0
/* Utils */
#define THERM_INPUT_MODE() THERM_DDR&=~(1<<THERM_DQ)
#define THERM_OUTPUT_MODE() THERM_DDR|=(1<<THERM_DQ)
#define THERM_LOW() THERM_PORT&=~(1<<THERM_DQ)
#define THERM_HIGH() THERM_PORT|=(1<<THERM_DQ)

#define THERM_CMD_CONVERTTEMP 0x44
#define THERM_CMD_RSCRATCHPAD 0xbe
#define THERM_CMD_WSCRATCHPAD 0x4e
#define THERM_CMD_CPYSCRATCHPAD 0x48
#define THERM_CMD_RECEEPROM 0xb8
#define THERM_CMD_RPWRSUPPLY 0xb4
#define THERM_CMD_SEARCHROM 0xf0
#define THERM_CMD_READROM 0x33
#define THERM_CMD_MATCHROM 0x55
#define THERM_CMD_SKIPROM 0xcc
#define THERM_CMD_ALARMSEARCH 0xec

inline __attribute__((gnu_inline)) void therm_delay(uint16_t delay) {
	while (delay--)
		asm volatile("nop");
}

static byte therm_reset() {
	byte i;

	//Pull line low and wait for 480uS
	THERM_LOW();
	THERM_OUTPUT_MODE();
	therm_delay(us(480));
	//Release line and wait for 60uS
	THERM_INPUT_MODE();
	therm_delay(us(60));
	//Store line value and wait until the completion of 480uS period
	i = (THERM_PIN & (1 << THERM_DQ));
	therm_delay(us(420));
	//Return the value read from the presence pulse (0=OK, 1=WRONG)
	return i;
}

static void therm_write_bit(byte bit) {
	//Pull line low for 1uS
	THERM_LOW();
	THERM_OUTPUT_MODE();
	therm_delay(us(1));
	//If we want to write 1, release the line (if not will keep low)
	if (bit)
		THERM_INPUT_MODE();
	//Wait for 60uS and release the line
	therm_delay(us(60));
	THERM_INPUT_MODE();
}

static byte therm_read_bit(void) {
	byte bit = 0;
	//Pull line low for 1uS
	THERM_LOW();
	THERM_OUTPUT_MODE();
	therm_delay(us(1));
	//Release line and wait for 14uS
	THERM_INPUT_MODE();
	therm_delay(us(14));
	//Read line value
	if (THERM_PIN & (1 << THERM_DQ))
		bit = 1;
	//Wait for 45uS to end and return read value
	therm_delay(us(45));
	return bit;
}

static byte therm_read_byte(void) {
	byte i = 8, n = 0;
	while (i--) {

		//Shift one position right and store read value
		n >>= 1;
		n |= (therm_read_bit() << 7);
	}
	return n;
}

static void therm_write_byte(byte out) {
	byte i = 8;

	while (i--) {
		//Write actual bit and shift one position right to make	the next bit ready
		therm_write_bit(out & 1);
		out >>= 1;
	}
}

#define THERM_DECIMAL_STEPS_12BIT 625 //.0625
void therm_read_temperature(char *buffer) {
	// Buffer length must be at least 12bytes long! ["+XXX.XXXX C"]
	byte temperature[2];
	int8_t digit;
	uint16_t decimal;
	//Reset, skip ROM and start temperature conversion
	therm_reset();
	therm_write_byte(THERM_CMD_SKIPROM);
	therm_write_byte(THERM_CMD_CONVERTTEMP);
	//Wait until conversion is complete
	while (!therm_read_bit())
		;
	//Reset, skip ROM and send command to read Scratchpad
	therm_reset();
	therm_write_byte(THERM_CMD_SKIPROM);
	therm_write_byte(THERM_CMD_RSCRATCHPAD);
	//Read Scratchpad (only 2 first bytes)
	temperature[0] = therm_read_byte();
	temperature[1] = therm_read_byte();
	therm_reset();
	//Store temperature integer digits and decimal digits
	digit = temperature[0] >> 4;
	digit |= (temperature[1] & 0x7) << 4;
	//Store decimal digits
	decimal = temperature[0] & 0xf;
	decimal *= THERM_DECIMAL_STEPS_12BIT;
	//Format temperature into a string [+XXX.XXXX C]
	snprintf(buffer, 12, "%+d.%04u C", digit, decimal);
}
