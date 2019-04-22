/*
 * ltc6811.h
 *
 *  Created on: Feb 10, 2019
 *      Author: Matt Flanagan
 */

#ifndef VSTACK_H_
#define VSTACK_H_

#include "bms.h"

#define VSTACK_STACK_SIZE 128
#define VSTACK_PRIORITY   1
#define VSTACK_RATE       20 / portTICK_RATE_MS

//SPI Definitions
#define LTC6811_SPI_ADDR	0x1	//Device Address
#define LTC6811_SPI			&hspi1

//ADC Channel definition
#define LTC6811_ADC_CALL	0x0
#define LTC6811_ADC_C17		0x1
#define LTC6811_ADC_C28		0x2
#define LTC6811_ADC_C39		0x3
#define LTC6811_ADC_C410	0x4
#define LTC6811_ADC_C511	0x5
#define LTC6811_ADC_C612	0x6

//Status Group Selection Definitions
#define LTC6811_STATUS_ALL 	0x0
#define LTC6811_STATUS_SOC	0x1
#define LTC6811_STATUS_ITMP	0x2
#define LTC6811_STATUS_VA	0x3
#define LTC6811_STATUS_VD	0x4
#define LTC6811_STATUS	LTC6811_STATUS_ALL


#define LTC6811_ADC_CH LTC6811_ADC_CALL
#define LTC6811_ADC_MODE 0x3
#define LTC6811_DCP			0x1
#define LTC6811_PUP			0x0	//Look up PUP

//CMD Table
#define LTC6811_CMD_WRCFGA	0x1
#define LTC6811_CMD_WRCFGB	0x24
#define LTC6811_CMD_RDCFGA	0x2
#define LTC6811_CMD_RDCFGB	0x26
#define LTC6811_CMD_RDCVA	0x4
#define LTC6811_CMD_RDCVB	0x6
#define LTC6811_CMD_RDCVC	0x8
#define LTC6811_CMD_RDCVD	0xA
#define LTC6811_CMD_RDCVE	0x9
#define LTC6811_CMD_RDCVF	0xB
#define LTC6811_CMD_RDAUXA	0xC
#define LTC6811_CMD_RDAUXB	0xE
#define LTC6811_CMD_RDAUXC	0xD
#define LTC6811_CMD_RDAUXD	0xF
#define LTC6811_CMD_RDSTATA	0x10
#define LTC6811_CMD_RDSTATB 0x12
#define LTC6811_CMD_WRSCTRL 	0x14
#define LTC6811_CMD_WRPWM		0x20
#define LTC6811_CMD_WRPSB		0x1C
#define LTC6811_CMD_RDSCTRL		0x16
#define LTC6811_CMD_RDPWM		0x21
#define LTC6811_CMD_RDPSB		0x1E
#define LTC6811_CMD_STSCTRL		0x19
#define LTC6811_CMD_CLRSCTRL	0x1C
#define LTC6811_CMD_ADCV		0x260
#define LTC6811_CMD_ADOW
#define LTC6811_CMD_CVST
#define LTC6811_CMD_ADOL
#define LTC6811_CMD_ADAX
#define LTC6811_CMD_ADAXD
#define LTC6811_CMD_AXST
#define LTC6811_CMD_ADSTAT
#define LTC6811_CMD_ADSTATD
#define LTC6811_CMD_STATST
#define LTC6811_CMD_ADCVAX
#define LTC6811_CMD_ADCVSC
#define LTC6811_CMD_CLRCELL
#define LTC6811_CMD_CLRAUX
#define LTC6811_CMD_CLRSTAT
#define LTC6811_CMD_PLADC
#define LTC6811_CMD_DIAGN
#define LTC6811_CMD_WRCOMM
#define LTC6811_CMD_RDCOMM
#define LTC6811_CMD_STCOMM

#define GPIOx

#define LTC6811_MAX_PCKV 50.4 //Max Pack voltage cutoff
#define LTC6811_MIN_PCKV 30 	//Min pack voltage cutoff


#define CRC15_POLY 0x4599

void task_VSTACK();
HAL_StatusTypeDef LTC6811_addrWrite(uint8_t * din,
		uint8_t len, uint16_t cmd);

#endif /* VSTACK_H_ */
