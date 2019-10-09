/*
 * ltc6811.h
 *
 *  Created on: Feb 10, 2019
 *      Author: Matt Flanagan
 */

#ifndef VSTACK_H_
#define VSTACK_H_

#include "bms.h"
#include "stm32l4xx_hal_spi.h"

//Essentials
#define GREAT				1
#define PER					GREAT

//SPI
#define LTC6811_SPI     	&hspi1
#define LTC6811_ADDR_ONE	0x1
#define LTC6811_ADDR_TWO	0x2

//Voltage flags
#define LTC6811_MAX_PCKV 	50.4 //Max Pack voltage cutoff
#define LTC6811_MIN_PCKV 	30   //Min pack voltage cutoff

//Task info
#define VSTACK_STACK_SIZE 	128
#define VSTACK_PRIORITY   	1
#define VSTACK_RATE       	20 / portTICK_RATE_MS

//ADCV
#define	NORMAL_MODE			0x2
#define REG_A				0x4
#define REG_B				0x6
#define REG_C				0x8
#define REG_D				0xA
#define REG_E				0x9
#define REG_F				0xB
#define SIX_TAU				(0.000864 * SystemCoreClock) //Six tau wait period for
#define CNV_TIME			(0.002335 * SystemCoreClock)

enum {
	ALL_CELLS,
	CELLS_1_7,
	CELLS_2_8,
	CELLS_3_9,
	CELLS_4_10,
	CELLS_5_11,
	CELLS_6_12
};

enum {
	DISCHARGE_NOT_PERMITTED,
	DISCHARGE_PERMITTED
};

//DWT -> Visit http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0337e/ch11s05s01.html for better understanding
//Note: TRCENA *MUST* be enabled in DEMCR for the DWT to be active outside debug mode! -> http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0337h/CACGCAIB.html
//Note: These values are redefined from the structure DWT just to simplify things a little bit
#define DWT_CONTROL			(*((volatile uint32_t*)0xE0001000)) //Used to assign values to data watchpoint and trace control registers
#define DWT_CYCCNT			(*((volatile uint32_t*)0xE0001004)) //Used to gather DWT cycle count
#define DEMCR				(*((volatile uint32_t*)0xE000EDFC)) //Used to toggle DWT outside debug mode
#define DWT_CYCCNTENA		(1UL << 0) //Enables cycle counting
#define DWT_CYCCNTDIS		(0UL << 0) //Disables cycle counting
#define DWT_TRCENA			(1UL << 24) //Enables DWT outside debug mode

//If WAIT_MICROS is undefined, generic, millisecond wait times will be used after ADCV
//#define WAIT_MICROS

//If FULL_DIAG is undefined, the BMIC diagnostics will not run. This means we will only recieve voltages regardless of BMIC health
//#define FULL_DIAG

//If DUAL_IC is defined, the program will attempt to use address communication and will initialize two sets of vtaps
//PLEASE NOTE!! DUAL_IC NEEDS TO BE DEFINED IN bms.h TO GET A VALID INITIALIZATION OF THE VTAPS
#define DUAL_IC

