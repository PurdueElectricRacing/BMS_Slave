/* Include guard */
#ifndef _LTC6811_
#define _LTC6811_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "bms.h"
#include "stm32l4xx_hal_spi.h"

/**
 * @name LTC6811 timings
 * @{
 */
#define TYP_T_WAKE  200 /**< [us] */
#define MAX_T_WAKE  400 /**< [us] */

#define MIN_T_SLEEP 1800000 /**< [us] */
#define TYP_T_SLEEP 2000000 /**< [us] */
#define MAX_T_SLEEP 2200000 /**< [us] */

#define MIN_T_REFUP 2700    /**< [us] */
#define TYP_T_REFUP 3500    /**< [us] */
#define MAX_T_REFUP 4400    /**< [us] */

#define MAX_T_READY 10  /**< [us] */

#define MIN_T_IDLE  4300    /**< [us] */
#define TYP_T_IDLE  5500    /**< [us] */
#define MAX_T_IDLE  6700    /**< [us] */

/* Conversion times */
#define T_CONV_CELL_ALL_7K 2300    /**< [us] */
#define T_CONV_CELL_ONE_7K 405     /**< [us] */
#define T_CONV_GPIO_ALL_PLUS_REFERENCE_7K 2300  /** [us] */
#define T_CONV_GPIO_ONE_7K  405 /** [us] */
#define T_CONV_STATUS_ALL_7K 1600   /** [us] */
#define T_CONV_STATUS_ONE_7K 405    /** [us] */

/*@}*/

/**
 * @name LTC6811 commands
 * @{
 */
#define WRCFGA      0x0001  /** Write to Configuration Register (CFGR) */
#define RDCFGA      0x0002  /** Read the Configuration Register (CFGR) */
#define RDCVA       0x0004  /** Read the Cell Voltage Register A (CVAR) */
#define RDCVB       0x0006  /** Read the Cell Voltage Register B (CVBR) */
#define RDCVC       0x0008  /** Read the Cell Voltage Register C (CBCR) */
#define RDCVD       0x000A  /** Read the Cell Voltage Register D (CBDR) */
#define RDAUXA      0x000C  /** Read the Auxiliary Register Group A (AVAR) */
#define RDAUXB      0x000E  /** Read the Auxiliary Register Group B (AVBR) */
#define RDSTATA     0x0010  /** Read the Status Register Group A (STAR) */
#define RDSTATB     0x0012  /** Read the Status Register Group B (STBR) */
#define WRSCTRL     0x0014  /** Write to the S Control Register Group (SCTRL) */
#define WRPWM       0x0020  /** Write to the PWM Register Group (PWMR) */
#define RDSCTRL     0x0016  /** Read the S Control Register Group (SCTRL) */
#define RDPWM       0x0022  /** Read the PWM Register Group (PWMR) */
#define STSCTRL     0x0019  /** Start S Control pulsing and poll status */
#define CLRSCTRL    0x0018  /** Clear S Control register */
/**
 * @brief Start Cell Voltage ADC Conversion and Poll Status
 */
#define ADCV(MD,DCP,CH)     (0x260 | (MD<<7) | (DCP<<4) | (CH))
/**
 * @brief Start Open Wire ADC Conversion and Poll Status
 */
#define ADOW(MD,PUP,DCP,CH) (0x228 | (MD<<7) | (PUP<<6) | (DCP<<4) | (CH))
/**
 * @brief Start Self Test Cell Voltage Conversion and Poll Status
 */
#define CVST(MD,ST)         (0x207 | (MD<<7) | (ST<<5))
/**
 * @brief Start Overlap Measurement of Cell 7 Voltage
 */
#define ADOL(MD,DCP)        (0x201 | (MD<<7) | (DCP<<4))
/**
 * @brief Start GPIOs ADC Conversion and Poll Status
 */
#define ADAX(MD,CHG)        (0x460 | (MD<<7) | (CHG))
/**
 * @brief Start GPIOs ADC Conversion With Digital Redundancy and Poll Status
 */
