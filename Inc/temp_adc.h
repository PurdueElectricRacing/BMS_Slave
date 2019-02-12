/*
 * temp_adc.h
 *
 *  Created on: Feb 10, 2019
 *      Author: Matt Flanagan
 */

#ifndef TEMP_ADC_H_
#define TEMP_ADC_H_

#include "bms.h"

#define ID_TEMP_1   0x7f
#define ID_TEMP_2   0x7e
#define TRIALS      2
#define TIMEOUT     50 / portTICK_RATE_MS

#define ACQUIRE_TEMP_STACK_SIZE 128
#define ACQUIRE_TEMP_PRIORITY   1

//read each set of temps at 2xRate
#define ACQUIRE_TEMP_RATE       500 / portTICK_RATE_MS

HAL_StatusTypeDef init_LTC2497();
void task_acquire_temp();

#endif /* TEMP_ADC_H_ */
