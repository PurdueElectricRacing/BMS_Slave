/***************************************************************************
*
*     File Information
*
*     Name of File: can.c
*
*     Authors (Include Email):
*       1. Matthew Flanagan       matthewdavidflanagan@outlook.com
*
*     File dependents: (header files, flow charts, referenced documentation)
*       1. bms.h
*
*     File Description: Main file that most of the cool stuff goes down in
*     such as RTOS init, can sending to the master, and state machine management
*
***************************************************************************/
#include "bms.h"

Success_t send_faults();
Success_t clear_faults();
void debug_lights(flag_t orange, flag_t red, flag_t green, flag_t blue);

char buffer[400];

/***************************************************************************
*
*     Function Information
*
*     Name of Function: heartbeat
*
*     Programmer's Name: Matt Flanagan
*
*     Function Return Type: None
*
*     Parameters (list data type, name, and comment one per line):
*       1. None
*
*      Global Dependents:
*       1. None
*
*     Function Description: Toggles a gpio at HEARTBEAT_RATE to satisfy the
*     watchdog timer
*
***************************************************************************/
void task_heartbeat() {
  TickType_t time_init = 0;
  while (1) {
    time_init = xTaskGetTickCount();
    HAL_GPIO_TogglePin(HEARTBEAT_GPIO_Port, HEARTBEAT_Pin);
    HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
    vTaskDelayUntil(&time_init, HEARTBEAT_RATE);
  }
}

/***************************************************************************
*
*     Function Information
*
*     Name of Function: task_error_check
*
*     Programmer's Name: Matt Flanagan
*
*     Function Return Type: None
*
*     Parameters (list data type, name, and comment one per line):
*       1. None
*
*      Global Dependents:
*       1. bms.fault
*
*     Function Description: Checks each error case at  ERROR_CHECK_RATE and
*     will fault the BMS if an error has been detected.
*
***************************************************************************/
void task_error_check() {
  TickType_t time_init = 0;
  fault_t fault = NORMAL;
  while (1) {
    time_init = xTaskGetTickCount();
    fault = NORMAL;
    if (bms.state == NORMAL_OP) {
      if (bms.connected == FAULTED ||
          bms.temp1_con == FAULTED ||
          bms.temp2_con == FAULTED ||
          bms.vstack_con == FAULTED) {
        fault = FAULTED;
      }
      
      if (fault == FAULTED) {
        if (xSemaphoreTake(bms.state_sem, TIMEOUT) == pdPASS) {
          bms.state = ERROR_BMS;
          xSemaphoreGive(bms.state_sem); //release sem
        }
      }
    } else {
      //suspend your task, it is task_bms_main's job to restart
//      vTaskSuspend(NULL);
    }
    
    vTaskDelayUntil(&time_init, ERROR_CHECK_RATE);
  }
}


/***************************************************************************
*
*     Function Information
*
*     Name of Function: initRTOSObjects
*
*     Programmer's Name: Matt Flanagan
*
*     Function Return Type: None
*
*     Parameters (list data type, name, and comment one per line):
*       1. None
*
*      Global Dependents:
*       1. bms
*
*     Function Description: Initializes required q's and creates the tasks
*     that freeRTOS will run
*
***************************************************************************/
void initRTOSObjects() {
  int i = 0;
	//define q's
  bms.q_tx_can = xQueueCreate(CAN_TX_Q_SIZE, sizeof(CanTxMsgTypeDef));
  bms.q_rx_can = xQueueCreate(CAN_RX_Q_SIZE, sizeof(CanRxMsgTypeDef));
  
  //start tasks
  xTaskCreate(task_txCan, "Transmit Can", CAN_TX_STACK_SIZE, NULL, CAN_TX_PRIORITY, *(TaskHandle_t*) &bms.tasks[i++]);
  xTaskCreate(task_CanProcess, "Process Can", CAN_RX_STACK_SIZE, NULL, CAN_RX_PRIORITY, *(TaskHandle_t*) &bms.tasks[i++]);
  xTaskCreate(task_bms_main, "Main Task", BMS_MAIN_STACK_SIZE, NULL, BMS_MAIN_PRIORITY, *(TaskHandle_t*) &bms.tasks[i++]);
  xTaskCreate(task_heartbeat, "Heartbeat", HEARTBEAT_STACK_SIZE, NULL, HEARTBEAT_PRIORITY, *(TaskHandle_t*) &bms.tasks[i++]);
  xTaskCreate(task_Master_WDawg, "Master WDawg", WDAWG_STACK_SIZE, NULL, WDAWG_PRIORITY, *(TaskHandle_t*) &bms.tasks[i++]);
  xTaskCreate(task_VSTACK, "VSTACK", VSTACK_STACK_SIZE, NULL, VSTACK_PRIORITY, *(TaskHandle_t*) &bms.tasks[i++]);
  xTaskCreate(task_acquire_temp, "temp", ACQUIRE_TEMP_STACK_SIZE, NULL, ACQUIRE_TEMP_PRIORITY, *(TaskHandle_t*) &bms.tasks[i++]);
  xTaskCreate(task_broadcast, "broadcast", BROAD_STACK_SIZE, NULL, BROAD_PRIORITY, *(TaskHandle_t*) &bms.tasks[i++]);
  xTaskCreate(task_error_check, "Error Check", ERROR_CHECK_STACK_SIZE, NULL,
              ERROR_CHECK_RATE_PRIORITY, *(TaskHandle_t*) &bms.tasks[i++]);
              
}

