/*
 * main.h
 *
 *  Created on: Nov 26, 2012
 *      Author: matthis
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "globals.h"

void drawWithBrightness(void);
void tick(void);
void vertical_time(void);
void vertical_num(uint8_t posx, uint8_t posy, uint8_t number);
void place_mono_char_checked(int16_t pos,uint8_t zeichen);
void running_letters(char* str, byte time);
void running_letters_simple(char* str);

#endif /* MAIN_H_ */
