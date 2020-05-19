/*
 * vstack.h
 *
 *  Created on: Feb 10, 2019
 *      Author: Matt Flanagan
 */

#ifndef VSTACK_H_
#define VSTACK_H_

#include "bms.h"
#include "stm32l4xx_hal_spi.h"
#include "devices/LTC6811.h"

//Voltage flags
#define LTC6811_MAX_PCKV 	50.4 //Max Pack voltage cutoff
#define LTC6811_MIN_PCKV 	30   //Min pack voltage cutoff

//Task info
#define VSTACK_STACK_SIZE 	128
#define VSTACK_PRIORITY   	1
#define VSTACK_RATE       	(20 / portTICK_RATE_MS)

//ADCV
#define SIX_TAU				(0.000864 * SystemCoreClock) //Six tau wait period for
#define CNV_TIME			(0.002335 * SystemCoreClock)

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

void task_VSTACK();

#endif /* VSTACK_H_ */
