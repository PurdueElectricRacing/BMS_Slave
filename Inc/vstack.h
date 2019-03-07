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

HAL_StatusTypeDef init_LTC2497();
void task_VSTACK();

typedef struct {
	SPI_HandleTypeDef * spi;
	uint8_t spi_addr;

}LTCHandle_t;


#endif /* VSTACK_H_ */
