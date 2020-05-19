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
	HAL_StatusTypeDef valid_PEC;
	uint16_t openCells;
	uint32_t ovuvCells;

	while (configure_LTC6811() == HAL_ERROR)
	{
		// Registers were not configured properly
		bms.vstack_con = FAULTED;
		vTaskDelay(1 / portTICK_RATE_MS);
	}
	bms.vstack_con = NORMAL;

	// Clear all cell readings
	poll_command(CLRCELL);

	while (1)
	{

		if (check_LTC6811_config_reset() == HAL_ERROR)
		{
			while (configure_LTC6811() == HAL_ERROR)
			{
				// Registers were not configured properly
				bms.vstack_con = FAULTED;
				vTaskDelay(1 / portTICK_RATE_MS);
			}
			bms.vstack_con = NORMAL;
		}

		poll_command(ADCVSC(MD_NORMAL, DCP_DISCHARGE_NOT_PERMITTED));

		// Read over/under voltage conditions
		openCells = pollOpenWireStatus();
		if (openCells != 0)
		{
			// Hanlde Open Wire
		}

		for (i = 0; i <= (NUM_VTAPS / 3); i++)
		{
			valid_PEC = read_command(volt_read_commands[i], data);

			if (valid_PEC == HAL_OK && xSemaphoreTake(bms.vtap.sem, TIMEOUT) == pdTRUE)
			{
				x = i * 3;
				bms.vtap.data[x] = byte_combine(data[1], data[0]); // Remember, these voltages need to be * 0.0001!
				bms.vtap.data[x + 1] = byte_combine(data[3], data[2]);
				bms.vtap.data[x + 2] = byte_combine(data[5], data[4]);
				xSemaphoreGive(bms.vtap.sem);
			}
		}

		// Check for over/under voltage status from BMIC
		ovuvCells = pollOVUVStatus();

		// Only care about the numver of cells configured for
		// Mask out the unused cells
		if (ovuvCells & ~(0xFFFFFFFF << (2 * NUM_VTAPS)) != 0) 
		{
			// Hnalde OV/UV
		}

		vTaskDelayUntil(&initTime, VSTACK_RATE);
	}
}
