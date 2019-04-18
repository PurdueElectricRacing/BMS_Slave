///*
// * ltc6811.c
// *
// *  Created on: Feb 10, 2019
// *      Author: Matt Flanagan
// */
//
#include "vstack.h"
#include "stm32l4xx_hal_spi.h"

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
#define LTC6811_CMD_ADCV		0x
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

HAL_StatusTypeDef LTC6811_init();

void task_VSTACK() {
	TickType_t time_init = 0;

	while (1) {
		time_init = xTaskGetTickCount();
		uint8_t din[1] = { 0x1 };
		LTC6811_init();
		//LTC6811_addrWrite(din, 1,  LTC6811_CMD_WRCFGA);
		vTaskDelayUntil(&time_init, VSTACK_RATE);
	}
}

HAL_StatusTypeDef init_LTC6811() {
	//this function is used to initiate communication to the LTC6811 chip

	return HAL_OK;
}

// Code Converted by Raymond Starts Here

//Wake isoSPI up from idle state
void wakeup_idle(uint8_t total_ic) {
	uint8_t tx_arr[1] = 0xFF;
	for (int i = 0; i < total_ic; i++) {
		HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin,
				GPIO_PIN_RESET);
		HAL_SPI_Transmit(LTC6811_SPI, tx_arr, 1, HAL_MAX_DELAY); //Guarantees the isoSPI will be in ready mode
		HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_SET);
	}
}

//Generic wakeup command to wake the LTC681x from sleep
void wakeup_sleep(uint8_t total_ic) {
	for (int i = 0; i < total_ic; i++) {
		HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin,
				GPIO_PIN_RESET);
		vTaskDelay(1);	// Guarantees the LTC681x will be in standby
		HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_SET);
		vTaskDelay(1);
	}
}

//Generic function to write 68xx commands. Function calculated PEC for tx_cmd data
void cmd_68(uint8_t tx_cmd[2]) {
	uint8_t cmd[4];
	uint16_t cmd_pec;
	uint8_t md_bits;
	cmd[0] = tx_cmd[0];
	cmd[1] = tx_cmd[1];
	cmd_pec = pec15_calc(&cmd, 2);
	cmd[2] = (uint8_t) (cmd_pec >> 8);
	cmd[3] = (uint8_t) (cmd_pec);
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(LTC6811_SPI, cmd, 4, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_SET);
}

//Generic function to write 68xx commands and write payload data. Function calculated PEC for tx_cmd data
void write_68(uint8_t total_ic, uint8_t tx_cmd[2], uint8_t data[]) {
	const uint8_t BYTES_IN_REG = 6;
	const uint8_t CMD_LEN = 4 + (8 * total_ic);
	uint8_t *cmd;
	uint16_t data_pec;
	uint16_t cmd_pec;
	uint8_t cmd_index;
	cmd = (uint8_t *) pvPortMalloc(CMD_LEN * sizeof(uint8_t));
	cmd[0] = tx_cmd[0];
	cmd[1] = tx_cmd[1];
	cmd_pec = pec15_calc(2, cmd);
	cmd[2] = (uint8_t) (cmd_pec >> 8);
	cmd[3] = (uint8_t) (cmd_pec);
	cmd_index = 4;
	for (uint8_t current_ic = total_ic; current_ic > 0; current_ic--) // executes for each LTC681x, this loops starts with the last IC on the stack.
			{	//The first configuration written is received by the last IC in the daisy chain
		for (uint8_t current_byte = 0; current_byte < BYTES_IN_REG;
				current_byte++) {
			cmd[cmd_index] = data[((current_ic - 1) * 6) + current_byte];
			cmd_index = cmd_index + 1;
		}
		data_pec = (uint16_t) pec15_calc(BYTES_IN_REG, &data[(current_ic - 1) * 6]); // calculating the PEC for each Iss configuration register data
		cmd[cmd_index] = (uint8_t) (data_pec >> 8);
		cmd[cmd_index + 1] = (uint8_t) data_pec;
		cmd_index = cmd_index + 2;
	}
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(LTC6811_SPI, cmd, CMD_LEN, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_SET);
	vPortFree(cmd);
}

