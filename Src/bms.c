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
		vTaskDelayUntil(&time_init, HEARTBEAT_RATE);
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
	//define q's
	bms.q_tx_can = xQueueCreate(CAN_TX_Q_SIZE, sizeof(CanTxMsgTypeDef));
	bms.q_rx_can = xQueueCreate(CAN_RX_Q_SIZE, sizeof(CanRxMsgTypeDef));

	//start tasks
	xTaskCreate(task_txCan, "Transmit Can", CAN_TX_STACK_SIZE, NULL, CAN_TX_PRIORITY, NULL);
	xTaskCreate(bms_main, "Main Task", BMS_MAIN_STACK_SIZE, NULL, BMS_MAIN_PRIORITY, NULL);
	xTaskCreate(task_heartbeat, "Heartbeat", HEARTBEAT_STACK_SIZE, NULL, HEARTBEAT_PRIORITY, NULL);
	xTaskCreate(task_Master_WDawg, "Master WDawg", WDAWG_STACK_SIZE, NULL, WDAWG_PRIORITY, NULL);
}

/***************************************************************************
*
*     Function Information
*
*     Name of Function: bms_main
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
void bms_main() {
	bms.can = &hcan1;
	bms.spi = &hspi1;
	bms.i2c = &hi2c1;
	bms.state_sem = xSemaphoreCreateBinary();
	bms.connected = 0;
	bms.passive_en = 0;
	xSemaphoreGive(bms.state_sem);
	//main while loop of execution
	//what it does? The world may never know
	while (1) {
		//just chilling
		switch (bms.state) {
			case LOW_POWER:
				//must have been woken up by interrupt
				if (xSemaphoreTake(bms.state_sem, TIMEOUT) == pdPASS) {
					bms.state = INIT;
					xSemaphoreGive(bms.state_sem); //release sem
				}
				break;
			case INIT:
				//TODO: establish contact with Vstack/temp sensors

				if (bms.connected) { //only move to normal op when everything is connected
					if (xSemaphoreTake(bms.state_sem, TIMEOUT) == pdPASS) {
						bms.state = NORMAL_OP;
						xSemaphoreGive(bms.state_sem); //release sem
					}
				}
				break;
			case NORMAL_OP:
				//TODO: read from all of the sensors
				//TODO: send data to master
				//TODO: manage passive balancing if necessary
				break;
			case ERROR_BMS:
				//TODO: handle error
				if (xSemaphoreTake(bms.state_sem, TIMEOUT) == pdPASS) {
					bms.state = SHUTDOWN;
					xSemaphoreGive(bms.state_sem); //release sem
				}
				break;
			case SHUTDOWN:
				//tell the WDAWG to disable
				HAL_GPIO_WritePin(LPM_GPIO_Port, LPM_Pin, GPIO_PIN_RESET); //active low
				//todo: disable the SPI/I2C periphs so only wakeup on can
				//enter sleep mode and wait for interrupt to wake back up
				if (xSemaphoreTake(bms.state_sem, TIMEOUT) == pdPASS) {
					bms.state = LOW_POWER;
					xSemaphoreGive(bms.state_sem); //release sem
				}
				HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
				break;
			default:
				break;
		}
	}
	//never get here
}

