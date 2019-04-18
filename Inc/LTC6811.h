/*
 * LTC6811.h
 *
 *  Created on: Apr 18, 2019
 *      Author: matth
 */

#ifndef LTC6811_H_
#define LTC6811_H_

#include "bms.h"
#include "vstack.h"

void LTC681x_adax(uint8_t MD, //ADC Mode
    uint8_t CHG //GPIO Channels to be measured)
    );
void LTC681x_adstat(uint8_t MD, //ADC Mode
    uint8_t CHST //GPIO Channels to be measured
    );
void LTC681x_adstatd(uint8_t MD, //ADC Mode
    uint8_t CHST //GPIO Channels to be measured
    );
void LTC681x_adow(uint8_t MD, //ADC Mode
    uint8_t PUP, //PULL UP OR DOWN current
    uint8_t CH, //Channels
    uint8_t DCP //Discharge Permit
    );
void LTC681x_adcvsc(uint8_t MD, //ADC Mode
    uint8_t DCP //Discharge Permit
    );
void LTC681x_adcvax(uint8_t MD, //ADC Mode
    uint8_t DCP //Discharge Permit
    );
void LTC681x_adol(uint8_t MD, //ADC Mode
    uint8_t DCP //Discharge Permit
    );
uint8_t LTC681x_pladc();
uint32_t LTC681x_pollAdc();
HAL_StatusTypeDef LTC6811_init();
HAL_StatusTypeDef LTC6811_addrRead(uint8_t *dout, uint8_t len, uint16_t cmd);
HAL_StatusTypeDef LTC6811_addrWrite(uint8_t *din, uint8_t len, uint16_t cmd);
uint16_t LTC6811Pec(uint8_t *data, uint8_t len);
uint16_t pec15_calc(uint8_t len, //Number of bytes that will be used to calculate a PEC
    uint8_t *data //Array of data that will be used to calculate  a PEC
    );
void wakeup_idle(uint8_t total_ic);
void LTC681x_wrsctrl(uint8_t sctrl_reg, uint8_t tx_data[]);
void LTC681x_rdcv_reg(uint8_t reg, //Determines which cell voltage register is read back
    uint8_t total_ic, //the number of ICs in the
    uint8_t *data //An array of the unparsed cell codes
    );
void LTC681x_adcv(uint8_t MD, //ADC Mode
    uint8_t DCP, //Discharge Permit
    uint8_t CH //Cell Channels to be measured
    );
int8_t read_68(uint8_t total_ic, uint8_t tx_cmd[2], uint8_t *rx_data);
void write_68(uint8_t total_ic, uint8_t tx_cmd[2], uint8_t data[]);
void cmd_68(uint8_t tx_cmd[2]);
void wakeup_sleep(uint8_t total_ic);


#endif /* LTC6811_H_ */
