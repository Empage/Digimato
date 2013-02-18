/*
 * conrad_dcf.h
 *
 *  Created on: Nov 26, 2012
 *      Author: matthis
 */

#ifndef CONRAD_DCF_H_
#define CONRAD_DCF_H_

#include "globals.h"

void conrad_state_init_dcf();
byte conrad_state_get_dcf_data();
byte conrad_get_dcf_data(byte* dcf_data);
byte conrad_check_parity();
void conrad_calculate_time();
void conrad_calculate_date();

#endif /* CONRAD_DCF_H_ */