#define ADAXD(MD,CHG)       (0x400 | (MD<<7) | (CHG))
/**
 * @brief Start Self Test GPIOs Conversion and Poll Status
 */
#define AXST(MD,ST)         (0x407 | (MD<<7) | (ST<<5))
/**
 * @brief Start Status Group ADC Conversion and Poll Status
 */
#define ADSTAT(MD,CHST)     (0x468 | (MD<<7) | (CHST))
/**
 * @brief Start Status Group ADC Conversion With Digital Redundancy and Poll
 * Status
 */
#define ADSTATD(MD,CHST)    (0x408 | (MD<<7) | (CHST))
/**
 * @brief Start Self Test Status Group Conversion and Poll Status
 */
#define STATS(MD,ST)        (0x40F | (MD<<7) | (ST<<5))
/**
 * @brief Start Combined Cell Voltage and GPIO1, GPIO2 Conversion and Poll
 * Status
 */
#define ADCVAX(MD,DCP)      (0x46F | (MD<<7) | (DCP<<4))
/**
 * @brief Start Combined Cell Voltage and SC Conversion and Poll Status
 */
#define ADCVSC(MD,DCP)      (0x467 | (MD<<7) | (DCP<<4))
#define CLRCELL     0x0711 /** Clear Cell Voltage Register Groups */
#define CLRAUX      0x0712 /** Clear Auxiliary Register Groups */
#define CLRSTAT     0x0713 /** Clear Status Register Groups */
#define PLADC       0x0714 /** Poll ADC Conversion Status */
#define DIAGN       0x0715 /** Diagnose MUX and Poll Status */
#define WRCOMM      0x0721 /** Write COMM Register Group */
#define RDCOMM      0x0722 /** Read COMM Register Group */
#define STCOMM      0x0723 /** Start I2C /SPI Communication */
/*@}*/

/**
 * @name LTC6811 command options
 * @{
 */

enum LTC6811_COMMAND_MD{
    MD_MODE_0 = 0,
    MD_MODE_1 = 1,
    MD_FAST = 1,
    MD_MODE_2 = 2,
    MD_NORMAL = 2,
    MD_MODE_3 = 3,
    MD_FILTERED = 3,
};

enum LTC6811_COMMAND_DCP{
    DCP_DISCHARGE_NOT_PERMITTED = 0,
    DCP_DISCHARGE_PERMITTED = 1,
};

enum LTC6811_COMMAND_CH{
    CH_ALL_CELLS = 0,
    CH_CELLS_1_7 = 1,
    CH_CELLS_2_8 = 2,
    CH_CELLS_3_9 = 3,
    CH_CELLS_4_10 = 4,
    CH_CELLS_5_11 = 5,
    CH_CELLS_6_12 = 6,
};

enum LTC6811_COMMAND_PUP{
    PUP_PULL_DOWN = 0,
    PUP_PULL_UP = 1,
};

enum LTC6811_COMMAND_ST{
    ST_SELF_TEST1 = 1,
    ST_SELF_TEST2 = 2,
};

enum LTC6811_COMMAND_CHG{
    CHG_ALL = 0,
    CHG_GPIO1 = 1,
    CHG_GPIO2 = 2,
    CHG_GPIO3 = 3,
    CHG_GPIO4 = 4,
    CHG_GPIO5 = 5,
    CHG_2ND_REFERENCE = 6,
};

enum LTC6811_COMMAND_CHST{
    CHST_SC_ITMP_VA_VD = 0,
    CHST_SC = 1,
    CHST_ITMP = 2,
    CHST_VA = 3,
    CHST_VD = 4,
};
/**@}*/

/**
 * @name LTC6811 States
 * @{
 */
typedef enum LTC6811_core_state_enum{
    CORE_SLEEP_STATE,
    CORE_STANDBY_STATE,
    CORE_REFUP_STATE,
    CORE_MEASURE_STATE,
} LTC6811_Core_State;

typedef enum LTC6811_isoSPI_state_enum{
    ISOSPI_IDLE_STATE,
    ISOSPI_READY_STATE,
    ISOSPI_ACTIVE_STATE,
} LTC6811_isoSPI_State;
/**@}*/


