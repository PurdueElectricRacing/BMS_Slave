#ifndef FAULTS_H
#define FAULTS_H

#include "main.h"
#include "string.h"
#include "FreeRTOS.h"

// Enumerations
enum {
	LOW_VOLTAGE,	// 0 - Undervoltage fault
	HIGH_VOLTAGE,	// 1 - Overvoltage fault
	HIGH_CURRENT,	// 2 - Overcurrent fault
	OPEN_WIRE,		// 3 - Open wire detected
	LOW_SOC,		// 4 - Under SOC condition
	HIGH_SOC,		// 5 - Over SOC condition
	COMM_FAIL,		// 6 - Master/slave comm failure
	AFE_FAIL,		// 7 - AFE comm/other failure
	PERM_FAIL,		// 8 - Permission mode failure
	HIGH_TEMP,		// 9 - Over temperature condition
	FAULT_COUNT		// FAULT_COUNT must come last!
} faults;

typedef enum {
	FALSE,
	TRUE
} status_t;

// Fault Settings
const uint16_t 	latch[FAULT_COUNT]          = {0, 0, 0, 0, 0, 0, 3000, 3000, 3000, 0};			// Fault latch times in [16.0] ms
const uint16_t 	unlatch[FAULT_COUNT]        = {10000, 10000, 10000, 10000, 0, 0, 0, 0, 0, 10};	// Fault unlatch times in [16.0] ms
const uint8_t 	fault_active[FAULT_COUNT]   = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};					// Fault active (0/1 -> TRUE/FALSE)
const uint8_t	fault_critical[FAULT_COUNT] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};					// Fault critical (0/1 -> TRUE/FALSE)

// Struct(s)
typedef struct {
	uint32_t	fault_activity;						// Bit flag marking fault as active
	uint32_t	fault_set;							// Bit flag for setting/unsetting
	uint32_t	fault_historic;						// Bit flag marking fault as historic
	uint16_t	fault_time[FAULT_COUNT];			// Current fault time in [16.0] ms
} faults_t;

extern faults_t fault_lib;

// Macros
#define bitExtract(data, idx) ((data >> idx) & 0x1)	// Pulls a single bit flag from a bit field

#endif
