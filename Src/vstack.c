///*
// * ltc6811.c
// *
// *  Created on: Feb 10, 2019
// *      Author: Matt Flanagan
// */
//
#include "vstack.h"

HAL_StatusTypeDef LTC6811_init();

void task_VSTACK() {
	TickType_t time_init = 0;

	while (1) {
		time_init = xTaskGetTickCount();
		uint8_t din[1] = { 0x1 };
		LTC6811_init();
		//LTC6811_addrWrite(din, 1,  LTC6811_CMD_WRCFGA);
		vTaskDelayUntil(&time_init, VSTACK_RATE);
	}
}

HAL_StatusTypeDef init_LTC6811() {
	//this function is used to initiate communication to the LTC6811 chip

	return HAL_OK;
}