/**
 * @brief Structure containing data needed for LTC6811 daisy chain communication
 *
 * One of this structures must be declared for each isoSPI daisy chain
 * connected.
 *
 * Pointers to enable_comm and disable_comm functions should be given to each
 * declared struct so that the slave select pins can be used.
 */
typedef struct LTC6811_daisy_chain{
    uint16_t n; /**< Number of LTC6811 ICs on the daisy chain */

    LTC6811_Core_State core_state;  /**< Current core state */
    LTC6811_isoSPI_State interface_state;   /**< Current isoSPI interface state */
    /**
     * @brief Function to enable (SS) communication with the LTC6811 daisy chain
     *
     * This usually involves pulling the Slave Select (SS) pin LOW.
     */
    void (*enable_comm)(void);
    /**
     * @brief Function to disable (SS) communication with the LTC6811 daisy chain
     *
     * This usually involves pulling the Slave Select (SS) pin HIGH.
     */
    void (*disable_comm)(void);

    /**
     * @brief Function to reset the watchdog monitoring when the isoSPI
     * interface goes to IDLE state or the core goes to SLEEP state.
     */
    void (*reset_interface_watchdog)(void);
} LTC6811_daisy_chain;


#define LTC6811_REG_SIZE    6 /**< LTC6811 register size in bytes */

/**
 * @brief Structure describing the LTC6811 register contents
 */
typedef struct LTC6811_struct{
    /**
     * @brief Contents of Configuration Register Group (CFGR)
     */
    union{
        struct{
            uint8_t ADCOPT : 1;
            uint8_t DTEN   : 1;
            uint8_t REFON  : 1;
            uint8_t GPIO    : 5;
            uint32_t : 24;
            uint16_t DCC    : 12;
            uint8_t DCTO    : 4;
        };
        uint8_t CFGR[6];
    };

    /**
     * @brief Contents of Cell Voltage Register Group A (CVAR)
     */
    union{
        struct{
            uint16_t C1V;
            uint16_t C2V;
            uint16_t C3V;
        };
        uint8_t CVAR[6];
    };
    /**
     * @brief Contents of Cell Voltage Register Group B (CVBR)
     */
    union{
        struct{
            uint16_t C4V;
            uint16_t C5V;
            uint16_t C6V;
        };
        uint8_t CVBR[6];
    };
    /**
     * @brief Contents of Cell Voltage Register Group C (CVCR)
     */
    union{
        struct{
            uint16_t C7V;
            uint16_t C8V;
            uint16_t C9V;
        };
        uint8_t CVCR[6];
    };
    /**
     * @brief Contents of Cell Voltage Register Group D (CVDR)
     */
    union{
        struct{
            uint16_t C10V;
            uint16_t C11V;
            uint16_t C12V;
        };
        uint8_t CVDR[6];
    };

    /**
     * @brief Contents of Auxiliary Register Group A (AVAR)
     */
    union{
        struct{
            uint16_t G1V;
            uint16_t G2V;
            uint16_t G3V;
        };
        uint8_t AVAR[6];
    };
    /**
     * @brief Contents of Auxiliary Register Group B (AVBR)
     */
    union{
        struct{
            uint16_t G4V;
            uint16_t G5V;
            uint16_t REF;
        };
        uint8_t AVBR[6];
    };

    /**
     * @brief Contents of Status Register Group A (STAR)
     */
    union{
        struct{
            uint16_t SC;
            uint16_t ITMP;
            uint16_t VA;
        };
        uint8_t STAR[6];
    };
    /**
     * @brief Contents of Status Register Group B (STBR)
     */
    union{
        struct{
            uint16_t VD;
            uint32_t CUOV   : 24; /**< CxUV and CxOV bits interleaved */
            bool THSD   : 1;
            bool MUXFAIL    : 1;
            uint8_t RSVD    : 2;
            uint8_t REV : 4;
        };
        uint8_t STBR[6];
    };

    /**
     * @brief Array for COMM Register Group
     *
     * This register isn't used and its contents are very strange
     */
    uint8_t COMM[6];

    /**
     * @brief S Control Register Group (SCTRL)
     */
    union{
        struct{
            uint8_t SCTL1   : 4;
            uint8_t SCTL2   : 4;
            uint8_t SCTL3   : 4;
            uint8_t SCTL5   : 4;
            uint8_t SCTL4   : 4;
            uint8_t SCTL6   : 4;
            uint8_t SCTL7   : 4;
            uint8_t SCTL8   : 4;
            uint8_t SCTL9   : 4;
            uint8_t SCTL10  : 4;
            uint8_t SCTL11  : 4;
            uint8_t SCTL12  : 4;
        };
        uint8_t SCTRL[6];
    };

    /**
     * @brief PWM Register Group (PWMR)
     */
    union{
        struct{
            uint8_t PWM1   : 4;
            uint8_t PWM2   : 4;
            uint8_t PWM3   : 4;
            uint8_t PWM5   : 4;
            uint8_t PWM4   : 4;
            uint8_t PWM6   : 4;
            uint8_t PWM7   : 4;
            uint8_t PWM8   : 4;
            uint8_t PWM9   : 4;
            uint8_t PWM10  : 4;
            uint8_t PWM11  : 4;
            uint8_t PWM12  : 4;
        };
        uint8_t PWMR[6];
    };

} LTC6811;


