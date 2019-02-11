/*
 * bms.h
 *
 *  Created on: Feb 1, 2019
 *      Author: Matt Flanagan
 */

#ifndef BMS_H_
#define BMS_H_

//Includes
#include "can.h"
#include "vstack.h"
#include "temp_adc.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "main.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_can.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//Constants

//RTOS Constants
#define BMS_MAIN_STACK_SIZE 128
#define BMS_MAIN_PRIORITY		1
#define HEARTBEAT_STACK_SIZE 128
#define HEARTBEAT_PRIORITY  1


//Rates
#define TIMEOUT 				5 / portTICK_RATE_MS
#define HEARTBEAT_RATE 	750 / portTICK_RATE_MS

//structures
enum bms_slave_state {
	LOW_POWER 	= 0,
	INIT				= 1,
	NORMAL_OP 	= 2,
	ERROR_BMS		= 3,
	SOFT_RESET 	= 4,
	SHUTDOWN   	= 5
};

//Main BMS structure that holds can handles and all of the queues
typedef struct
{
  CAN_HandleTypeDef* can;
  I2C_HandleTypeDef* i2c;
  SPI_HandleTypeDef* spi;
  QueueHandle_t 		q_rx_can;
  QueueHandle_t 		q_tx_can;

  SemaphoreHandle_t state_sem;
  enum bms_slave_state state;
  //might not need q to receive since polling TBD
}bms_t;

//Global Variables
volatile bms_t bms;
extern CAN_HandleTypeDef 	hcan1;
extern I2C_HandleTypeDef 	hi2c1;
extern SPI_HandleTypeDef	hspi1;

//Functions
void bms_main();
void initRTOSObjects();

#endif /* BMS_H_ */