static const uint16_t crc15Table[256] = { 0x0, 0xc599, 0xceab, 0xb32, 0xd8cf,
    0x1d56, 0x1664, 0xd3fd, 0xf407, 0x319e,
    0x3aac,  //!<precomputed CRC15 Table
    0xff35, 0x2cc8, 0xe951, 0xe263, 0x27fa, 0xad97, 0x680e, 0x633c, 0xa6a5,
    0x7558, 0xb0c1, 0xbbf3, 0x7e6a, 0x5990, 0x9c09, 0x973b, 0x52a2, 0x815f,
    0x44c6, 0x4ff4, 0x8a6d, 0x5b2e, 0x9eb7, 0x9585, 0x501c, 0x83e1, 0x4678,
    0x4d4a, 0x88d3, 0xaf29, 0x6ab0, 0x6182, 0xa41b, 0x77e6, 0xb27f, 0xb94d,
    0x7cd4, 0xf6b9, 0x3320, 0x3812, 0xfd8b, 0x2e76, 0xebef, 0xe0dd, 0x2544,
    0x2be, 0xc727, 0xcc15, 0x98c, 0xda71, 0x1fe8, 0x14da, 0xd143, 0xf3c5,
    0x365c, 0x3d6e, 0xf8f7, 0x2b0a, 0xee93, 0xe5a1, 0x2038, 0x7c2, 0xc25b,
    0xc969, 0xcf0, 0xdf0d, 0x1a94, 0x11a6, 0xd43f, 0x5e52, 0x9bcb, 0x90f9,
    0x5560, 0x869d, 0x4304, 0x4836, 0x8daf, 0xaa55, 0x6fcc, 0x64fe, 0xa167,
    0x729a, 0xb703, 0xbc31, 0x79a8, 0xa8eb, 0x6d72, 0x6640, 0xa3d9, 0x7024,
    0xb5bd, 0xbe8f, 0x7b16, 0x5cec, 0x9975, 0x9247, 0x57de, 0x8423, 0x41ba,
    0x4a88, 0x8f11, 0x57c, 0xc0e5, 0xcbd7, 0xe4e, 0xddb3, 0x182a, 0x1318,
    0xd681, 0xf17b, 0x34e2, 0x3fd0, 0xfa49, 0x29b4, 0xec2d, 0xe71f, 0x2286,
    0xa213, 0x678a, 0x6cb8, 0xa921, 0x7adc, 0xbf45, 0xb477, 0x71ee, 0x5614,
    0x938d, 0x98bf, 0x5d26, 0x8edb, 0x4b42, 0x4070, 0x85e9, 0xf84, 0xca1d,
    0xc12f, 0x4b6, 0xd74b, 0x12d2, 0x19e0, 0xdc79, 0xfb83, 0x3e1a, 0x3528,
    0xf0b1, 0x234c, 0xe6d5, 0xede7, 0x287e, 0xf93d, 0x3ca4, 0x3796, 0xf20f,
    0x21f2, 0xe46b, 0xef59, 0x2ac0, 0xd3a, 0xc8a3, 0xc391, 0x608, 0xd5f5,
    0x106c, 0x1b5e, 0xdec7, 0x54aa, 0x9133, 0x9a01, 0x5f98, 0x8c65, 0x49fc,
    0x42ce, 0x8757, 0xa0ad, 0x6534, 0x6e06, 0xab9f, 0x7862, 0xbdfb, 0xb6c9,
    0x7350, 0x51d6, 0x944f, 0x9f7d, 0x5ae4, 0x8919, 0x4c80, 0x47b2, 0x822b,
    0xa5d1, 0x6048, 0x6b7a, 0xaee3, 0x7d1e, 0xb887, 0xb3b5, 0x762c, 0xfc41,
    0x39d8, 0x32ea, 0xf773, 0x248e, 0xe117, 0xea25, 0x2fbc, 0x846, 0xcddf,
    0xc6ed, 0x374, 0xd089, 0x1510, 0x1e22, 0xdbbb, 0xaf8, 0xcf61, 0xc453, 0x1ca,
    0xd237, 0x17ae, 0x1c9c, 0xd905, 0xfeff, 0x3b66, 0x3054, 0xf5cd, 0x2630,
    0xe3a9, 0xe89b, 0x2d02, 0xa76f, 0x62f6, 0x69c4, 0xac5d, 0x7fa0, 0xba39,
    0xb10b, 0x7492, 0x5368, 0x96f1, 0x9dc3, 0x585a, 0x8ba7, 0x4e3e, 0x450c,
    0x8095 };

static const uint8_t readCmd[6] = {REG_A, REG_B, REG_C, REG_D, REG_E, REG_F};

#ifdef DUAL_IC
static const uint8_t spiAddr[2] = {LTC6811_ADDR_ONE, LTC6811_ADDR_TWO}; //Yes... I know... I should just define them here. I will in the future, I just think this is easier to read.
#endif

void task_VSTACK();
HAL_StatusTypeDef initLTC();
HAL_StatusTypeDef ADCV(uint8_t MD, uint8_t DCP, uint8_t CH);
#ifdef DUAL_IC
HAL_StatusTypeDef RDCVX(uint8_t cellGroup, uint8_t * dataIn, uint8_t addr);
#else
HAL_StatusTypeDef RDCVX(uint8_t cellGroup, uint8_t * dataIn);
#endif
HAL_StatusTypeDef CLRCELL();
HAL_StatusTypeDef RDSTATB(uint8_t * dataIn);
int ADOW(uint8_t MD, uint8_t DCP);
void wakeSPI();
uint16_t pec(uint8_t * data, uint8_t len);
HAL_StatusTypeDef sendSPI(uint8_t * data, int len);
HAL_StatusTypeDef recieveSPI(uint8_t * cmd, int cmdLen, uint8_t * data, int dataLen);
void handleHALError();
void waitMicros(uint32_t cycles);

#endif /* VSTACK_H_ */
