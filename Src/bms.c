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

	//main while loop of execution
	//what it does? The world may never know
	while (1) {
		//just chilling
		switch (bms.state) {
			case LOW_POWER:
				break;
			case INIT:
				//establish contact with the master
				//establish contact with Vstack/temp sensors
				break;
			case NORMAL_OP:
				break;
			case SOFT_RESET:
				break;
			case ERROR_BMS:
				break;
			default:
				break;
		}
	}
	//never get here
}

