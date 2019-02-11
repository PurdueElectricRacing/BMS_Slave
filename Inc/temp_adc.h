/*
 * temp_adc.h
 *
 *  Created on: Feb 10, 2019
 *      Author: Matt Flanagan
 */

#ifndef TEMP_ADC_H_
#define TEMP_ADC_H_

#include "bms.h"

#define ID_TEMP_1		0x7f
#define ID_TEMP_2 	0x7e
#define TRIALS			2
#define TIMEOUT			50 / portTICK_RATE_MS

#endif /* TEMP_ADC_H_ */
