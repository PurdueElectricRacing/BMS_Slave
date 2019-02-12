/*
 * temp_adc.c
 *
 *  Created on: Feb 10, 2019
 *      Author: Matt Flanagan
 */
#include "temp_adc.h"

void task_acquire_temp() {
  HAL_StatusTypeDef status = HAL_OK;
  uint8_t toggle = 0;
  status = init_LTC2497(); //don't need to handle an error TODO: (maybe use a while loop)
  TickType_t time_init = 0;
  while (1) {
    time_init = xTaskGetTickCount();
    
    if (toggle % 2 == 0) {
      //process temp1
      //todo: read_temp1();
      //todo: process_temp(temp_values* temps);
      //todo: add temps to tx_queue
    } else {
      //process temp2
      //todo: read_temp2();
      //todo: process_temp(temp_values* temps);
      //todo: add temps to tx_queue
    }
    vTaskDelayUntil(&time_init, ACQUIRE_TEMP_RATE);
  }
}

HAL_StatusTypeDef init_LTC2497() {
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
  
  return success1 && success2;
}
