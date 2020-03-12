/*
 * vstack.c
 *
 * Created on: Feb 10, 2019
 * Authors: Matt Flanagan & Dawson Moore
 *
 */

#include "vstack.h"

void task_VSTACK() {
	TickType_t initTime = 0;
	uint8_t i, x;
	uint16_t pecRet; //Marked as unused due to invalid PEC issue
	uint8_t data[8]; //6 bytes for the data and 2 for the pec

	wakeSPI(); //This shouldn't be required anymore, but it's still good to have
	while (initLTC()) {
		//Probably a bus issue
	}
	broadcast_poll(CLRCELL);
	while (1) {

		vTaskDelay(1 / portTICK_PERIOD_MS);
		broadcast_poll(
				ADCV(MD_NORMAL, DCP_DISCHARGE_NOT_PERMITTED, CH_ALL_CELLS));

		vTaskDelay(3 / portTICK_PERIOD_MS);

		for (i = 0; i <= (NUM_VTAPS / 3); i++) {
			broadcast_read(readCmd[i], LTC6811_REG_SIZE, data);
			pecRet = byte_combine(data[6], data[7]);
			if (xSemaphoreTake(bms.vtap.sem, TIMEOUT) == pdTRUE) {
				x = (i) * 3;
				bms.vtap.data[x++] = byte_combine(data[1], data[0]); // Remember, these voltages need to be * 0.0001!
				bms.vtap.data[x++] = byte_combine(data[3], data[2]);
				bms.vtap.data[x] = byte_combine(data[5], data[4]);
				xSemaphoreGive(bms.vtap.sem);
			}
		}
		vTaskDelayUntil(&initTime, VSTACK_RATE);
	}
}

HAL_StatusTypeDef initLTC() {
	uint8_t cmd[LTC6811_REG_SIZE] = { 0b10100000, 123, 123, 123, 0, 0 };
	uint8_t cjck[LTC6811_REG_SIZE] = {0};

	wakeSPI();
	broadcast_write(WRCFGA, LTC6811_REG_SIZE, cmd);

	broadcast_read(RDCFGA, LTC6811_REG_SIZE, cjck);
	broadcast_read(RDCFGA, LTC6811_REG_SIZE, cjck);
	broadcast_read(RDCFGA, LTC6811_REG_SIZE, cjck);


	return 0;
}

HAL_StatusTypeDef RDCVX(uint8_t cellGroup, uint8_t * dataIn) {
	uint8_t cmd[4] = { 0 };
	cmd[0] = 0x00;
	cmd[1] = cellGroup;
	uint16_t cmdPEC = pec(cmd, 2);
	cmd[2] = (uint8_t) (cmdPEC >> 8);
	cmd[3] = (uint8_t) cmdPEC;

	return recieveSPI(cmd, 4, dataIn, 8);
}

void wakeSPI() {
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin,
			GPIO_PIN_RESET);
	vTaskDelay(1);  // Guarantees the LTC681x will be in standby
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_SET);
	vTaskDelay(1);
}

uint16_t pec(uint8_t * data, uint8_t len) {
	uint16_t remainder, addr;
	remainder = 16; //Initialize the PEC
	for (uint8_t i = 0; i < len; i++) //Loops for each byte in data array
			{
		addr = ((remainder >> 7) ^ data[i]) & 0xff; //Calculate PEC table address
		remainder = (remainder << 8) ^ crc15Table[addr];
	}

	return (remainder * 2); //The CRC15 has a 0 in the LSB so the remainder must be multiplied by 2
}

HAL_StatusTypeDef sendSPI(uint8_t * cmd, int len) {
	HAL_StatusTypeDef state;
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin,
			GPIO_PIN_RESET); //SS Low
	state = HAL_SPI_Transmit(LTC6811_SPI, cmd, len, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_SET); //SS High

	return state;
}

HAL_StatusTypeDef recieveSPI(uint8_t * cmd, int cmdLen, uint8_t * data,
		int dataLen) {
//Note: dataLen should be number of bytes in the register group being read
	HAL_StatusTypeDef state;
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin,
			GPIO_PIN_RESET); //SS Low
	state = HAL_SPI_Transmit(LTC6811_SPI, cmd, cmdLen, HAL_MAX_DELAY);
	HAL_Delay(1);
	HAL_SPI_Receive(LTC6811_SPI, data, dataLen, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_SET); //SS High

	return state;
}