/**
 * @brief Initializes a LTC6811 struct
 */
void LTC6811_init(LTC6811* ltc);

/**
 * @brief Wakes up an entire daisy chain from core SLEEP state
 *
 * @warning
 * The function is blocking and takes daisy_chain_n * MAX_T_WAKE.
 * This is about 2.4ms for a 6 IC daisy chain and 4.8ms for a 12 IC daisy chain
 */
void wake_up_LTC6811_from_SLEEP();
/**
 * @brief Wakes up an entire daisy chain from isoSPI interface IDLE state
 *
 * @warning
 * The function is blocking and takes daisy_chain_n * MAX_T_READY.
 * This is about 60us for a 6 IC daisy chain and 120us for a 12 IC daisy chain
 */
void wake_up_LTC6811_from_IDLE();

/**
 * @brief Broadcasts a poll command
 *
 * @param [in]  daisy_chain LTC6811 daisy chain to broadcast the command
 * @param [in]  command     Command to broadcast
 */
int broadcast_poll(uint16_t command);

/**
 * @brief Writes size*daisy_chain.n bytes from the location pointed by data and
 * sends them to the daisy_chain using command.
 *
 * @param   [in]    daisy_chain LTC6811 daisy chain to write to.
 * @param   [in]    command     Command to use to write
 * @param   [in]    size        Number of bytes to write to each LTC6811
 * @param   [in]    data        Location where the data to write is
 */
void broadcast_write(
		uint16_t command,
		uint16_t size,
		uint8_t *data);

/**
 * @brief Reads size*daisy_chain.n bytes from the daisy_chain using command and
 * stores them in the location pointed by data
 *
 * @param   [in]    daisy_chain LTC6811 daisy chain from where to read.
 * @param   [in]    command     Command to use to read
 * @param   [in]    size        Number of bytes to read from each LTC6811
 * @param   [out]   data        Location to store the read data
 */
int broadcast_read(
        uint16_t command,
        uint16_t size,
        uint8_t *data);

uint16_t PEC_calculate(uint8_t *data , int len);
int PEC_verify(uint8_t *data, uint16_t n, uint16_t PEC);

