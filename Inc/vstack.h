/*
 * ltc6811.h
 *
 *  Created on: Feb 10, 2019
 *      Author: Matt Flanagan
 */

#ifndef VSTACK_H_
#define VSTACK_H_

#include "bms.h"

#define VSTACK_STACK_SIZE 128
#define VSTACK_PRIORITY   1
#define VSTACK_RATE       20 / portTICK_RATE_MS

void task_VSTACK();
HAL_StatusTypeDef LTC6811_addrWrite(uint8_t * din,
		uint8_t len, uint16_t cmd);

#endif /* VSTACK_H_ */
