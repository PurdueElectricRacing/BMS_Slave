/*
 * can.h
 *
 *  Created on: Feb 1, 2019
 *      Author: Matt Flanagan
 */

#ifndef CAN_H_
#define CAN_H_
//Includes
#include "bms.h"
#include "stm32l4xx_hal_can.h"

//Constants
#define SLAVE_ONE 1
#define SLAVE_TWO 0

//used to set the internal address of can messages so Master knows who is sending
#ifdef SLAVE_ONE
#define ID_SLAVE  0x1
#elif SLAVE_TWO
#define ID_SLAVE  0x2
#endif
//IDs
//Master -> Slave
#define ID_MAS_POW_CMD        0x600
#define ID_MAS_CONFIG  				0x601
#define ID_MAS_PASSIVE   			0x603
#define ID_MAS_WDAWG					0x604

//Slave -> Master
#define ID_SLAVE_ACK					0x640
#define ID_SLAVE_FAULT_CODE   0x641
#define ID_SLAVE_VOLT_MSG			0x642
#define ID_SLAVE_TEMP_MSG			0x643


//rates
#define CAN_TX_RATE 50 / portTICK_RATE_MS //send at 20Hz
#define CAN_RX_RATE 50 / portTICK_RATE_MS //send at 20Hz
#define WDAWG_RATE  5000 / portTICK_RATE_MS //5 second timeout value
#define WDAWG_BLOCK 10 / portTICK_RATE_MS

//TX RTOS
#define CAN_TX_STACK_SIZE   128
#define CAN_TX_Q_SIZE       8
#define CAN_TX_PRIORITY     1

//RX Process RTOS
#define CAN_RX_STACK_SIZE   128
#define CAN_RX_Q_SIZE       8
#define CAN_RX_PRIORITY     1

//WDawg RTOS
#define WDAWG_STACK_SIZE    128
#define WDAWG_PRIORITY      1


//structures
typedef struct {
  uint32_t StdId;    /*!< Specifies the standard identifier.
                          This parameter must be a number between Min_Data = 0 and Max_Data = 0x7FF */

  uint32_t ExtId;    /*!< Specifies the extended identifier.
                          This parameter must be a number between Min_Data = 0 and Max_Data = 0x1FFFFFFF */

  uint32_t IDE;      /*!< Specifies the type of identifier for the message that will be transmitted.
                          This parameter can be a value of @ref CAN_Identifier_Type */

  uint32_t RTR;      /*!< Specifies the type of frame for the message that will be transmitted.
                          This parameter can be a value of @ref CAN_remote_transmission_request */

  uint32_t DLC;      /*!< Specifies the length of the frame that will be transmitted.
                          This parameter must be a number between Min_Data = 0 and Max_Data = 8 */

  uint8_t Data[8];   /*!< Contains the data to be transmitted.
                          This parameter must be a number between Min_Data = 0 and Max_Data = 0xFF */

} CanTxMsgTypeDef;

typedef struct {
  uint32_t StdId;       /*!< Specifies the standard identifier.
                             This parameter must be a number between Min_Data = 0 and Max_Data = 0x7FF */

  uint32_t ExtId;       /*!< Specifies the extended identifier.
                             This parameter must be a number between Min_Data = 0 and Max_Data = 0x1FFFFFFF */

  uint32_t IDE;         /*!< Specifies the type of identifier for the message that will be received.
                             This parameter can be a value of @ref CAN_Identifier_Type */

  uint32_t RTR;         /*!< Specifies the type of frame for the received message.
                             This parameter can be a value of @ref CAN_remote_transmission_request */

  uint32_t DLC;         /*!< Specifies the length of the frame that will be received.
                             This parameter must be a number between Min_Data = 0 and Max_Data = 8 */

  uint8_t Data[8];      /*!< Contains the data to be received.
                             This parameter must be a number between Min_Data = 0 and Max_Data = 0xFF */

  uint32_t FMI;         /*!< Specifies the index of the filter the message stored in the mailbox passes through.
                             This parameter must be a number between Min_Data = 0 and Max_Data = 0xFF */

  uint32_t FIFONumber;  /*!< Specifies the receive FIFO number.
                             This parameter can be CAN_FIFO0 or CAN_FIFO1 */

} CanRxMsgTypeDef;

typedef struct {
  TickType_t last_msg;
  TickType_t new_msg;
  SemaphoreHandle_t master_sem;
} WatchDawg_t;

//Global Variables
volatile WatchDawg_t wdawg;

//Functions
void can_filter_init();
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan);
void task_txCan();
void task_Master_WDawg();
void task_CanProcess();
void send_ack();


#endif /* CAN_H_ */
