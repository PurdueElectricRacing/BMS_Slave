/*
 * LTC6811.c
 *
 * Implementation of the interface with LTC6811-1
 */
#include "LTC6811.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Initializes a LTC6811 struct
 */
//void LTC6811_init(LTC6811* ltc){
//    memset(ltc, 0, sizeof(LTC6811));
//
//    return;
//}
/**
 * @brief Wakes up an entire daisy chain from core SLEEP state
 *
 * @warning
 * The function is blocking and takes daisy_chain_n * MAX_T_WAKE.
 * This is about 2.4ms for a 6 IC daisy chain and 4.8ms for a 12 IC daisy chain
 */
void wake_up_LTC6811_from_SLEEP() {
	uint16_t i;

//    for(i = 0; i < daisy_chain->n; i++){
//        daisy_chain->enable_comm();
//        daisy_chain->disable_comm();
//        __delay_us(MAX_T_WAKE);
//    }
//    daisy_chain->reset_interface_watchdog();
//    daisy_chain->core_state = CORE_STANDBY_STATE;

// Pulse SS line
	start_comm_with_LTC6811();
	//	HAL_Delay(1);
	end_comm_with_LTC6811();

	return;
}

/**
 * @brief Wakes up an entire daisy chain from isoSPI interface IDLE state
 *
 * @warning
 * The function is blocking and takes daisy_chain_n * MAX_T_READY.
 * This is about 60us for a 6 IC daisy chain and 120us for a 12 IC daisy chain
 */
void wake_up_LTC6811_from_IDLE() {
//    uint16_t i;
//    for(i = 0; i < daisy_chain->n; i++){
//        daisy_chain->enable_comm();
//        daisy_chain->disable_comm();
//        __delay_us(MAX_T_READY);
//    }
//    daisy_chain->reset_interface_watchdog();
//    daisy_chain->interface_state = ISOSPI_READY_STATE;

	start_comm_with_LTC6811();
//	HAL_Delay(1);
	end_comm_with_LTC6811();

	return;
}

/**
 * @brief Ensures the daisy_chain is waken and enables SS
 */
void start_comm_with_LTC6811() {

	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin,
			GPIO_PIN_RESET);

	return;
}

/**
 * @brief Disables SS
 */
void end_comm_with_LTC6811() {

	HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_SET);

	return;
}

/**
 * @brief Broadcasts a poll command
 *
 * @param [in]  daisy_chain LTC6811 daisy chain to broadcast the command
 * @param [in]  command     Command to broadcast
 */
int broadcast_poll(uint16_t command) {

	uint8_t message[4];
	uint16_t PEC;

	message[0] = command >> 8;
	message[1] = command;

	PEC = PEC_calculate(message, 2);
	message[2] = PEC >> 8;
	message[3] = PEC;

	start_comm_with_LTC6811();
	HAL_SPI_Transmit(LTC6811_SPI, message, 4, HAL_MAX_DELAY);

	end_comm_with_LTC6811();

	return 0;
}

/**
 * @brief Writes size*daisy_chain.n bytes from the location pointed by data and
 * sends them to the daisy_chain using command.
 *
 * @param   [in]    daisy_chain LTC6811 daisy chain to write to.
 * @param   [in]    command     Command to use to write
 * @param   [in]    size        Number of bytes to write to each LTC6811
 * @param   [in]    data        Location where the data to write is
 */
void broadcast_write(uint16_t command, uint16_t size, uint8_t *data) {

	uint16_t PEC;
	uint8_t message[4 + 6 + 2];

	message[0] = command >> 8;
	message[1] = command;
	PEC = PEC_calculate(message, 2);
	message[2] = PEC >> 8;
	message[3] = PEC;

	memcpy(&message[4], data, 6);

	PEC = PEC_calculate(data, size);
	message[4 + 6 + 0] = PEC >> 8;
	message[4 + 6 + 1] = PEC;

	start_comm_with_LTC6811();

	HAL_SPI_Transmit(LTC6811_SPI, message, 4 + size + 2, HAL_MAX_DELAY);

	end_comm_with_LTC6811();

	return;
}

/**
 * @brief Reads size*daisy_chain.n bytes from the daisy_chain using command and
 * stores them in the location pointed by data
 *
 * @param   [in]    daisy_chain LTC6811 daisy chain from where to read.
 * @param   [in]    command     Command to use to read
 * @param   [in]    size        Number of bytes to read from each LTC6811
 * @param   [out]   data        Location to store the read data
 */
int broadcast_read(uint16_t command, uint16_t size, uint8_t *data) {

	uint8_t command_message[4];
	uint8_t data_PEC[2];
	uint16_t PEC;
	uint16_t command_PEC;

	command_message[0] = command >> 8;
	command_message[1] = command;
	command_PEC = PEC_calculate(command_message, 2);
	command_message[2] = command_PEC >> 8;
	command_message[3] = command_PEC;

	start_comm_with_LTC6811();

	// Send command
	HAL_SPI_Transmit(LTC6811_SPI, command_message, 4, HAL_MAX_DELAY);
//	HAL_Delay(1);

	// Receive data
	HAL_SPI_Receive(LTC6811_SPI, data, size, HAL_MAX_DELAY);

	// Receive PEC
	HAL_SPI_Receive(LTC6811_SPI, data_PEC, 2, HAL_MAX_DELAY);

	end_comm_with_LTC6811();

	// Verify PEC
	PEC = data_PEC[0] << 8 | data_PEC[1];
	if (PEC_verify(data, size, PEC) < 0) {
		return -1;
	}

	return 0;
}

// PEC
uint16_t PEC_calculate(uint8_t *data, int len) {
	uint16_t remainder, address;
	remainder = 16;/*PEC seed*/
	int i;
	for (i = 0; i < len; i++) {
		address = ((remainder >> 7) ^ data[i]) & 0xff;/*calculate PEC table address*/
		remainder = (remainder << 8) ^ crc15Table_NEW[address];
	}
	return ((remainder * 2) & 0xffff);/*The CRC15 has a 0 in the LSB so the final value must be multiplied by 2*/
}

/**********************************************************************
 * Name:	PEC_verify
 * Args:    uint8_t data, uint16_t n, uint8_t PEC
 * Return:	exit status
 * Desc:	Verifies if PEC is correct for given data.
 **********************************************************************/
int PEC_verify(uint8_t *data, uint16_t n, uint16_t PEC) {

	if (PEC_calculate(data, n) == PEC) {
		return 0;
	}
	return -1;
}

