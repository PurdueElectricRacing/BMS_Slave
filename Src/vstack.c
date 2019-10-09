///*
// * ltc6811.c
// *
// *  Created on: Feb 10, 2019
// *      Author: Matt Flanagan
// */
//
#include "vstack.h"

void task_VSTACK()
{
	TickType_t initTime = 0;
	uint8_t i, x;
	uint8_t diagCount = 0; //Note: This will be marked as unused if FULL_DIAG is undefined
	uint16_t pecRet;
	uint8_t data[8]; //6 bytes for the data and 2 for the pec
	DEMCR |= DWT_TRCENA; //Enables DWT for microsecond waits

	wakeSPI();
	if (initLTC())
	{
		//Do some error handling here
		//HAL returned an error!
	}
	CLRCELL();
	while (PER == GREAT)
	{
		ADCV(NORMAL_MODE, DISCHARGE_NOT_PERMITTED, CELLS_1_7);
#ifndef WAIT_MICROS
		vTaskDelay(1 / portTICK_PERIOD_MS);
#else
		waitMicros(SIX_TAU);
#endif
		ADCV(NORMAL_MODE, DISCHARGE_NOT_PERMITTED, ALL_CELLS);
#ifndef WAIT_MICROS
		vTaskDelay(3 / portTICK_PERIOD_MS);
#else
		waitMicros(CNV_TIME);
#endif
#ifdef DUAL_IC
int j;
	for (j = 0; j < 2; ++j)
	{
		for (i = 0; i <= (NUM_VTAPS / 3); i++)
		{
			RDCVX(readCmd[i], data, spiAddr[j]);
			pecRet = byte_combine(data[6], data[7]);
			if (pecRet == pec(data, 6))
			{
				if (xSemaphoreTake(bms.vtap[j].sem, TIMEOUT) == pdTRUE)
				{
					x = (i-1) * 3;
					bms.vtap[j].data[x++] = byte_combine(data[1], data[0]); // Remember, these voltages need to be * 0.0001!
					bms.vtap[j].data[x++] = byte_combine(data[3], data[2]);
					bms.vtap[j].data[x] = byte_combine(data[5], data[4]);
					xSemaphoreGive(bms.vtap[j].sem);
				}
			}
		}
	}

#else
	for (i = 0; i <= (NUM_VTAPS / 3); i++)
	{
		RDCVX(readCmd[i], data);
		pecRet = byte_combine(data[6], data[7]);
		if (pecRet == pec(data, 6))
		{
			if (xSemaphoreTake(bms.vtap.sem, TIMEOUT) == pdTRUE)
			{
				x = (i-1) * 3;
				bms.vtap.data[x++] = byte_combine(data[1], data[0]); // Remember, these voltages need to be * 0.0001!
				bms.vtap.data[x++] = byte_combine(data[3], data[2]);
				bms.vtap.data[x] = byte_combine(data[5], data[4]);
				xSemaphoreGive(bms.vtap.sem);
			}
		}
	}
#endif

#ifndef DUAL_IC //The diagnostics are not tested on one IC. Therefore, they have not yet been updated to comply with the new slave layout
#ifdef FULL_DIAG
		if (++diagCount == DIAGNOSTICS_RATE)
		{
			RDSTATB(data);
			if (data[2] != 0x00 || data[3] != 0x00 || data[4] != 0x00)
			{
				//TODO: add logic to figure out which cell faulted
				bms.vstack_con = FAULTED; //A cell has under/over-volted
			}
			if (data[5] & (1 << 1))
			{
				//TODO: Start the MUX check
				bms.vstack_con = FAULTED; //The MUX has faulted
			}
			diagCount = 0;
		}
#endif
#endif

		vTaskDelayUntil(&initTime, VSTACK_RATE);
	}
}

