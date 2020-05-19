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
	while (initLTC() == HAL_ERROR) {
		// Registers were not configured properly
	}

	// Clear all cell readings
	broadcast_poll(CLRCELL);

	while (1) {

		vTaskDelay(1 / portTICK_PERIOD_MS);
		broadcast_poll(ADCVSC(MD_NORMAL, DCP_DISCHARGE_NOT_PERMITTED));

		// Read over/under voltage conditions
		pollOpenWire();
		pollOVUVStatus();

		for (i = 0; i <= (NUM_VTAPS / 3); i++) {
			int valid_PEC = broadcast_read(readCmd[i], LTC6811_REG_SIZE, data);

			if (valid_PEC
					>= 0&& xSemaphoreTake(bms.vtap.sem, TIMEOUT) == pdTRUE) {
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

	uint16_t voltage_low = VUV(1);
	uint16_t voltage_high = VOV(4.2);

	uint8_t cmd[LTC6811_REG_SIZE] = { 0b00000101, voltage_low & 0xFF,
			(((voltage_high & 0xF) << 4) & 0xF0)
					| (((voltage_low >> 8) & 0xF) & 0x0F), (voltage_high >> 4)
					& 0xFF, 0, 0 };

	uint8_t register_check[LTC6811_REG_SIZE] = { 0 };

	wakeSPI();
	broadcast_write(WRCFGA, LTC6811_REG_SIZE, cmd);
	broadcast_read(RDCFGA, LTC6811_REG_SIZE, register_check);

	// Ensure that configuration register is as expected
	for (int i = 0; i < LTC6811_REG_SIZE; i++) {
		if (register_check[i] != cmd[i])
			return HAL_ERROR;
	}

	broadcast_poll(DIAGN);

	return HAL_OK;
}

HAL_StatusTypeDef pollOVUVStatus() {
	uint8_t reg[LTC6811_REG_SIZE] = { 0 };

	broadcast_read(RDSTATA, LTC6811_REG_SIZE, reg);

	uint16_t sum_voltages = byte_combine(reg[1], reg[0]);

	broadcast_read(RDSTATB, LTC6811_REG_SIZE, reg);

	if (reg[2] != 0 || reg[3] != 0b00000100) {
		// If the LTC has not had a command in the past 2 seconds, the CFG0-5 registers will be reset.
		broadcast_read(RDCFGA, LTC6811_REG_SIZE, reg);

		if (reg[1] == 0) {
			// Watchdog has tripped, reset Configuration registers
			while (initLTC() == HAL_ERROR)
				;
			return HAL_OK;
		}
		broadcast_read(RDSTATB, LTC6811_REG_SIZE, reg);

		return HAL_ERROR;
	}

	return HAL_OK;

}

HAL_StatusTypeDef pollOpenWire() {
	int x, i;
	uint8_t data[8];

	broadcast_poll(
			ADOW(MD_NORMAL, PUP_PULL_UP, DCP_DISCHARGE_NOT_PERMITTED,
					CH_ALL_CELLS));

	broadcast_poll(
			ADOW(MD_NORMAL, PUP_PULL_UP, DCP_DISCHARGE_NOT_PERMITTED,
					CH_ALL_CELLS));

	for (i = 0; i <= (NUM_VTAPS / 3); i++) {
		int valid_PEC = broadcast_read(readCmd[i], LTC6811_REG_SIZE, data);

		if (valid_PEC >= 0 && xSemaphoreTake(bms.vtap.sem, TIMEOUT) == pdTRUE) {
			x = (i) * 3;
			bms.vtap.data[x++] = byte_combine(data[1], data[0]); // Remember, these voltages need to be * 0.0001!
			bms.vtap.data[x++] = byte_combine(data[3], data[2]);
			bms.vtap.data[x] = byte_combine(data[5], data[4]);
			xSemaphoreGive(bms.vtap.sem);
		}
	}

	broadcast_poll(
			ADOW(MD_NORMAL, PUP_PULL_DOWN, DCP_DISCHARGE_NOT_PERMITTED,
					CH_ALL_CELLS));
	broadcast_poll(
			ADOW(MD_NORMAL, PUP_PULL_DOWN, DCP_DISCHARGE_NOT_PERMITTED,
					CH_ALL_CELLS));

	for (i = 0; i <= (NUM_VTAPS / 3); i++) {
		int valid_PEC = broadcast_read(readCmd[i], LTC6811_REG_SIZE, data);
		if (valid_PEC >= 0) {
			x = (i) * 3;
			for (int j = 0; j < 3; j++) {
				// Delta greater than 400mV from pull-up compared to pull-down indicates an open wire
				if (abs(
						bms.vtap.data[x + j]
								- byte_combine(data[2 * j + 1], data[2 * j]))
						> 4000)
					return HAL_ERROR;
			}
		}
	}

	return HAL_OK;

}

void wakeSPI() {
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin,
			GPIO_PIN_RESET);
	vTaskDelay(1);  // Guarantees the LTC681x will be in standby
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_SET);
	vTaskDelay(1);
}