/***************************************************************************
*
*     Function Information
*
*     Name of Function: initBMSobject
*
*     Programmer's Name: Matt Flanagan
*
*     Function Return Type: None
*
*     Parameters (list data type, name, and comment one per line):
*       1.
*
*      Global Dependents:
*       1. bms
*
*     Function Description: Initialize the BMS structure
***************************************************************************/
void initBMSobject(flag_t first) {
  uint8_t x = 0;

  if (first == ASSERTED) {
    bms.state_sem = xSemaphoreCreateBinary();
    bms.param.sem = xSemaphoreCreateBinary();
    bms.can = &hcan1;
    bms.spi = &hspi1;
    bms.i2c = &hi2c1;
    bms.temp.sem = xSemaphoreCreateBinary();
    bms.vtap.sem = xSemaphoreCreateBinary();

    wdawg.master_sem = xSemaphoreCreateBinary();

    xSemaphoreGive(bms.temp.sem);
    xSemaphoreGive(bms.vtap.sem);

    xSemaphoreGive(wdawg.master_sem); //allows it to be taken

    xSemaphoreGive(bms.state_sem);
    xSemaphoreGive(bms.param.sem);
  }

  bms.connected = FAULTED;
  bms.passive_en = DEASSERTED;
  bms.temp1_con = NORMAL; //todo: change when integrated
  bms.temp2_con = NORMAL; //unused for senior design TODO: fix when it's real
  bms.vstack_con = NORMAL; //todo: change when integrated
  
  bms.param.temp_msg_en = ASSERTED;
  bms.param.volt_msg_en = ASSERTED;
  bms.param.temp_msg_rate = TEMP_POLL_RATE;
  bms.param.volt_msg_rate = VOLT_POLL_RATE;
  
  for (x = 0; x < NUM_VTAPS; x ++) {
    bms.vtap.data[x] = VOLT_LOW_IMPOS;
  }
  for (x = 0; x < NUM_TEMP; x ++) {
    bms.temp.data[x] = TEMP_LOW_IMPOS;
  }
  
  wdawg.new_msg = xTaskGetTickCount();
}

/***************************************************************************
*
*     Function Information
*
*     Name of Function: task_bms_main
*
*     Programmer's Name: Matt Flanagan
*
*     Function Return Type: None
*
*     Parameters (list data type, name, and comment one per line):
*       1.
*
*      Global Dependents:
*       1. bms
*
*     Function Description: Main execution loop of the program. Will collect all
*     processed information and send it out via can to main_bms. Will also have the
*     ability to update paramaters on the fly
***************************************************************************/
void task_bms_main() {
  TickType_t time_init = 0;

  CanTxMsgTypeDef msg;

  xQueueSendToBack(bms.q_tx_can, &msg, 100);
  while (1) {
    //just chilling
    time_init = xTaskGetTickCount();
    switch (bms.state) {
      case LOW_POWER:
        debug_lights(0, 0, 0, 0);

        //must have been woken up by interrupt
        if (xSemaphoreTake(bms.state_sem, TIMEOUT) == pdPASS) {
          bms.state = INIT;
          xSemaphoreGive(bms.state_sem); //release sem
        }
        break;
      case INIT:
        debug_lights(0, 0, 0, 1);
        HAL_GPIO_WritePin(LPM_GPIO_Port, LPM_Pin, GPIO_PIN_SET);
        if (bms.connected && bms.vstack_con && bms.temp1_con &&
            bms.temp2_con) { //only move to normal op when everything is connected
          if (xSemaphoreTake(bms.state_sem, TIMEOUT) == pdPASS) {
            bms.state = NORMAL_OP;
            xSemaphoreGive(bms.state_sem); //release sem
          }
//          //re-enable the non-critical tasks
//          vTaskResume(bms.tasks[BROADCAST]);
//          vTaskResume(bms.tasks[ERROR_CHECK]);
        }
        break;
      case NORMAL_OP:
        debug_lights(0, 0, 1, 0);
        //TODO: read from all of the sensors
        //TODO: manage passive balancing if necessary
        break;
      case ERROR_BMS:
        debug_lights(0, 0, 1, 1);
        send_faults();
        if (bms.connected) {
          vTaskDelay(SEND_ERROR_DELAY);
        } else {
          if (xSemaphoreTake(bms.state_sem, TIMEOUT) == pdPASS) {
            bms.state = SHUTDOWN;
            xSemaphoreGive(bms.state_sem); //release sem
          }
        }
        break;
      case SHUTDOWN:
        debug_lights(0, 1, 0, 0);
        //tell the WDAWG to disable
        //delete uneccessary tasks
        HAL_GPIO_WritePin(LPM_GPIO_Port, LPM_Pin, GPIO_PIN_RESET); //active low
        //todo: disable the SPI/I2C periphs so only wakeup on can
        //enter sleep mode and wait for interrupt to wake back up
//        vTaskSuspendAll(); //disable the scheduler
//        HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
//        HAL_Delay(500);
//        HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
//
//        HAL_SuspendTick();
//        HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
//        HAL_ResumeTick();
//        xTaskResumeAll();
        break;
      default:
        break;
    }
    
    msg.IDE = CAN_ID_STD;
    msg.RTR = CAN_RTR_DATA;
    msg.DLC = 2; //one for the macro faults
    msg.StdId = 0x400;

    msg.Data[0] = ID_SLAVE;
    msg.Data[1] = (uint8_t) bms.state;

    xQueueSendToBack(bms.q_tx_can, &msg, TIMEOUT);
    vTaskDelayUntil(&time_init, BMS_MAIN_RATE);
  }
  //never get here
}

