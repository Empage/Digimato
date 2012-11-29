/*
 * main.h
 *
 *  Created on: Nov 26, 2012
 *      Author: matthis
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "defines.h"

void mainLoop(void);
void pollSwichtes(void);
/* Animations */
void tickSecondAnimation(void);
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


#endif /* MAIN_H_ */
