#include "faults.h"

// @funcname: initFaults()
//
// @brief: Loads historic faults and asserts faults inactive
//
// @exec_period: Arbitrary
//
// @return: none
void initFaults()
{
	// TODO: Load historic faults
	memset(&fault_lib.fault_time, 0, sizeof(fault_lib.fault_time));			// Assert each timing to 0 prior to start
	memset(&fault_lib.fault_activity, 0, sizeof(fault_lib.fault_activity));	// Assert faults to un-faulted state
	memset(&fault_lib.fault_set, 0, sizeof(fault_lib.fault_set));			// Assert faults to un-faulted state
}

// @funcname: setFault()
//
// @brief: Updates the status of a fault
//
// @exec_period: Arbitrary
//
// @param idx: Index of the fault to set
// @param status: Boolean
//
// @return: none
void setFault(uint8_t idx, status_t status)
{
	if (idx <= FAULT_COUNT)											// Check if the given fault index is valid
	{
		fault_lib.fault_set &= ~(1 << idx);							// Reset the bit corresponding to the given index
		fault_lib.fault_set |= status << idx;						// Shift in the new status
	}
}

// @funcname: processFaults()
//
// @brief: Updates timing and status for each fault based on the previous updates
//
// @exec_period: Arbitrary
//
// @return: none
void processFaults()
{
	// Locals
	uint8_t				i;											// Generic counter variable
	uint8_t				crit_count = 0;								// Number of critical faults encountered
	TickType_t			in_time;									// Start time of the loop in ms
	static TickType_t	old_time;									// Start time of last loop in ms

	in_time = HAL_GetTick();										// Get current time since boot in ms

	for (i = 0; i <= FAULT_COUNT; ++i)								// Loop through each fault
	{
		if (bitExtract(fault_lib.fault_set, i) && fault_active[i])	// Check the status of the fault (and don't fault an inactive fault)
		{
			if (fault_lib.fault_time[i] <= latch[i])				// Fault is active, so check if we have already hit latch time
			{
				fault_lib.fault_time[i] += in_time - old_time;		// No, so add the time passed since last run
			}
			else													// Yes, so set fault
			{
				fault_lib.fault_activity |= (1 << i);				// Mark the fault as active
				fault_lib.fault_historic |= (1 << i);				// Mark the historic fault

				if (fault_critical[i])								// Check if the fault is critical
				{
					++crit_count;									// Disengage from the bus
				}
			}
		}
		else														// Fault is inactive, so walk back latch time
		{
			if (fault_lib.fault_time[i] >= 0)						// Check if the fault is already inactive
			{
				if ((in_time - old_time) > fault_lib.fault_time[i])	// Check if we'd wrap
				{
					fault_lib.fault_time[i] = 0;					// We would, so assert to 0
					fault_lib.fault_activity &= ~(1 << i);			// Assert the fault to off
				}
				else
				{
					fault_lib.fault_time[i] -= in_time - old_time;	// We wouldn't so just subtract
				}
			}
		}
	}

	if (crit_count)													// Check if we've developed any critical faults
	{
		// TODO: Disengage from the bus if we have a critical fault
	}
}

// @funcname: checkFault()
//
// @brief: Returns current state of a fault given its index
//
// @exec_period: Arbitrary
//
// @param idx: Index of the fault to check
//
// @return: Current cell fault status
status_t checkFault(uint8_t idx)
{
	if (idx <= FAULT_COUNT)
	{
		return (bitExtract(fault_lib.fault_activity, idx) & fault_active[idx]);
	}

	return 0;
}