//Generic function to write 68xx commands and read data. Function calculated PEC for tx_cmd data
int8_t read_68(uint8_t total_ic, uint8_t tx_cmd[2], uint8_t *rx_data) {
	const uint8_t BYTES_IN_REG = 8;
	uint8_t cmd[4];
	uint8_t data[256];
	int8_t pec_error = 0;
	uint16_t cmd_pec;
	uint16_t data_pec;
	uint16_t received_pec;
	cmd[0] = tx_cmd[0];
	cmd[1] = tx_cmd[1];
	cmd_pec = pec15_calc(2, cmd);
	cmd[2] = (uint8_t) (cmd_pec >> 8);
	cmd[3] = (uint8_t) (cmd_pec);
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(LTC6811_SPI, cmd, 4, HAL_MAX_DELAY);
	HAL_SPI_Receive(LTC6811_SPI, data, (BYTES_IN_REG * total_ic), HAL_MAX_DELAY);	//Read the configuration data of all ICs on the daisy chain into rx_data[] array
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_SET);

	for (uint8_t current_ic = 0; current_ic < total_ic; current_ic++) //executes for each LTC681x in the daisy chain and packs the data
			{	//into the r_comm array as well as check the received data for any bit errors
		for (uint8_t current_byte = 0; current_byte < BYTES_IN_REG;
				current_byte++) {
			rx_data[(current_ic * 8) + current_byte] = data[current_byte
					+ (current_ic * BYTES_IN_REG)];
		}
		received_pec = (rx_data[(current_ic * 8) + 6] << 8)
				+ rx_data[(current_ic * 8) + 7];
		data_pec = pec15_calc(6, &rx_data[current_ic * 8]);
		if (received_pec != data_pec) {
			pec_error = -1;
		}
	}
	return (pec_error);
}

//Calculates  and returns the CRC15
uint16_t pec15_calc(uint8_t len, //Number of bytes that will be used to calculate a PEC
		uint8_t *data //Array of data that will be used to calculate  a PEC
		) {
	uint16_t remainder, addr;
	remainder = 16; //initialize the PEC
	for (uint8_t i = 0; i < len; i++) // loops for each byte in data array
			{
		addr = ((remainder >> 7) ^ data[i]) & 0xff; //calculate PEC table address
		remainder = (remainder << 8) ^ crc15Table[addr];
	}
	return (remainder * 2); //The CRC15 has a 0 in the LSB so the remainder must be multiplied by 2
}

//Starts cell voltage conversion
void LTC681x_adcv(uint8_t MD, //ADC Mode
		uint8_t DCP, //Discharge Permit
		uint8_t CH //Cell Channels to be measured
		) {
	uint8_t cmd[2];
	uint8_t md_bits;
	md_bits = (MD & 0x02) >> 1;
	cmd[0] = md_bits + 0x02;
	md_bits = (MD & 0x01) << 7;
	cmd[1] = md_bits + 0x60 + (DCP << 4) + CH;
	cmd_68(cmd);
}

//Start a GPIO and Vref2 Conversion
void LTC681x_adax(uint8_t MD, //ADC Mode
		uint8_t CHG //GPIO Channels to be measured)
		) {
	uint8_t cmd[4];
	uint8_t md_bits;

	md_bits = (MD & 0x02) >> 1;
	cmd[0] = md_bits + 0x04;
	md_bits = (MD & 0x01) << 7;
	cmd[1] = md_bits + 0x60 + CHG;
	cmd_68(cmd);

}

//Start a Status ADC Conversion
void LTC681x_adstat(uint8_t MD, //ADC Mode
		uint8_t CHST //GPIO Channels to be measured
		) {
	uint8_t cmd[4];
	uint8_t md_bits;

	md_bits = (MD & 0x02) >> 1;
	cmd[0] = md_bits + 0x04;
	md_bits = (MD & 0x01) << 7;
	cmd[1] = md_bits + 0x68 + CHST;
	cmd_68(cmd);
}

// Start a Status register redundancy test Conversion
void LTC681x_adstatd(uint8_t MD, //ADC Mode
		uint8_t CHST //GPIO Channels to be measured
		) {
	uint8_t cmd[2];
	uint8_t md_bits;

	md_bits = (MD & 0x02) >> 1;
	cmd[0] = md_bits + 0x04;
	md_bits = (MD & 0x01) << 7;
	cmd[1] = md_bits + 0x08 + CHST;
	cmd_68(cmd);

}

// Start an open wire Conversion
void LTC681x_adow(uint8_t MD, //ADC Mode
		uint8_t PUP, //PULL UP OR DOWN current
		uint8_t CH, //Channels
		uint8_t DCP //Discharge Permit
		) {
	uint8_t cmd[2];
	uint8_t md_bits;
	md_bits = (MD & 0x02) >> 1;
	cmd[0] = md_bits + 0x02;
	md_bits = (MD & 0x01) << 7;
	cmd[1] = md_bits + 0x28 + (PUP << 6) + CH + (DCP << 4);
	cmd_68(cmd);
}

//Starts cell voltage and SOC conversion
void LTC681x_adcvsc(uint8_t MD, //ADC Mode
		uint8_t DCP //Discharge Permit
		) {
	uint8_t cmd[2];
	uint8_t md_bits;
	md_bits = (MD & 0x02) >> 1;
	cmd[0] = md_bits | 0x04;
	md_bits = (MD & 0x01) << 7;
	cmd[1] = md_bits | 0x60 | (DCP << 4) | 0x07;
	cmd_68(cmd);

}

