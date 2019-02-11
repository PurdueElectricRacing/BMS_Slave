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
*       1. can.h
*
*     File Description: This manages all of the can being sent for the slave bms board
*
***************************************************************************/

#include "can.h"

/***************************************************************************
*
*     Function Information
*
*     Name of Function: HAL_CAN_RxFifo0MsgPendingCallback
*
*     Programmer's Name: Matt Flanagan
*
*     Function Return Type: None
*
*     Parameters (list data type, name, and comment one per line):
*       1. CAN_HandleTypeDef *hcan      Can Handle
*
*      Global Dependents:
*       1. None
*
*     Function Description: After a message has been received add it to the
*     rx can queue and move on with life.
*
***************************************************************************/
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  CanRxMsgTypeDef rx;
  TickType_t temp;
  CAN_RxHeaderTypeDef header;
  HAL_CAN_GetRxMessage(hcan, 0, &header, rx.Data);
  rx.DLC = header.DLC;
  rx.StdId = header.StdId;
  xQueueSendFromISR(bms.q_rx_can, &rx, 0);

  //master watchdawg task
  if (xSemaphoreTakeFromISR(wdawg.master_sem, NULL) == pdPASS) {
		//semaphore successfully taken
		temp = wdawg.new_msg;
		wdawg.new_msg = xTaskGetTickCountFromISR();
		wdawg.last_msg = temp;
		xSemaphoreGiveFromISR(wdawg.master_sem, NULL); //give the sem back
  }
}

/***************************************************************************
*
*     Function Information
*
*     Name of Function: master_watchDawg
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
*     Function Description: Monitors can traffic to ensure Master is still in
*     comms, if it doesn't receive a message from Master within x time then
*     enter sleep mode
***************************************************************************/
void task_Master_WDawg() {
	wdawg.master_sem = xSemaphoreCreateBinary();
	wdawg.last_msg = xTaskGetTickCount();
	wdawg.new_msg = xTaskGetTickCount();
	TickType_t time_init = 0;

	xSemaphoreGive(wdawg.master_sem); //allows it to be taken

	while (1) {
		time_init = xTaskGetTickCount();
		if (xSemaphoreTake(wdawg.master_sem, WDAWG_BLOCK) == pdPASS) {
			//semaphore successfully taken
			if ((wdawg.new_msg - wdawg.last_msg) > WDAWG_RATE) {
				//master is not responding go into shutdown
				if (xSemaphoreTake(bms.state_sem, TIMEOUT) == pdPASS) {
					bms.state = SHUTDOWN;
					xSemaphoreGive(bms.state_sem); //release sem
				}
			}
			xSemaphoreGive(wdawg.master_sem);
		}
		vTaskDelayUntil(&time_init, WDAWG_RATE);
	}
}

/***************************************************************************
*
*     Function Information
*
*     Name of Function: task_txCan
*
*     Programmer's Name: Matt Flanagan
*
*     Function Return Type: None
*
*     Parameters (list data type, name, and comment one per line):
*       1. None
*
*      Global Dependents:
*       1. Can queue and such
*
*     Function Description: Task that runs every CAN_TX_RATE and polls for can
*     messages to arrive to send them out to the bms master.
*
***************************************************************************/
void task_txCan() {
  CanTxMsgTypeDef tx;
  TickType_t time_init = 0;
  while (1)
  {
    time_init = xTaskGetTickCount();
    //check if this task is triggered
    if (xQueuePeek(bms.q_tx_can, &tx, TIMEOUT) == pdTRUE)
    {
      xQueueReceive(bms.q_tx_can, &tx, TIMEOUT);  //actually take item out of queue
      CAN_TxHeaderTypeDef header;
      header.DLC = tx.DLC;
      header.IDE = tx.IDE;
      header.RTR = tx.RTR;
      header.StdId = tx.StdId;
      header.TransmitGlobalTime = DISABLE;
      uint32_t mailbox;
      //send the message
      while (!HAL_CAN_GetTxMailboxesFreeLevel(bms.can)); // while mailboxes not free
      HAL_CAN_AddTxMessage(bms.can, &header, tx.Data, &mailbox);
    }
    vTaskDelayUntil(&time_init, CAN_TX_RATE);
  }
}

/***************************************************************************
*
*     Function Information
*
*     Name of Function: can_filter_init
*
*     Programmer's Name: Matt Flanagan
*
*     Function Return Type: None
*
*     Parameters (list data type, name, and comment one per line):
*       1. CAN_HandleTypeDef* hcan        Can Handle
*
*      Global Dependents:
*       1. None
*
*     Function Description: Sets the can filter to only take Messages from BMS master.
*     Only uses FIFO0. If more messages need to be read change FilterMaskIdHigh
*     and FilterMaskIdLow.
*
***************************************************************************/
void can_filter_init(CAN_HandleTypeDef* hcan) {
  CAN_FilterTypeDef FilterConf;
  FilterConf.FilterIdHigh =         ID_BMS_MASTER << 5;
  FilterConf.FilterIdLow =          ID_BMS_MASTER_CONFIG << 5;
  FilterConf.FilterMaskIdHigh =     0;
  FilterConf.FilterMaskIdLow =      0;
  FilterConf.FilterFIFOAssignment = CAN_FilterFIFO0;
  FilterConf.FilterBank = 0;
  FilterConf.FilterMode = CAN_FILTERMODE_IDLIST;
  FilterConf.FilterScale = CAN_FILTERSCALE_16BIT;
  FilterConf.FilterActivation = ENABLE;
  HAL_CAN_ConfigFilter(hcan, &FilterConf);
}

/***************************************************************************
*
*     Function Information
*
*     Name of Function: can_filter_init
*
*     Programmer's Name: Matt Flanagan
*
*     Function Return Type: None
*
*     Parameters (list data type, name, and comment one per line):
*       1. CAN_HandleTypeDef* hcan        Can Handle
*
*      Global Dependents:
*       1. None
*
*     Function Description: Send an acknowledgment to main
*
***************************************************************************/
void send_ack() {
	CanTxMsgTypeDef msg;
	msg.IDE = CAN_ID_STD;
	msg.RTR = CAN_RTR_DATA;
	msg.DLC = 1;
	msg.StdId = ID_BMS_MASTER;
	msg.Data[0] = ID_SLAVE;

	xQueueSendToBack(bms.q_tx_can, &msg, 100);
}

