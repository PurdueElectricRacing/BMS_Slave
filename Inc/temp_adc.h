/*
 * temp_adc.h
 *
 *  Created on: Feb 10, 2019
 *      Author: Matt Flanagan
 */

#ifndef TEMP_ADC_H_
#define TEMP_ADC_H_

#include "bms.h"

#define NUM_CHANNELS 16

#define ID_TEMP_1   0x76
#define ID_TEMP_2   0x7e
#define TRIALS      2
#define NUM_CHANNELS 16
#define ACQUIRE_TEMP_STACK_SIZE 128
#define ACQUIRE_TEMP_PRIORITY   1
#define I2C_TIMEOUT             100 / portTICK_RATE_MS
#define READ_REQ_WAIT						150 / portTICK_RATE_MS


//writing
#define WRITE_MSG_SIZE    3
#define WRITE_ENABLE      0x00 //xxxx-xxx0
#define CHANGE_CHANNEL    0xA0 //101x-xxxx

#define SAME_CHANNEL      0x00 //000x-xxxx

#define SGL_SHIFT         4
#define SGL_MASK          0x10  //xxx1-xxxx
#define SGL_DIFF          0
#define SGL_SINGLE        1

#define CHANNEL_0         0x00 //xxxx-0000
#define CHANNEL_1         0x08 //xxxx-1000
#define CHANNEL_2         0x01 //xxxx-0001
#define CHANNEL_3         0x09 //xxxx-1001
#define CHANNEL_4         0x02 //xxxx-0010
#define CHANNEL_5         0x0A //xxxx-1010
#define CHANNEL_6         0x03 //xxxx-0011
#define CHANNEL_7         0x0B //xxxx-1011

#define CHANNEL_8         0x04 //xxxx-0100
#define CHANNEL_9         0x0C //xxxx-1100
#define CHANNEL_10        0x05 //xxxx-0101
#define CHANNEL_11        0x0D //xxxx-1101
#define CHANNEL_12        0x06 //xxxx-0110
#define CHANNEL_13        0x0E //xxxx-1110
#define CHANNEL_14        0x07 //xxxx-0111
#define CHANNEL_15        0x0F //xxxx-1111

//reading
#define READ_ENABLE       0x01 //xxxx-xxx1
#define READ_MSG_SIZE     3    //24 bits sent per msg

//read each set of temps at 2xRate
#define ACQUIRE_TEMP_RATE       500 / portTICK_RATE_MS

//macros
#define channel_combine(channel) CHANGE_CHANNEL | SGL_MASK | channel
#define set_address(address, write_en) (address << 1) | write_en

void task_acquire_temp();

#endif /* TEMP_ADC_H_ */
