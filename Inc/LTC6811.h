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

//Defines
#define LTC8584_SHIGH          0 //part is disabled voltage of cells is transmitted
#define LTC8584_SLOW           8 //enable balancing in simple mode
#define LTC8584_2PULSE         1 //part is enabled for balancing voltage of cells is transmitted
#define LTC8584_3PULSE         2
#define LTC8584_4PULSE         3
#define LTC8584_5PULSE         4
#define LTC8584_6PULSE         5
#define LTC8584_7PULSE         6
#define LTC8584_8PULSE         7

#define LTC6811_MD_00 0 //ADCOPT = 0 => 422 Hz else 1kHz
#define LTC6811_MD_01 1 //ADCOPT = 0 => 27kHz (fast) else 14kHz
#define LTC6811_MD_10 2 //ADCOPT = 0 => 7kHz (Normal) else 3kHz
#define LTC6811_MD_11 3 //ADCOPT = 0 => 26 Hz (Filtered) else 2kHz

#define DISCHARGE_PERMITTED     1
#define DISCHARGE_NOT_PERMITTED 0

//ADC Channel definition
#define LTC6811_ADC_CALL  0x0 //all cells
#define LTC6811_ADC_C17   0x1 //cells 1 and 7
#define LTC6811_ADC_C28   0x2 // 2 and 8
#define LTC6811_ADC_C39   0x3 // 3 and 9
#define LTC6811_ADC_C410  0x4 // 4 and 10
#define LTC6811_ADC_C511  0x5 // 5 and 11
#define LTC6811_ADC_C612  0x6 // 6 and 12

#define LTC6811_GPIO1		0x01
#define LTC6811_GPIO2		0x02
#define LTC6811_GPIO3		0x04
#define LTC6811_GPIO4		0x08
#define LTC6811_GPIO5		0x10

typedef enum {
  RDCVA = 1,
  RDCVB = 2,
  RDCVC = 3,
  RDCVD = 4,
  RDCVE = 5,
  RDCVF = 6
}cell_groups_t;

//Function Prototypes
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
void LTC6811_wgpio(uint8_t gpiox);
uint8_t LTC681x_pladc();
uint32_t LTC681x_pollAdc();
HAL_StatusTypeDef LTC6811_init();
HAL_StatusTypeDef LTC6811_addrRead(uint8_t *dout, uint8_t len, uint16_t cmd);
HAL_StatusTypeDef LTC6811_addrWrite(uint8_t *din, uint8_t len, uint16_t cmd);
uint16_t LTC6811Pec(uint8_t *data, uint8_t len);
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
HAL_StatusTypeDef LTC6811_broadCMD(uint16_t cmd);
HAL_StatusTypeDef LTC6811_broadRead(uint8_t *dout,
		uint8_t len, uint16_t cmd);
void wakeup_sleep(uint8_t total_ic);
void LTC681x_clrsctrl();

#endif /* LTC6811_H_ */
