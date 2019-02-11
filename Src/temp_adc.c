/*
 * temp_adc.c
 *
 *  Created on: Feb 10, 2019
 *      Author: Matt Flanagan
 */
#include "temp_adc.h"

void task_acquire_temp() {

	//try to initialize communication with the I2C chip
	if (HAL_I2C_IsDeviceReady(bms.i2c, ID_TEMP_1, TRIALS, TIMEOUT) != HAL_OK) {
		//Device not connected
		//TODO maybe go to error state here?
	}


}
