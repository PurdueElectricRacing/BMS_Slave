/*
 * LTC6811.c
 *
 *  Created on: Apr 18, 2019
 *      Author: matth
 */

#include "LTC6811.h"

//Generic wakeup command to wake the LTC681x from sleep
void wakeup_sleep(uint8_t total_ic) {
  for (int i = 0; i < total_ic; i++) {
    HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin,
        GPIO_PIN_RESET);
    vTaskDelay(1);  // Guarantees the LTC681x will be in standby
    HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_SET);
    vTaskDelay(1);
  }
}

//Wake SPI up from idle state
void wakeup_idle(uint8_t total_ic) {
  uint8_t tx_arr[1] = 0xFF;
  for (int i = 0; i < total_ic; i++) {
    HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin,
        GPIO_PIN_RESET);
    HAL_SPI_Transmit(LTC6811_SPI, tx_arr, 1, HAL_MAX_DELAY); //Guarantees the isoSPI will be in ready mode
    HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_SET);
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
      { //The first configuration written is received by the last IC in the daisy chain
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
  HAL_SPI_Receive(LTC6811_SPI, data, (BYTES_IN_REG * total_ic), HAL_MAX_DELAY); //Read the configuration data of all ICs on the daisy chain into rx_data[] array
  HAL_GPIO_WritePin(VSTACK_SPI_SS_GPIO_Port, VSTACK_SPI_SS_Pin, GPIO_PIN_SET);

  for (uint8_t current_ic = 0; current_ic < total_ic; current_ic++) //executes for each LTC681x in the daisy chain and packs the data
      { //into the r_comm array as well as check the received data for any bit errors
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


//Start a GPIO and Vref2 Conversion Not required
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

//Start a Status ADC Conversion Not required
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

// Start a Status register redundancy test Conversion Not required
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

// Start an open wire Conversion Not required
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

//Starts cell voltage and SOC conversion Not required
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

// Starts cell voltage  and GPIO 1&2 conversion Not required
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

//Starts cell voltage overlap conversion Not required
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

//Sends the poll adc command Not required
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

//This function will block operation until the ADC has finished it's conversion Not required
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

