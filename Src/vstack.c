/*
 * ltc6811.c
 *
 *  Created on: Feb 10, 2019
 *      Author: Matt Flanagan
 */

#include "vstack.h"

void task_VSTACK() {
  TickType_t time_init = 0;
  
  while (1) {
    time_init = xTaskGetTickCount();
    
    vTaskDelayUntil(&time_init, VSTACK_RATE);
  }
}

HAL_StatusTypeDef init_LTC6811() {
  //this function is used to initiate communication to the LTC6811 chip
  
  return HAL_OK;
}