/***************************************************************************
*
*     Function Information
*
*     Name of Function: clear_faults
*
*     Programmer's Name: Matt Flanagan
*
*     Function Return Type: Success status
*
*     Parameters (list data type, name, and comment one per line):
*       1.
*
*      Global Dependents:
*       1.
*
*     Function Description: clears all pending faults in the BMS
***************************************************************************/
Success_t clear_faults() {
  bms.connected = NORMAL;
  bms.temp1_con = NORMAL;
  bms.temp2_con = NORMAL;
  bms.vstack_con = NORMAL;
  
  return SUCCESSFUL;
}

/***************************************************************************
*
*     Function Information
*
*     Name of Function: wakeup_slaves
*
*     Programmer's Name: Matt Flanagan
*
*     Function Return Type: Success status
*
*     Parameters (list data type, name, and comment one per line):
*       1.
*
*      Global Dependents:
*       1. q_tx_bmscan
*
*     Function Description: sends fault code to the GUI see CAN msg docs for
*     more info on what each bit is representing
***************************************************************************/
Success_t send_faults() {
  CanTxMsgTypeDef msg;
  msg.IDE = CAN_ID_STD;
  msg.RTR = CAN_RTR_DATA;
  msg.DLC = ERROR_MSG_LENGTH; //one for the macro faults
  msg.StdId = ID_SLAVE_FAULT_CODE;
  
  msg.Data[0] = ID_SLAVE;
  msg.Data[1] = bitwise_or(FAULT_VOLT_SHIFT, FAULT_VOLT_MASK, bms.vstack_con);
  msg.Data[1] |= bitwise_or(FAULT_TEMP1_SHIFT, FAULT_TEMP1_MASK, bms.temp1_con);
  msg.Data[1] |= bitwise_or(FAULT_TEMP2_SHIFT, FAULT_TEMP2_MASK, bms.temp2_con);
  msg.Data[1] |= bitwise_or(FAULT_BMSCON_SHIFT, FAULT_BMSCON_MASK, bms.connected);
  
  msg.Data[1] = ~(msg.Data[1]) & 0x0F;
  xQueueSendToBack(bms.q_tx_can, &msg, 100);
  return SUCCESSFUL;
}

void debug_lights(flag_t green, flag_t blue, flag_t orange, flag_t red) {

  if (orange == ASSERTED) {
    HAL_GPIO_WritePin(ORANGE_LED_GPIO_Port, ORANGE_LED_Pin, GPIO_PIN_SET);
  } else {
    HAL_GPIO_WritePin(ORANGE_LED_GPIO_Port, ORANGE_LED_Pin, GPIO_PIN_RESET);
  }
  if (red == ASSERTED) {
    HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_SET);
  } else {
    HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
  }
  if (green == ASSERTED) {
    HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_SET);
  } else {
    HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
  }
  if (blue == ASSERTED) {
    HAL_GPIO_WritePin(BLUE_LED_GPIO_Port, BLUE_LED_Pin, GPIO_PIN_SET);
  } else {
    HAL_GPIO_WritePin(BLUE_LED_GPIO_Port, BLUE_LED_Pin, GPIO_PIN_RESET);
  }
}














