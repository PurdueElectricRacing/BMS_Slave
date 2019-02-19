/*
 * bms.h
 *
 *  Created on: Feb 1, 2019
 *      Author: Matt Flanagan
 */

#ifndef BMS_H_
#define BMS_H_

//Includes
#include "main.h"
#include "cmsis_os.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_can.h"
#include "FreeRTOS.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "can.h"
#include "vstack.h"
#include "temp_adc.h"

//Constants

//RTOS Constants
#define BMS_MAIN_STACK_SIZE 128
#define BMS_MAIN_PRIORITY   1
#define HEARTBEAT_STACK_SIZE 128
#define HEARTBEAT_PRIORITY  1


//Rates
#define TIMEOUT         5 / portTICK_RATE_MS
#define HEARTBEAT_RATE  750 / portTICK_RATE_MS
// Defaults (can be configured by master in real time)
#define TEMP_POLL_RATE	1000 / portTICK_RATE_MS
#define VOLT_POLL_RATE	25 / portTICK_RATE_MS

//enums
typedef enum {
  SUCCESSFUL = 0,
  FAILURE = 1,
} Success_t;

typedef enum flag_state {
  ASSERTED = 1,
  DEASSERTED = 0,
} flag_t;

//structures
enum bms_slave_state {
  LOW_POWER   = 0,
  INIT        = 1,
  NORMAL_OP   = 2,
  ERROR_BMS   = 3,
  SOFT_RESET  = 4,
  SHUTDOWN    = 5
};

typedef struct {
  //broadcasts
  flag_t volt_msg_en;
  flag_t temp_msg_en;

  //rates can be max BROADCAST_RATE hz and can be any integer multiple of that
  uint16_t volt_msg_rate;
  uint16_t temp_msg_rate;

  SemaphoreHandle_t sem;
} params_t;

//Main BMS structure that holds can handles and all of the queues
typedef struct {
  CAN_HandleTypeDef* can;
  I2C_HandleTypeDef* i2c;
  SPI_HandleTypeDef* spi;
  QueueHandle_t     q_rx_can;
  QueueHandle_t     q_tx_can;
  uint8_t           connected; //used to determine if connected to master
  uint8_t           vstack_con; //connected to the vstack
  uint8_t           temp1_con;
  uint8_t           temp2_con;
  uint8_t           passive_en;
  
  params_t 					param;

  SemaphoreHandle_t state_sem;
  enum bms_slave_state state;
  //might not need q to receive since polling TBD
} bms_t;

//Global Variables
volatile bms_t bms;
extern CAN_HandleTypeDef  hcan1;
extern I2C_HandleTypeDef  hi2c1;
extern SPI_HandleTypeDef  hspi1;

//Functions
void task_bms_main();
void initRTOSObjects();

#endif /* BMS_H_ */