HAL_StatusTypeDef initLTC()
{
	//The WRCFGA command is 0x0001
	uint16_t packUndervolt = LTC6811_MIN_PCKV * (16 * 0.0001);
	uint16_t packOvervolt = LTC6811_MAX_PCKV * (16 * 0.0001);
	uint8_t cmd[12] = {0};
	cmd[1] = 0x01;
	uint16_t cmdPEC = pec(cmd, 2);
	cmd[2] = (uint8_t) (cmdPEC >> 8);
	cmd[3] = (uint8_t) cmdPEC;
	cmd[5] = (uint8_t) packUndervolt;
	cmd[6] = (0x0F & (uint8_t) (packUndervolt >> 8)) | ((uint8_t) (packOvervolt << 4));
	cmd[7] = (uint8_t) packOvervolt;
	uint16_t dataPEC = pec(&cmd[4], 6);
	cmd[10] = (uint8_t) (dataPEC >> 8);
	cmd[11] = (uint8_t) dataPEC;

	return sendSPI(cmd, 12);
}

HAL_StatusTypeDef ADCV(uint8_t MD, uint8_t DCP, uint8_t CH)
{
	uint8_t cmd[4] = {0};
	cmd[0] = 0x02 | (MD >> 1);
	cmd[1] = 0x60 | (MD << 7) | (DCP << 4) | CH;
	uint16_t cmdPEC = pec(cmd, 2);
	cmd[2] = (uint8_t) (cmdPEC >> 8);
	cmd[3] = (uint8_t) cmdPEC;

	return sendSPI(cmd, 4);
}

#ifdef DUAL_IC
HAL_StatusTypeDef RDCVX(uint8_t cellGroup, uint8_t * dataIn, uint8_t addr)
{
	uint8_t cmd[4] = {0};
	cmd[0] = 0x00 | (addr << 3);
	cmd[1] = 0x00 | cellGroup;
	uint16_t cmdPEC = pec(cmd, 2);
	cmd[2] = (uint8_t) (cmdPEC >> 8);
	cmd[3] = (uint8_t) cmdPEC;

	return recieveSPI(cmd, 4, dataIn, 8);
}
#else
HAL_StatusTypeDef RDCVX(uint8_t cellGroup, uint8_t * dataIn)
{
	uint8_t cmd[4] = {0};
	cmd[1] = 0x00 | cellGroup;
	uint16_t cmdPEC = pec(cmd, 2);
	cmd[2] = (uint8_t) (cmdPEC >> 8);
	cmd[3] = (uint8_t) cmdPEC;

	return recieveSPI(cmd, 4, dataIn, 8);
}
#endif

HAL_StatusTypeDef CLRCELL()
{
	//The CLRCELL command is 0x0711
	uint8_t cmd[4] = {0};
	cmd[0] = 0x07;
	cmd[1] = 0x11;
	uint16_t cmdPEC = pec(cmd, 2);
	cmd[2] = (uint8_t) (cmdPEC >> 8);
	cmd[3] = (uint8_t) cmdPEC;

	return sendSPI(cmd, 4);
}

HAL_StatusTypeDef RDSTATB(uint8_t * dataIn)
{
	//The RDSTATA command is 0x0012
	uint8_t cmd[4] = {0};
	cmd[1] = 0x12;
	uint16_t cmdPEC = pec(cmd, 2);
	cmd[2] = (uint8_t) (cmdPEC >> 8);
	cmd[3] = (uint8_t) cmdPEC;

	return recieveSPI(cmd, 4, dataIn, 8);
}

