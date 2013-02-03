/*
 * conrad_dcf.h
 *
 *  Created on: Nov 26, 2012
 *      Author: matthis
 */

#ifndef CONRAD_DCF_H_
#define CONRAD_DCF_H_

#include "globals.h"

byte conrad_get_dcf_data(byte* dcf_data);
byte conrad_check_parity(byte* dcf_data);
void conrad_calculate_time(byte* dcf_data);
void conrad_calculate_date(byte* dcf_data);

#endif /* CONRAD_DCF_H_ */