// Starts cell voltage  and GPIO 1&2 conversion
void LTC681x_adcvax(uint8_t MD, //ADC Mode
		uint8_t DCP //Discharge Permit
		) {
	uint8_t cmd[2];
	uint8_t md_bits;
	md_bits = (MD & 0x02) >> 1;
	cmd[0] = md_bits | 0x04;
	md_bits = (MD & 0x01) << 7;
	cmd[1] = md_bits | ((DCP & 0x01) << 4) + 0x6F;
	cmd_68(cmd);
}

//Starts cell voltage overlap conversion
void LTC681x_adol(uint8_t MD, //ADC Mode
		uint8_t DCP //Discharge Permit
		) {
	uint8_t cmd[2];
	uint8_t md_bits;
	md_bits = (MD & 0x02) >> 1;
	cmd[0] = md_bits + 0x02;
	md_bits = (MD & 0x01) << 7;
	cmd[1] = md_bits + (DCP << 4) + 0x01;
	cmd_68(cmd);
}

//Sends the poll adc command
uint8_t LTC681x_pladc() {
	uint8_t cmd[4];
	uint8_t adc_state = 0xFF;
	uint16_t cmd_pec;
	cmd[0] = 0x07;
	cmd[1] = 0x14;
	cmd_pec = pec15_calc(2, cmd);
	cmd[2] = (uint8_t) (cmd_pec >> 8);
	cmd[3] = (uint8_t) (cmd_pec);
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(LTC6811_SPI, cmd, 4, HAL_MAX_DELAY);
	HAL_SPI_Receive(LTC6811_SPI, &adc_state, 1, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_SET);
	return (adc_state);
}

//This function will block operation until the ADC has finished it's conversion
uint32_t LTC681x_pollAdc() {
	uint32_t counter = 0;
	uint8_t finished = 0;
	uint8_t current_time = 0;
	uint8_t cmd[4];
	uint16_t cmd_pec;
	cmd[0] = 0x07;
	cmd[1] = 0x14;
	cmd_pec = pec15_calc(2, cmd);
	cmd[2] = (uint8_t) (cmd_pec >> 8);
	cmd[3] = (uint8_t) (cmd_pec);
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(LTC6811_SPI, cmd, 4, HAL_MAX_DELAY);
	while ((counter < 200000) && (finished == 0)) {
		HAL_SPI_Receive(LTC6811_SPI, &current_time, 1, HAL_MAX_DELAY);
		if (current_time > 0) {
			finished = 1;
		} else {
			counter = counter + 10;
		}
	}
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_SET);
	return (counter);
}

// Reads the raw cell voltage register data
void LTC681x_rdcv_reg(uint8_t reg, //Determines which cell voltage register is read back
		uint8_t total_ic, //the number of ICs in the
		uint8_t *data //An array of the unparsed cell codes
		) {
	const uint8_t REG_LEN = 8; //number of bytes in each ICs register + 2 bytes for the PEC
	uint8_t cmd[4];
	uint16_t cmd_pec;
	if (reg == 1)     //1: RDCVA
			{
		cmd[1] = 0x04;
		cmd[0] = 0x00;
	} else if (reg == 2) //2: RDCVB
			{
		cmd[1] = 0x06;
		cmd[0] = 0x00;
	} else if (reg == 3) //3: RDCVC
			{
		cmd[1] = 0x08;
		cmd[0] = 0x00;
	} else if (reg == 4) //4: RDCVD
			{
		cmd[1] = 0x0A;
		cmd[0] = 0x00;
	} else if (reg == 5) //5: RDCVE
			{
		cmd[1] = 0x09;
		cmd[0] = 0x00;
	} else if (reg == 6) //6: RDCVF
			{
		cmd[1] = 0x0B;
		cmd[0] = 0x00;
	}
	cmd_pec = pec15_calc(2, cmd);
	cmd[2] = (uint8_t) (cmd_pec >> 8);
	cmd[3] = (uint8_t) (cmd_pec);

	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(LTC6811_SPI, cmd, 4, HAL_MAX_DELAY);
	HAL_SPI_Receive(LTC6811_SPI, data, (REG_LEN * total_ic), HAL_MAX_DELAY);
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_SET);
}

// Write the ltc6811 Sctrl register
void LTC681x_wrsctrl(uint8_t sctrl_reg, uint8_t tx_data[]) {
	uint8_t cmd[2];
	uint8_t write_buffer[256];
	uint8_t write_count = 0;
	uint8_t c_ic = 0;
	if (sctrl_reg == 0) {
		cmd[0] = 0x00;
		cmd[1] = 0x14;
	} else {
		cmd[0] = 0x00;
		cmd[1] = 0x1C;
	}
	for (uint8_t data = 0; data < 6; data++) {
		write_buffer[write_count] = tx_data[data];
		write_count++;
	}
	write_68(1, cmd, write_buffer);
}

