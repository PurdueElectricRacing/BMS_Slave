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

Success_t process_slave_param_set(CanRxMsgTypeDef* rx_can);
Success_t send_volt_msg();
Success_t send_temp_msg();
Success_t send_generic_msg(uint16_t items, can_broadcast_t msg_type);

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
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan) {
  CanRxMsgTypeDef rx;
  CAN_RxHeaderTypeDef header;
  HAL_CAN_GetRxMessage(hcan, 0, &header, rx.Data);
  rx.DLC = header.DLC;
  rx.StdId = header.StdId;
  xQueueSendFromISR(bms.q_rx_can, &rx, 0);
  
  //master watchdawg task
  if (xSemaphoreTakeFromISR(wdawg.master_sem, NULL) == pdPASS) {
    //semaphore successfully taken
    wdawg.new_msg = xTaskGetTickCountFromISR();
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
  TickType_t time_init = 0;
  
  while (1) {
    time_init = xTaskGetTickCount();
    if (xSemaphoreTake(wdawg.master_sem, WDAWG_BLOCK) == pdPASS) {
      //semaphore successfully taken
      if ((xTaskGetTickCount() - wdawg.new_msg) > WDAWG_RATE) {
        //master is not responding go into shutdown
        bms.connected = 0;
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
  while (1) {
    time_init = xTaskGetTickCount();
    //check if this task is triggered
    if (xQueuePeek(bms.q_tx_can, &tx, TIMEOUT) == pdTRUE) {
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
*     Name of Function: task_CanProcess
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
*     Function Description: Processes all of the new messages that have been
*     received via can.
***************************************************************************/
void task_CanProcess() {
  CanRxMsgTypeDef rx_can;
  TickType_t time_init = 0;
  while (1) {
    time_init = xTaskGetTickCount();
    
    if (xQueuePeek(bms.q_rx_can, &rx_can, TIMEOUT) == pdTRUE) {
      xQueueReceive(bms.q_rx_can, &rx_can, TIMEOUT);
      
      switch (rx_can.StdId) {
        case ID_MAS_POW_CMD:
          //check if you need to go to sleep or wake up
          if (rx_can.Data[0] == POWER_ON && bms.state != ERROR_BMS) {
            //wakeup message send ack
            send_ack();
            bms.connected = NORMAL;
            if (xSemaphoreTake(bms.state_sem, TIMEOUT) == pdPASS) {
							bms.state = LOW_POWER;
							xSemaphoreGive(bms.state_sem); //release sem
						}
          } else if (rx_can.Data[0] == POWER_OFF) {
            //shutdown message was received send ack and shutdown
            send_ack();
            bms.connected = FAULTED;
            if (xSemaphoreTake(bms.state_sem, TIMEOUT) == pdPASS) {
              bms.state = SHUTDOWN;
              xSemaphoreGive(bms.state_sem); //release sem
            }
          }
          break;
        case ID_MAS_PASSIVE:
          //see if this pertains to you and then toggle passive balancing if so
          if (rx_can.Data[0] == ID_SLAVE && bms.state == NORMAL_OP) {
            bms.passive_en = !bms.passive_en;
            if (bms.passive_en == 0) {
              //todo: shutdown_passive();
            } else {
              //todo: enable_passive();
            }
          }
          break;
        case ID_MAS_WDAWG:
        	send_ack();
        	break;
        case ID_MAS_CONFIG:
        	process_slave_param_set(&rx_can);
        	break;
      }
    }
    
    vTaskDelayUntil(&time_init, CAN_RX_RATE);
  }
}

/***************************************************************************
*
*     Function Information
*
*     Name of Function: task_broadcast
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
*     Function Description: brodcasts msg's at the defined rate and if they are
*     enabled
***************************************************************************/
void task_broadcast() {
  TickType_t time_init = 0;
  uint16_t i = 0;
  while (1) {
    time_init = xTaskGetTickCount();
    if (bms.state == NORMAL_OP) {
    	if (bms.param.volt_msg_en == ASSERTED) {
				if (execute_broadcast(bms.param.volt_msg_rate, i)) {
					send_volt_msg();
				}
			}
			if (bms.param.temp_msg_en == ASSERTED) {
				if (execute_broadcast(bms.param.temp_msg_rate, i)) {
					send_temp_msg();
				}
			}

			i++;
    }
    vTaskDelayUntil(&time_init, BROADCAST_RATE);
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
  FilterConf.FilterIdHigh =         ID_MAS_POW_CMD << 5;
  FilterConf.FilterIdLow =          ID_MAS_CONFIG << 5;
  FilterConf.FilterMaskIdHigh =     ID_MAS_PASSIVE << 5;
  FilterConf.FilterMaskIdLow =      ID_MAS_WDAWG << 5;
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
  msg.StdId = ID_SLAVE_ACK;
  msg.Data[0] = ID_SLAVE;
  
  xQueueSendToBack(bms.q_tx_can, &msg, 100);
}

/***************************************************************************
*
*     Function Information
*
*     Name of Function: process_gui_param_set
*
*     Programmer's Name: Matt Flanagan
*
*     Function Return Type: Success_t
*
*     Parameters (list data type, name, and comment one per line):
*       1. CanRxMsgTypeDef* rx_can
*
*      Global Dependents:
*       1. bms.params
*
*     Function Description: processes a gui param config msg and sets the appropriate variable
*
***************************************************************************/
Success_t process_slave_param_set(CanRxMsgTypeDef* rx_can) {
  Success_t status = SUCCESSFUL;
  int16_t temp = 0;
  uint16_t volt = 0;
  if (xSemaphoreTake(bms.param.sem, TIMEOUT) == pdTRUE) {
  	bms.param.volt_msg_en = bit_extract(CONFIG_VOLT_MSG_MASK, CONFIG_VOLT_MSG_SHIFT, rx_can->Data[0]);
  	bms.param.temp_msg_en = bit_extract(CONFIG_TEMP_MSG_MASK, CONFIG_TEMP_MSG_SHIFT, rx_can->Data[0]);

  	volt = byte_combine(rx_can->Data[1], rx_can->Data[2]);
  	temp = byte_combine(rx_can->Data[3], rx_can->Data[4]);

  	if (volt > VOLT_MSG_RATE) {
  		bms.param.volt_msg_rate = volt;
  	}

  	if (temp > TEMP_MSG_RATE) {
  		bms.param.temp_msg_rate = temp;
  	}

    xSemaphoreGive(bms.param.sem);
  } else {
    status = FAILURE;
  }

  return status;
}

Success_t send_volt_msg() {
  Success_t status = SUCCESSFUL;
  status = send_generic_msg(NUM_VTAPS, VOLT_MSG);
  return status;
}

Success_t send_temp_msg() {
  Success_t status = SUCCESSFUL;
  status = send_generic_msg(NUM_TEMP, TEMP_MSG);
  return status;
}

Success_t send_generic_msg(uint16_t items, can_broadcast_t msg_type) {
  Success_t status = SUCCESSFUL;
  uint8_t i = 0;
  uint8_t x = 0;
  CanTxMsgTypeDef msg;
  msg.IDE = CAN_ID_STD;
  msg.RTR = CAN_RTR_DATA;

  switch (msg_type) {
    case VOLT_MSG:
			for (x = 0; x < NUM_VTAPS; x = x + VALUES_PER_MSG) {
				msg.DLC = GENERIC_MSG_LENGTH;
				msg.StdId = ID_SLAVE_VOLT_MSG;
				msg.Data[0] = i;  //slave id
				msg.Data[1] = x / VALUES_PER_MSG; //row
				msg.Data[2] = extract_MSB(bms.vtap.data[x]);
				msg.Data[3] = extract_LSB(bms.vtap.data[x]);
				if (x + 1 < NUM_VTAPS) {
					msg.Data[4] = extract_MSB(bms.vtap.data[x + 1]);
					msg.Data[5] = extract_LSB(bms.vtap.data[x + 1]);
				} else {
					msg.Data[4] = 0;
					msg.Data[5] = 0;
				}
				if (x + 2 < NUM_VTAPS) {
					msg.Data[6] = extract_MSB(bms.vtap.data[x + 2]);
					msg.Data[7] = extract_LSB(bms.vtap.data[x + 2]);
				} else {
					msg.Data[6] = 0;
					msg.Data[7] = 0;
				}

				if (xQueueSendToBack(bms.q_tx_can, &msg, 100) != pdPASS) {
					status = FAILURE;
				}
			}
      break;
    case TEMP_MSG:
			for (x = 0; x < NUM_TEMP; x = x + VALUES_PER_MSG) {
				msg.DLC = MACRO_MSG_LENGTH;
				msg.StdId = ID_SLAVE_TEMP_MSG;
				msg.Data[0] = i;  //slave id
				msg.Data[1] = x / VALUES_PER_MSG; //row
				msg.Data[2] = extract_MSB(bms.temp.data[x]);
				msg.Data[3] = extract_LSB(bms.temp.data[x]);
				if (x + 1 < NUM_TEMP) {
					msg.Data[4] = extract_MSB(bms.temp.data[x + 1]);
					msg.Data[5] = extract_LSB(bms.temp.data[x + 1]);
				} else {
					msg.Data[4] = 0;
					msg.Data[5] = 0;
				}
				if (x + 2 < NUM_TEMP) {
					msg.Data[6] = extract_MSB(bms.temp.data[x + 2]);
					msg.Data[7] = extract_LSB(bms.temp.data[x + 2]);
				} else {
					msg.Data[6] = 0;
					msg.Data[7] = 0;
				}

				if (xQueueSendToBack(bms.q_tx_can, &msg, 100) != pdPASS) {
					status = FAILURE;
				}
			}
      break;
  }

  return status;
}
