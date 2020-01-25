/*
 * temp_adc.c
 *
 *  Created on: Feb 10, 2019
 *      Author: Matt Flanagan
 */
#include "temp_adc.h"

Success_t init_LTC2497();

uint8_t temp_array[READ_MSG_SIZE];
uint16_t adc_val, adc_val0, adc_val1, adc_val2, adc_val3, adc_val4;
const uint8_t channel[NUM_CHANNELS] = {CHANNEL_0, CHANNEL_1,
                                       CHANNEL_2, CHANNEL_3, CHANNEL_4, CHANNEL_5, CHANNEL_6,
                                       CHANNEL_7, CHANNEL_8, CHANNEL_9, CHANNEL_10, CHANNEL_11,
                                       CHANNEL_12, CHANNEL_13, CHANNEL_14, CHANNEL_15
                                      };

flag_t conv_complete;

void task_acquire_temp() {
  conv_complete = DEASSERTED;
  uint8_t toggle = 0;
  uint8_t i = 0;
  uint8_t read_byte = 0;
  uint8_t write_data[WRITE_MSG_SIZE];
  init_LTC2497(); //don't need to handle an error TODO: (maybe use a while loop)


  TickType_t time_init = 0;
  while (1) {
    time_init = xTaskGetTickCount();
    
    if (toggle % 2 == 0) {
      //process temp1
      //todo: read_temp1();
      //todo: process_temp(temp_values* temps);
      //todo: add temps to tx_queue
      for (i = 0; i < (NUM_TEMP / 2); i++) {
      	vTaskDelay(WRITE_REQ_WAIT);
        conv_complete = DEASSERTED;
        write_data[0] = set_address(ID_TEMP_1, WRITE_ENABLE);
        write_data[1] = channel_combine(channel[i]);
        write_data[2] = 0xFF;
        HAL_I2C_Master_Transmit_IT(&hi2c1, (uint16_t) write_data[0], &write_data[1], WRITE_MSG_SIZE);
        while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
				{
				}
        //TODO optimize how it sends the receive
        vTaskDelay(READ_REQ_WAIT);
        read_byte = set_address(ID_TEMP_1, READ_ENABLE);
        HAL_I2C_Master_Receive_IT(&hi2c1,(uint16_t) read_byte, &temp_array[0], READ_MSG_SIZE);
        while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
				{
				}

        adc_val = adc_extract(&temp_array);
//        //TODO: Raymond need to conver the adc_val of this to actual temperature
        if (xSemaphoreTake(bms.temp.sem, TIMEOUT) == pdTRUE) {
        	bms.temp.data[i] = adc_val;
        	xSemaphoreGive(bms.temp.sem);
        } else {
        	//error rip
        }
      }
      
    } else {
      //process temp2
      //todo: read_temp2();
      //todo: process_temp(temp_values* temps);
    }
    
    vTaskDelayUntil(&time_init, ACQUIRE_TEMP_RATE);
  }
}

Success_t init_LTC2497() {
  Success_t success1 = SUCCESSFUL;
  Success_t success2 = SUCCESSFUL;
  //try to initialize communication with the I2C chip
//  if (HAL_I2C_IsDeviceReady(bms.i2c, ID_TEMP_1 << 1, TRIALS, TIMEOUT) != HAL_OK) {
//    //Device not connected
//    success1 = FAILURE;
//    if (xSemaphoreTake(bms.state_sem, TIMEOUT) == pdPASS) {
//      bms.state = ERROR_BMS;
//      xSemaphoreGive(bms.state_sem); //release sem
//    }
//  } else {
//    bms.temp1_con = 1;
//  }
  
//  if (HAL_I2C_IsDeviceReady(bms.i2c, ID_TEMP_2, TRIALS, TIMEOUT) != HAL_OK) {
//    //Device not connected
//    success2 = FAILURE;
//    if (xSemaphoreTake(bms.state_sem, TIMEOUT) == pdPASS) {
//      bms.state = ERROR_BMS;
//      xSemaphoreGive(bms.state_sem); //release sem
//    }
//  } else {
//    bms.temp2_con = 1;
//  }
//
  if (success2 == FAILURE || success1 == FAILURE) {
    return FAILURE;
  } else {
    return SUCCESSFUL;
  }
}