#ifndef DUAL_IC
#ifdef FULL_DIAG
int ADOW(uint8_t MD, uint8_t DCP)
{
	fault_t state = NORMAL;
	//Clear the voltage registers
	CLRCELL();
	uint8_t data[8];
	float pupCV[12];
	float pdpCV[12];
	int i = 0;
	int x;
	uint8_t cmd[4] = {0};

	//Pull up ADOW
	cmd[0] = 0x02 | (MD >> 1);
	cmd[1] = 0x28 | (MD << 7) | (DCP << 4);
	uint16_t cmdPEC = pec(cmd, 2);
	cmd[2] = (uint8_t) (cmdPEC >> 8);
	cmd[3] = (uint8_t) cmdPEC;

	sendSPI(cmd, 4); //Need to run this twice to allow the BMIC to generate the required current
	sendSPI(cmd, 4);
#ifndef WAIT_MICROS
		vTaskDelay(3 / portTICK_PERIOD_MS);
#else
		waitMicros(CNV_TIME);
#endif

	for (i = 0; i <= 4; i++)
	{
		RDCVX(readCmd[i], data);
		uint16_t pecRet = byte_combine(data[6], data[7]);
		if (pecRet == pec(data, 6))
		{
			x = (i-1) * 3;
			pupCV[x++] = byte_combine(data[1], data[0]);
			pupCV[x++] = byte_combine(data[3], data[2]);
			pupCV[x] = byte_combine(data[5], data[4]);
		}
	}

	//Pull down ADOW
	cmd[0] = 0x02 | (MD >> 1);
	cmd[1] = 0x68 | (MD << 7) | (DCP << 4);
	cmdPEC = pec(cmd, 2);
	cmd[2] = (uint8_t) (cmdPEC >> 8);
	cmd[3] = (uint8_t) cmdPEC;

	sendSPI(cmd, 4);
	sendSPI(cmd, 4); //Again, need to run this twice to allow the BMIC to generate the required current
#ifndef WAIT_MICROS
		vTaskDelay(3 / portTICK_PERIOD_MS);
#else
		waitMicros(CNV_TIME);
#endif

	for (i = 0; i <= 4; i++)
	{
		RDCVX(readCmd[i], data);
		uint16_t pecRet = byte_combine(data[6], data[7]);
		if (pecRet == pec(data, 6))
		{
			x = (i-1) * 3;
			pdpCV[x++] = byte_combine(data[1], data[0]);
			pdpCV[x++] = byte_combine(data[3], data[2]);
			pdpCV[x] = byte_combine(data[5], data[4]);
		}
	}

	return state;
}
#endif
#endif

void wakeSPI()
{
    HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_RESET);
    vTaskDelay(1);  // Guarantees the LTC681x will be in standby
    HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_SET);
    vTaskDelay(1);
}

uint16_t pec(uint8_t * data, uint8_t len)
{
	uint16_t remainder, addr;
	remainder = 16; //Initialize the PEC
	for (uint8_t i = 0; i < len; i++) //Loops for each byte in data array
	{
	  addr = ((remainder >> 7) ^ data[i]) & 0xff; //Calculate PEC table address
	  remainder = (remainder << 8) ^ crc15Table[addr];
	}

	return (remainder * 2); //The CRC15 has a 0 in the LSB so the remainder must be multiplied by 2
}

//uint16_t pec15_calc(uint8_t len, //Number of bytes that will be used to calculate a PEC
//                    uint8_t *data //Array of data that will be used to calculate  a PEC
//                   )
//{
//  uint16_t remainder,addr;
//
//  remainder = 16;//initialize the PEC
//  for (uint8_t i = 0; i<len; i++) // loops for each byte in data array
//  {
//    addr = ((remainder>>7)^data[i])&0xff;//calculate PEC table address
//    remainder = (remainder<<8)^pgm_read_word_near(crc15Table+addr);
//  }
//  return(remainder*2);//The CRC15 has a 0 in the LSB so the remainder must be multiplied by 2
//}

HAL_StatusTypeDef sendSPI(uint8_t * cmd, int len)
{
	HAL_StatusTypeDef state;
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_RESET);	//SS Low
	state = HAL_SPI_Transmit(LTC6811_SPI, cmd, len, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_SET);	//SS High

	return state;
}

HAL_StatusTypeDef recieveSPI(uint8_t * cmd, int cmdLen, uint8_t * data, int dataLen)
{
	//Note: dataLen should be number of bytes in the register group being read
	HAL_StatusTypeDef state;
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_RESET);	//SS Low
	state = HAL_SPI_Transmit(LTC6811_SPI, cmd, cmdLen, HAL_MAX_DELAY);
	HAL_SPI_Receive(LTC6811_SPI, data, dataLen, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_SET);	//SS High

	return state;
}

void handleHALError()
{
	//Basically just to let us know that we ran into an issue during transmission
}

void waitMicros(uint32_t cycles)
{
	//Will wait for at least a set amount of cycles
	//TODO: Count cycles per check using disassemble -> https://www.carminenoviello.com/2015/09/04/precisely-measure-microseconds-stm32/
	uint32_t cycleCount;
	DWT_CYCCNT = 0;
	DWT_CONTROL |= DWT_CYCCNTENA;
	do
	{
		cycleCount = DWT_CYCCNT;
	} while (cycleCount < cycles);
}
