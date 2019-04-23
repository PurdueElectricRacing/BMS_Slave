///*
// * ltc6811.c
// *
// *  Created on: Feb 10, 2019
// *      Author: Matt Flanagan
// */
//
#include "vstack.h"

HAL_StatusTypeDef init_LTC6811();

void task_VSTACK() {
	TickType_t time_init = 0;
	uint8_t i;
	uint8_t x;
	uint8_t data[8];
	uint16_t pec_val;
	uint16_t recv_pec_val;

	init_LTC6811();

	while (1) {
		time_init = xTaskGetTickCount();

		//start voltage conversion on all cells
		wakeup_sleep(1);
		LTC681x_adcv(LTC6811_MD_10, DISCHARGE_NOT_PERMITTED, LTC6811_ADC_CALL);
		//poll until it is complete
		LTC681x_pollAdc();
		//read the ADC values
		for (i = 1; i <= (NUM_VTAPS / 3); i++) { //each cell reg contains 3 voltage values
		  LTC681x_rdcv_reg((cell_groups_t) i, 1, data);
		  //confirm the PEC value is correct
		  recv_pec_val = byte_combine(data[6], data[7]);
		  pec_val = LTC6811Pec(data, 6);
		  if (recv_pec_val == pec_val) {
		    //valid voltage data update the table
		    if (xSemaphoreTake(bms.vtap.sem, TIMEOUT) == pdTRUE) {
		      x = (i-1) * 3;
		      bms.vtap.data[x++] = byte_combine(data[1], data[0]);
		      bms.vtap.data[x++] = byte_combine(data[3], data[2]);
		      bms.vtap.data[x] = byte_combine(data[5], data[4]);
		      xSemaphoreGive(bms.vtap.sem);
		    }
		  }
		  else {
		    //TODO bad voltage value what to do
		  }
		}

		vTaskDelayUntil(&time_init, VSTACK_RATE);
	}
}

HAL_StatusTypeDef init_LTC6811() {
	//this function is used to initiate communication to the LTC6811 chip
  wakeup_sleep(1);

  //initialize the LTC8584's to be disabled and sending the voltage values back
  LTC681x_clrsctrl();

	return HAL_OK;
}
