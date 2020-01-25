/*
 * temp_adc.c
 *
 *  Created on: Feb 10, 2019
 *      Author: Matt Flanagan
 */
#include "temp_adc.h"

Success_t init_LTC2497(uint8_t * write_data);
uint16_t adc2temp(uint16_t adc_val);
inline uint16_t adc_extract(uint8_t * arr);
uint16_t adc2temp(uint16_t adc_val);

uint8_t temp_array[READ_MSG_SIZE];
uint16_t adc_val, adc_val0, adc_val1, adc_val2, adc_val3, adc_val4;
const uint8_t channel[NUM_CHANNELS] = {CHANNEL_0, CHANNEL_1,
                                       CHANNEL_2, CHANNEL_3, CHANNEL_4, CHANNEL_5, CHANNEL_6,
                                       CHANNEL_7, CHANNEL_8, CHANNEL_9, CHANNEL_10, CHANNEL_11,
                                       CHANNEL_12, CHANNEL_13, CHANNEL_14, CHANNEL_15
                                      };

const uint8_t chip[2] = {ID_TEMP_1, ID_TEMP_2};
uint8_t read_byte = 0;
uint8_t write_data[WRITE_MSG_SIZE + 1];

void task_acquire_temp() {
	uint8_t i, tap, faultCount;
	uint16_t temperature;

	init_LTC2497(write_data);

	TickType_t time_init = 0;
	while (PER == GREAT)
	{
	  for (i = 0; i < NUM_TEMP; ++i)
	  {
		  //Set the next channel to read for both ICs and start conversion
		  write_data[1] = channel_combine(channel[i]);
		  HAL_I2C_Master_Transmit(&hi2c1, (uint16_t) write_data[0], &write_data[1], WRITE_MSG_SIZE, 0xFFFF);
		  while (hi2c1.State != HAL_I2C_STATE_READY)
		  {
			  //Wait for the send to stop
		  }
		  vTaskDelay(WRITE_REQ_WAIT);
		  for (tap = 0; tap <= 1; ++tap)
		  {
			  temperature = readLTCValue(i, tap);

			  if (xSemaphoreTake(bms.temp.sem, TIMEOUT) == pdTRUE)
			  {
				  if (temperature <= 0 || temperature >= 1000 )
				  {
					  faultCount = 0;
					  //Probably a bad sensor reading
					  do {
						  temperature = readLTCValue(i, tap); //Retake the measurement until we get a good value
					  } while (temperature <= 10 && temperature >= 100 && faultCount++ < 10); //Try 10 times
					  if (faultCount == 10) //We tried 10 times and it still doesn't work
					  {
						  if (tap == 1)
						  {
							  bms.temp1_con = FAILURE;
						  } else
						  {
							  bms.temp2_con = FAILURE;
						  }
					  } else
					  {
						  bms.temp.data[tap][i] = temperature;
					  }
				  } else
				  {
					  bms.temp.data[tap][i] = temperature;
				  }
				  xSemaphoreGive(bms.temp.sem);
			  } else
			  {
				  //Error
			  }
		  }
	  }
	}
	vTaskDelayUntil(&time_init, ACQUIRE_TEMP_RATE);
}

uint16_t readLTCValue(uint8_t currentChannel, uint8_t tap)
{
	//Set the next channel to read for both ICs and start conversion
	write_data[1] = channel_combine(channel[currentChannel]);
	HAL_I2C_Master_Transmit(&hi2c1, (uint16_t) write_data[0], &write_data[1], WRITE_MSG_SIZE, 0xFFFF);
	while (hi2c1.State != HAL_I2C_STATE_READY)
	{
	  //Wait for the send to stop
	}
	vTaskDelay(WRITE_REQ_WAIT);
	read_byte = set_address(chip[tap], READ_ENABLE);
	HAL_I2C_Master_Receive(&hi2c1,(uint16_t) read_byte, &temp_array[0], READ_MSG_SIZE, 0xFFFF);
	while (hi2c1.State != HAL_I2C_STATE_READY)
	{
	  //Wait for the send to stop
	}
	adc_val = adc_extract(temp_array);

	return(adc2temp(adc_val));
}

inline uint16_t adc_extract(uint8_t * arr)
{
	uint16_t result;
	result = ((uint16_t) arr[0] << 10) | ((uint16_t) arr[1] << 2) | ((uint16_t) arr[2] >> 6);
	return result;
}

uint16_t adc2temp(uint16_t adc_value)
{
	float voltage;
	float thermistor_res;
	double temperature;
	//calculate the voltage from the adc_val
	voltage = VOLTAGE_REF * ((float) (adc_value)) / 0xFFFF;
	//calculate the resistance from the voltage
	thermistor_res = (voltage * THERM_RESIST) / (VOLTAGE_TOP - voltage);
	//calculate the temperature
	temperature =  B_VALUE / log (thermistor_res / R_INF_3977) - KELVIN_2_CELSIUS;

	return (uint16_t)(10 * temperature);
}

Success_t init_LTC2497(uint8_t * write_data)
{
	//Try connecting to both individually to check connection status
	uint8_t timeout = 0;
	Success_t status = SUCCESSFUL;
	write_data[0] = set_address(ID_TEMP_1, WRITE_ENABLE);
	write_data[1] = channel_combine(channel[0]);
	write_data[2] = 0xFF;
	HAL_I2C_Master_Transmit(&hi2c1, (uint16_t) write_data[0], &write_data[1], WRITE_MSG_SIZE, 0xFFFF);
	while (hi2c1.State != HAL_I2C_STATE_READY && timeout++ < WRITE_TIMEOUT)
	{
	  //Wait for the send to stop
	}
	if (timeout >= WRITE_TIMEOUT)
	{
		status = FAILURE;
		bms.temp1_con = FAULTED;
	}
	vTaskDelay(WRITE_REQ_WAIT);

	write_data[0] = set_address(ID_TEMP_2, WRITE_ENABLE);
	write_data[1] = channel_combine(channel[0]);
	write_data[2] = 0xFF;
	HAL_I2C_Master_Transmit(&hi2c1, (uint16_t) write_data[0], &write_data[1], WRITE_MSG_SIZE, 0xFFFF);
	while (hi2c1.State != HAL_I2C_STATE_READY && timeout++ < WRITE_TIMEOUT)
	{
	  //Wait for the send to stop
	}
	if (timeout >= WRITE_TIMEOUT)
	{
		status = FAILURE;
		bms.temp2_con = FAULTED;
	}
	vTaskDelay(WRITE_REQ_WAIT);

	return status;
}
