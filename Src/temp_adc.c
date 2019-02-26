/*
 * temp_adc.c
 *
 *  Created on: Feb 10, 2019
 *      Author: Matt Flanagan
 */
#include "temp_adc.h"

uint8_t temp_array[READ_MSG_SIZE];
flag_t conv_complete;

void task_acquire_temp() {
	Success_t success = SUCCESSFUL;
	conv_complete = DEASSERTED;
  uint8_t toggle = 0;
  uint8_t i = 0;
  uint8_t write_data[WRITE_MSG_SIZE];
  uint16_t temp = 0;
  success = init_LTC2497(); //don't need to handle an error TODO: (maybe use a while loop)

  TickType_t time_init = 0;
  while (1) {
    time_init = xTaskGetTickCount();
    
    if (toggle % 2 == 0) {
      //process temp1
      //todo: read_temp1();
      //todo: process_temp(temp_values* temps);
      //todo: add temps to tx_queue
    	for (i = 0; i < (NUM_TEMP / 2); i++) {
    		conv_complete = DEASSERTED;
    		write_data[0] = ID_TEMP_1 << 1 | WRITE_ENABLE;
    		//HAL_I2C_Master_Transmit(&hi2c1, , )
    		//write a channel select blocking
    		//read in nonblocking mode 24 bits
    		//store the data
    	}

    } else {
      //process temp2
      //todo: read_temp2();
      //todo: process_temp(temp_values* temps);
      //todo: add temps to tx_queue
    }

    //pack the data into can msgs
    //send out

    vTaskDelayUntil(&time_init, ACQUIRE_TEMP_RATE);
  }
}

Success_t init_LTC2497() {
  HAL_StatusTypeDef success1 = HAL_OK;
  HAL_StatusTypeDef success2 = HAL_OK;
  //try to initialize communication with the I2C chip
  if (HAL_I2C_IsDeviceReady(bms.i2c, ID_TEMP_1, TRIALS, TIMEOUT) != HAL_OK) {
    //Device not connected
    success1 = HAL_ERROR;
    if (xSemaphoreTake(bms.state_sem, TIMEOUT) == pdPASS) {
      bms.state = ERROR_BMS;
      xSemaphoreGive(bms.state_sem); //release sem
    }
  } else {
    bms.temp1_con = 1;
  }
  
  if (HAL_I2C_IsDeviceReady(bms.i2c, ID_TEMP_2, TRIALS, TIMEOUT) != HAL_OK) {
    //Device not connected
    success2 = HAL_ERROR;
    if (xSemaphoreTake(bms.state_sem, TIMEOUT) == pdPASS) {
      bms.state = ERROR_BMS;
      xSemaphoreGive(bms.state_sem); //release sem
    }
  } else {
    bms.temp2_con = 1;
  }
  
  return (Success_t) (success1 && success2);
}
