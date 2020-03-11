/*
 * vstack.c
 *
 * Created on: Feb 10, 2019
 * Authors: Matt Flanagan & Dawson Moore
 *
 */

#include "vstack.h"

void task_VSTACK()
{
	TickType_t initTime = 0;
	uint8_t i, x;
	uint16_t pecRet; //Marked as unused due to invalid PEC issue
	uint8_t data[8]; //6 bytes for the data and 2 for the pec

	wakeSPI(); //This shouldn't be required anymore, but it's still good to have
	while (initLTC())
	{
		//Probably a bus issue
	}
    CLRCELL();
	while (PER == GREAT)
	{
		ADCV(NORMAL_MODE, DISCHARGE_NOT_PERMITTED, CELLS_1_7);
		vTaskDelay(1 / portTICK_PERIOD_MS);
		ADCV(NORMAL_MODE, DISCHARGE_NOT_PERMITTED, ALL_CELLS);
		vTaskDelay(3 / portTICK_PERIOD_MS);
		for (i = 0; i <= (NUM_VTAPS / 3); i++)
		{
			RDCVX(readCmd[i], data);
			pecRet = byte_combine(data[6], data[7]);
			if (xSemaphoreTake(bms.vtap.sem, TIMEOUT) == pdTRUE)
			{
				x = (i-1) * 3;
				bms.vtap.data[x++] = byte_combine(data[1], data[0]); // Remember, these voltages need to be * 0.0001!
				bms.vtap.data[x++] = byte_combine(data[3], data[2]);
				bms.vtap.data[x] = byte_combine(data[5], data[4]);
				xSemaphoreGive(bms.vtap.sem);
			}
		}
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
	uint16_t cmd[2] = {0};
	cmd[0] = 0x260 | (MD << 7) | (DCP << 4) | CH;
	cmd[1] = pec((uint8_t*) cmd, 2);

	return sendSPI((uint8_t*) cmd, 4);
}

HAL_StatusTypeDef RDCVX(uint8_t cellGroup, uint8_t * dataIn)
{
	uint8_t cmd[4] = {0};
	cmd[1] = 0x00 | cellGroup;
	uint16_t cmdPEC = pec(cmd, 2);
	cmd[2] = (uint8_t) (cmdPEC >> 8);
	cmd[3] = (uint8_t) cmdPEC;

	return recieveSPI(cmd, 4, dataIn, 8);
}

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
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_SET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Receive(LTC6811_SPI, data, dataLen, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_SET);	//SS High

	return state;
}