// Code Converted by Raymond Ends Here

//Calculates PEC or CRC
//TODO might want to make LTCHandle_t a pointer
uint16_t LTC6811Pec(uint8_t *data, uint8_t len) {
	uint16_t remainder, addr;

	remainder = 16;	//initialize the PEC
	for (uint8_t i = 0; i < len; i++) // loops for each byte in data array
			{
		addr = ((remainder >> 7) ^ data[i]) & 0xff; //calculate PEC table address
		remainder = (remainder << 8) ^ crc15Table[addr];
	}

	return (remainder * 2); //The CRC15 has a 0 in the LSB so the remainder must be multiplied by 2
}

HAL_StatusTypeDef LTC6811_addrWrite(uint8_t *din, uint8_t len, uint16_t cmd) {
	uint8_t * tx_arr;
	tx_arr = (uint8_t *) pvPortMalloc((len + 6) * sizeof(*tx_arr));
	if (NULL == tx_arr)
		return HAL_ERROR;

	//Generate CMD0 and CMD1 bits
	tx_arr[0] = (uint8_t) cmd;
	tx_arr[1] = 0xFF & LTC6811_SPI_ADDR & ((uint8_t) cmd >> 8);

	//Generate PEC
	uint16_t pec = LTC6811Pec(tx_arr, 2);
	tx_arr[2] = (uint8_t) pec;
	tx_arr[3] = (uint8_t) (pec >> 8);
	//Compile write array
	memcpy(&tx_arr[4], din, len);

	//Generate PEC for data
	pec = LTC6811Pec(&tx_arr[4], len);
	tx_arr[len + 6 - 2] = (uint8_t) pec;
	tx_arr[len + 6 - 1] = (uint8_t) (pec >> 8);

	//send data
	//TODO change to interrupt and include error checking
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_RESET);//SS Low
	HAL_SPI_Transmit(LTC6811_SPI, tx_arr, len + 6, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_SET);//SS High
	vPortFree(tx_arr);
	return HAL_OK;
}

HAL_StatusTypeDef LTC6811_addrRead(uint8_t *dout, uint8_t len, uint16_t cmd) {

	uint8_t tx_arr[4];

	//Generate CMD0 and CMD1 bits
	tx_arr[0] = (uint8_t) cmd;
	tx_arr[1] = 0xFF & LTC6811_SPI_ADDR & ((uint8_t) cmd >> 8);

	//Generate PEC
	uint16_t pec = LTC6811Pec((uint8_t *) &cmd, 2);
	tx_arr[2] = (uint8_t) pec;
	tx_arr[3] = (uint8_t) (pec >> 8);

	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_RESET);//SS Low
	HAL_SPI_Transmit(LTC6811_SPI, tx_arr, 4, HAL_MAX_DELAY);

	HAL_SPI_Receive(LTC6811_SPI, dout, len, 5);
	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_SET);//SS High

	//HAL_SPI_TransmitReceive(LTC6811_SPI, tx_arr, dout, len + 4, 5);

	//Check PEC
	if (LTC6811Pec(dout, len - 2) == (uint16_t) dout[len - 2]) {
		return HAL_OK;
	} else
		return HAL_ERROR;
}

//HAL_StatusTypeDef LTC6811_addrRead_IT(LTCHandle_t ltc)

//HAL_StatusTypeDef LTC6811_addrPoll(LTCHandle_t ltc, uint8_t *din,
//		uint8_t len) {
//	//Generate CMD0 and CMD1 bits
//	//Compile write array
//	//pull SS line low
//	//HAL_SPI_TransmitReceive
//}

HAL_StatusTypeDef LTC6811_init() {
	//Config ADC
	uint8_t tx_arr[6] = { 0, 0, 0, 0, 0, 0 };
	//Set max pack voltage
	tx_arr[1] = LTC6811_MIN_PCKV * (16 * .0001);
	//TODO need to fix bit shifting
	//tx_arr[2] =  LTC6811_MAX_PCKV / (16 * .0001);

	if (HAL_OK != LTC6811_addrWrite(tx_arr, 6, LTC6811_CMD_WRCFGA))
		return HAL_ERROR;

	uint8_t rx_arr[6] = { 0, 0, 0, 0, 0, 0 };
	LTC6811_addrRead(rx_arr, 6, LTC6811_CMD_RDCFGA);
	if (rx_arr[1] != tx_arr[1] || rx_arr[2] != tx_arr[2])
		return HAL_ERROR;

	return HAL_OK;
}

//HAL_StatusTypeDef LTC6811_deInit(LTCHandle_t ltc, uint8_t *din,
//		uint8_t len) {
//
//}