static const uint16_t crc15Table_NEW[256] = {0x0,0xc599, 0xceab, 0xb32, 0xd8cf, 0x1d56, 0x1664, 0xd3fd, 0xf407, 0x319e,
0x3aac, 0xff35, 0x2cc8, 0xe951, 0xe263, 0x27fa, 0xad97, 0x680e, 0x633c, 0xa6a5, 0x7558, 0xb0c1, 0xbbf3, 0x7e6a, 0x5990,
0x9c09, 0x973b, 0x52a2, 0x815f, 0x44c6, 0x4ff4, 0x8a6d, 0x5b2e,0x9eb7, 0x9585, 0x501c, 0x83e1, 0x4678, 0x4d4a, 0x88d3,
0xaf29, 0x6ab0, 0x6182, 0xa41b, 0x77e6, 0xb27f, 0xb94d, 0x7cd4, 0xf6b9, 0x3320, 0x3812, 0xfd8b, 0x2e76, 0xebef, 0xe0dd,
0x2544, 0x2be,  0xc727, 0xcc15, 0x98c,  0xda71, 0x1fe8, 0x14da, 0xd143, 0xf3c5, 0x365c, 0x3d6e, 0xf8f7, 0x2b0a, 0xee93,
0xe5a1, 0x2038, 0x7c2,  0xc25b, 0xc969, 0xcf0,  0xdf0d, 0x1a94, 0x11a6, 0xd43f, 0x5e52, 0x9bcb, 0x90f9, 0x5560, 0x869d,
0x4304, 0x4836, 0x8daf, 0xaa55, 0x6fcc, 0x64fe, 0xa167, 0x729a, 0xb703, 0xbc31, 0x79a8, 0xa8eb, 0x6d72, 0x6640, 0xa3d9,
0x7024, 0xb5bd, 0xbe8f, 0x7b16, 0x5cec, 0x9975, 0x9247, 0x57de, 0x8423, 0x41ba, 0x4a88, 0x8f11, 0x57c,  0xc0e5, 0xcbd7,
0xe4e,  0xddb3, 0x182a, 0x1318, 0xd681, 0xf17b, 0x34e2, 0x3fd0, 0xfa49, 0x29b4, 0xec2d, 0xe71f, 0x2286, 0xa213, 0x678a,
0x6cb8, 0xa921, 0x7adc, 0xbf45, 0xb477, 0x71ee, 0x5614, 0x938d, 0x98bf, 0x5d26, 0x8edb, 0x4b42, 0x4070, 0x85e9, 0xf84,
0xca1d, 0xc12f, 0x4b6,  0xd74b, 0x12d2, 0x19e0, 0xdc79, 0xfb83, 0x3e1a, 0x3528, 0xf0b1, 0x234c, 0xe6d5, 0xede7, 0x287e,
0xf93d, 0x3ca4, 0x3796, 0xf20f, 0x21f2, 0xe46b, 0xef59, 0x2ac0, 0xd3a,  0xc8a3, 0xc391, 0x608,  0xd5f5, 0x106c, 0x1b5e,
0xdec7, 0x54aa, 0x9133, 0x9a01, 0x5f98, 0x8c65, 0x49fc, 0x42ce, 0x8757, 0xa0ad, 0x6534, 0x6e06, 0xab9f, 0x7862, 0xbdfb,
0xb6c9, 0x7350, 0x51d6, 0x944f, 0x9f7d, 0x5ae4, 0x8919, 0x4c80, 0x47b2, 0x822b, 0xa5d1, 0x6048, 0x6b7a, 0xaee3, 0x7d1e,
0xb887, 0xb3b5, 0x762c, 0xfc41, 0x39d8, 0x32ea, 0xf773, 0x248e, 0xe117, 0xea25, 0x2fbc, 0x846,  0xcddf, 0xc6ed, 0x374,
0xd089, 0x1510, 0x1e22, 0xdbbb, 0xaf8,  0xcf61, 0xc453, 0x1ca,  0xd237, 0x17ae, 0x1c9c, 0xd905, 0xfeff, 0x3b66, 0x3054,
0xf5cd, 0x2630, 0xe3a9, 0xe89b, 0x2d02, 0xa76f, 0x62f6, 0x69c4, 0xac5d, 0x7fa0, 0xba39, 0xb10b, 0x7492, 0x5368, 0x96f1,
0x9dc3, 0x585a, 0x8ba7, 0x4e3e, 0x450c, 0x8095};


#endif /* End include guard */
