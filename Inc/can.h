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
#define SLAVE_ONE 0
#define SLAVE_TWO 1

//used to set the internal address of can messages so Master knows who is sending
#ifdef SLAVE_ONE
#define ID_SLAVE  0x0
#elif SLAVE_TWO
#define ID_SLAVE  0x1
#endif
//IDs
//Master -> Slave
#define ID_MAS_POW_CMD        0x600
#define ID_MAS_CONFIG         0x601
#define ID_MAS_PASSIVE        0x603
#define ID_MAS_WDAWG          0x604

//Slave -> Master
#define ID_SLAVE_ACK          0x640
#define ID_SLAVE_FAULT_CODE   0x641
#define ID_SLAVE_VOLT_MSG     0x642
#define ID_SLAVE_TEMP_MSG     0x643

#define MACRO_MSG_LENGTH          7
#define GENERIC_MSG_LENGTH        8
#define VALUES_PER_MSG            3
#define PARAM_RES_MSG_LENGTH      3
#define ERROR_MSG_LENGTH          2

//Masks
#define FAULT_VOLT_MASK       0x01
#define FAULT_TEMP1_MASK      0x02
#define FAULT_TEMP2_MASK      0x04
#define FAULT_BMSCON_MASK     0x08

#define FAULT_VOLT_SHIFT      0
#define FAULT_TEMP1_SHIFT     1
#define FAULT_TEMP2_SHIFT     2
#define FAULT_BMSCON_SHIFT    3

//rates
#define CAN_TX_RATE 5 / portTICK_RATE_MS //send at 20Hz
#define CAN_RX_RATE 20 / portTICK_RATE_MS //send at 20Hz
#define WDAWG_RATE  5000 / portTICK_RATE_MS //5 second timeout value
#define WDAWG_BLOCK 10 / portTICK_RATE_MS
#define BROADCAST_MS    25
#define BROADCAST_RATE  BROADCAST_MS / portTICK_RATE_MS //fastest broadcast is 20hz
//used to keep canrxq from overflowing
#define BROADCAST_DELAY (BROADCAST_MS / 10) / portTICK_RATE_MS

//TX RTOS
#define CAN_TX_STACK_SIZE   128
#define CAN_TX_Q_SIZE       20
#define CAN_TX_PRIORITY     1

//RX Process RTOS
#define CAN_RX_STACK_SIZE   128
#define CAN_RX_Q_SIZE       10
#define CAN_RX_PRIORITY     1

//Broadcast Process RTOS
#define BROAD_STACK_SIZE   128
#define BROAD_PRIORITY     1

//WDawg RTOS
#define WDAWG_STACK_SIZE    128
#define WDAWG_PRIORITY      1

//Masks
#define CONFIG_VOLT_MSG_MASK      0x01
#define CONFIG_TEMP_MSG_MASK      0x02
#define CONFIG_VOLT_MSG_SHIFT     0
#define CONFIG_TEMP_MSG_SHIFT     1

//macros
#define bit_extract(mask, shift, byte) (byte & mask) >> shift
#define byte_combine(msb, lsb) ((msb << 8) | lsb)

//enums
enum params_enum {
  VOLT_MSG_EN   = 0,
  TEMP_MSG_EN   = 1,
  VOLT_MSG_RATE = 2,
  TEMP_MSG_RATE = 3
};

enum defaults_enum {
  DEFAULT = 0,
  CHANGE = 1
};

typedef enum dcan_broadcast {
  VOLT_MSG = 0,
  TEMP_MSG = 1
} can_broadcast_t;

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
void task_broadcast();

#endif /* CAN_H_ */
