#include <stdio.h>
#include <stdint.h>

#include "globals.h"

/* Thermometer Connections */
#define THERM_PORT PORTA
#define THERM_DDR DDRA
#define THERM_PIN PINA
#define THERM_DQ PA6
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

static byte therm_reset() {
	byte i;

	//Pull line low and wait for 480uS
	cli();
	THERM_LOW();
	THERM_OUTPUT_MODE();
	_delay_us(480);
	//Release line and wait for 60uS
	THERM_INPUT_MODE();
	_delay_us(60);
	//Store line value and wait until the completion of 480uS period
	i = (THERM_PIN & (1 << THERM_DQ));
	_delay_us(420);
	sei();
	//Return the value read from the presence pulse (0=OK, 1=WRONG)
	return i;
}

static void therm_write_bit(byte bit) {
	//Pull line low for 1uS
	cli();
	THERM_LOW();
	THERM_OUTPUT_MODE();
	_delay_us(1);
	//If we want to write 1, release the line (if not will keep low)
	if (bit)
		THERM_INPUT_MODE();
	//Wait for 60uS and release the line
	_delay_us(60);
	THERM_INPUT_MODE();
	sei();
}

static byte therm_read_bit(void) {
	byte bit = 0;
	//Pull line low for 1uS
	cli();
	THERM_LOW();
	THERM_OUTPUT_MODE();
	_delay_us(1);
	//Release line and wait for 14uS
	THERM_INPUT_MODE();
	_delay_us(14);
	//Read line value
	if (THERM_PIN & (1 << THERM_DQ))
		bit = 1;
	//Wait for 45uS to end and return read value
	_delay_us(45);
	sei();
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

void therm_initiate_temperature_read() {

	/* Reset, skip ROM and start temperature conversion */
	therm_reset();
	therm_write_byte(THERM_CMD_SKIPROM);
	therm_write_byte(THERM_CMD_CONVERTTEMP);
}

/*
 * Der DS18S20 braucht 750 ms fuer die Umwandlung, so lange sollte man warten
 * nach dem Call an initiate_temperature_read, sonst wartet man hier in der
 * while-Schleife
 * Buffer length must be at least 9 Bytes! ["+YYY.X C"]
 */
void therm_get_temperature(char *buffer) {
	byte temperature[2];
	int8_t digit;
	/* Wait until conversion is complete */
	while (!therm_read_bit())
		;
	/* Reset, skip ROM and send command to read Scratchpad */
	therm_reset();
	therm_write_byte(THERM_CMD_SKIPROM);
	therm_write_byte(THERM_CMD_RSCRATCHPAD);
	//Read Scratchpad (only 2 first bytes)
	temperature[0] = therm_read_byte();
	temperature[1] = therm_read_byte();
	therm_reset();
	//Store temperature integer digits and decimal digits
	digit = temperature[0] >> 1;
	digit |= temperature[1] << 7;
	//Store decimal digits
	//Format temperature into a string [+YYY.X C]
	/* If first bit is set, its .5 */
	if (temperature[0] & 1) {
		snprintf(buffer, 12, "%+d.5 C", digit);
	} else {
		snprintf(buffer, 12, "%+d.0 C", digit);
	}
}
