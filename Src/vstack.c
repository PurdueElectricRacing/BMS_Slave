///*
// * ltc6811.c
// *
// *  Created on: Feb 10, 2019
// *      Author: Matt Flanagan
// */
//
//#include "vstack.h"
//#include "stm32l4xx_hal_spi.h"
//#ifdef LTC_DEBUG
////Device Address
//#define LTC6811_DEV_ADDR
//
////ADC Channel definition
//#define LTC6811_ADC_CALL	0x0
//#define LTC6811_ADC_C17		0x1
//#define LTC6811_ADC_C28		0x2
//#define LTC6811_ADC_C39		0x3
//#define LTC6811_ADC_C410	0x4
//#define LTC6811_ADC_C511	0x5
//#define LTC6811_ADC_C612	0x6
//
////Status Group Selection Definitions
//#define LTC6811_STATUS_ALL 	0x0
//#define LTC6811_STATUS_SOC	0x1
//#define LTC6811_STATUS_ITMP	0x2
//#define LTC6811_STATUS_VA	0x3
//#define LTC6811_STATUS_VD	0x4
//#define LTC6811_STATUS	LTC6811_STATUS_ALL
//
//
//#define LTC6811_ADC_CH LTC6811_ADC_CALL
//#define LTC6811_ADC_MODE 0x3
//#define LTC6811_DCP			0x1
//#define LTC6811_PUP			0x0	//Look up PUP
//
////CMD Table
//#define LTC6811_CMD_WRCFGA	0x1
//#define LTC6811_CMD_WRCFGB	0x24
//#define LTC6811_CMD_RDCFGA	0x2
//#define LTC6811_CMD_RDCFGB	0x26
//#define LTC6811_CMD_RDCVA	0x4
//#define LTC6811_CMD_RDCVB	0x6
//#define LTC6811_CMD_RDCVC	0x8
//#define LTC6811_CMD_RDCVD	0xA
//#define LTC6811_CMD_RDCVE	0x9
//#define LTC6811_CMD_RDCVF	0xB
//#define LTC6811_CMD_RDAUXA	0xC
//#define LTC6811_CMD_RDAUXB	0xE
//#define LTC6811_CMD_RDAUXC	0xD
//#define LTC6811_CMD_RDAUXD	0xF
//#define LTC6811_CMD_RDSTATA	0x10
//#define LTC6811_CMD_RDSTATB 0x12
//#define LTC6811_CMD_WRSCTRL 	0x14
//#define LTC6811_CMD_WRPWM		0x20
//#define LTC6811_CMD_WRPSB		0x1C
//#define LTC6811_CMD_RDSCTRL		0x16
//#define LTC6811_CMD_RDPWM		0x21
//#define LTC6811_CMD_RDPSB		0x1E
//#define LTC6811_CMD_STSCTRL		0x19
//#define LTC6811_CMD_CLRSCTRL	0x1C
//#define LTC6811_CMD_ADCV		0x
//#define LTC6811_CMD_ADOW
//#define LTC6811_CMD_CVST
//#define LTC6811_CMD_ADOL
//#define LTC6811_CMD_ADAX
//#define LTC6811_CMD_ADAXD
//#define LTC6811_CMD_AXST
//#define LTC6811_CMD_ADSTAT
//#define LTC6811_CMD_ADSTATD
//#define LTC6811_CMD_STATST
//#define LTC6811_CMD_ADCVAX
//#define LTC6811_CMD_ADCVSC
//#define LTC6811_CMD_CLRCELL
//#define LTC6811_CMD_CLRAUX
//#define LTC6811_CMD_CLRSTAT
//#define LTC6811_CMD_PLADC
//#define LTC6811_CMD_DIAGN
//#define LTC6811_CMD_WRCOMM
//#define LTC6811_CMD_RDCOMM
//#define LTC6811_CMD_STCOMM
//
//
//
//
//
//void task_VSTACK() {
//  TickType_t time_init = 0;
//
//  while (1) {
//    time_init = xTaskGetTickCount();
//
//    vTaskDelayUntil(&time_init, VSTACK_RATE);
//  }
//}
//
//HAL_StatusTypeDef init_LTC6811() {
//  //this function is used to initiate communication to the LTC6811 chip
//
//  return HAL_OK;
//}
//
////Calculates PEC or CRC
////TODO might want to make LTCHandle_t a pointer
//uint16_t LTC6811Pec(LTCHandle_t ltc, uint8_t *din, uint8_t len) {
//	uint16_t new_pec = ltc.pec;
//
//	//CRC Algorithm
//
//}
//
//
//HAL_StatusTypeDef LTC6811_addrWrite(LTCHandle_t ltc, uint8_t *din,
//		uint8_t len, uint16_t cmd) {
//	uint8_t tx_arr[];
//	tx_arr = malloc((len + 6) *sizeof(*tx_arr));
//	if (NULL == tx_arr)
//		return HAL_ERROR;
//
//	//Generate CMD0 and CMD1 bits
//	tx_arr[0] = 0xFF & ltc.spi_addr & ((uint8_t) cmd >> 8);
//	tx_arr[1] = (uint8_t) cmd;
//
//	//Generate PEC
//	uint16_t pec = LTC6811Pec(ltc)
//	//Compile write array
//
//	//pull SS line low
//	//send data
//
//}
//
//HAL_StatusTypeDef LTC6811_addrRead(LTCHandle_t ltc, uint8_t *dout,
//		uint8_t len) {
//	//Generate CMD0 and CMD1 bits
//	//Generate PEC
//	//Compile write array
//	//pull SS line low
//	//HAL_SPI_TransmitReceive_IT
//}
//
//HAL_StatusTypeDef LTC6811_addrRead_IT(LTCHandle_t ltc)
//
//HAL_StatusTypeDef LTC6811_addrPoll(LTCHandle_t ltc, uint8_t *din,
//		uint8_t len) {
//	//Generate CMD0 and CMD1 bits
//	//Compile write array
//	//pull SS line low
//	//HAL_SPI_TransmitReceive
//}
//
//HAL_StatusTypeDef LTC6811_init(LTCHandle_t ltc, uint8_t *din,
//		uint8_t len) {
//
//}
//
//HAL_StatusTypeDef LTC6811_deInit(LTCHandle_t ltc, uint8_t *din,
//		uint8_t len) {
//
//}
//#endif
