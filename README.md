# **BMS Slave**

## **Overview**

BMS slave is a portion of a distributed battery management system outlined [here](https://drive.google.com/drive/u/0/folders/10BhgTpdeEc__XZ9EmfNQqM7rGZ7nbBgL). The high level functionality of the board covers two main components.

- Gather voltage data from each cell in a battery module
- Relay information about each cell as well as any faults in the system to the master board of the system.

This is accomplished via the use of the `LTC6811-X` IC. This IC uses a multiplexer to take analog values of each cell's voltage in an efficient manner. Using this IC and an `STM32` microcontroller, the data required can be collected and sent to the master board for further processing. This document will outline the code written to facillitate the use of this IC, as well as describing the methods used in the code and why they are implemented on a higher level. Hopefully, this document will outline holes in the software while giving information about the processes running on the microcontroller.

## **Basic Structure**

The BMS slave `STM32` uses `FreeRTOS` to manage scheduling and execution of code. By doing this, the application can run multiple threads "simultaneously" allowing for easier development. Upon startup, a few tasks are spawned. In order, the functions started are:

```
task_txCan()
task_CanProcess()
task_bms_main()
task_heartbeat()
task_Master_WDawg()
task_VSTACK()
task_acquire_temp()
task_broadcast()
task_error_check()
```

Before diving into what each task is and how the operate, first, the BMS slave must initialize a few key components. `initRTOSObjects()` handles the setup and spawning of the main threads. However, `initBMSObject()` handles setup of initial variable states as well as binary semaphores. The semaphores are used to safeguard data during threaded operations. More information about how binary semaphores work can be found [here](https://www.geeksforgeeks.org/semaphores-in-process-synchronization/). After these two functions complete (along with some other basic initialization), `osKernelStart()` is called and `FreeRTOS` takes control. From this point on, control of the program should be considered psuedo-random as the exact state of the program is hard to detect. Essentially, the coming task list is not indicative of the order in which the tasks run.

## **Tasks**

### _**task_bms_main()**_

`task_bms_main`, as the name implies, is the main funcitoning task of the slave module. It is a [`finite state machine`](https://brilliant.org/wiki/finite-state-machines/) that determines the operating characteristics of the slave module. The states of slave are `low power`, `init`, `normal operation`, `error`, and `shutdown`. When the master module sends a `CAN` message commanding the slave module to enter low power mode, the lights of the board are disabled and processing effectively stops. In order for the slave to wake again, it must be woken via an interrupt of some form. Essentially, low power mode just waits for an interrupt and does nothing special. The initializatin state is used to verify a valid connection to the master module. Also, prior to entering the normal operation state, the debug lights are set to verify the current state of the board. Following init comes normal operation. This is the typical state for the slave module. While the task does nothing special inside it, the threads `FreeRTOS` runs sends required information over `CAN`. In the future, this state will be populated with required parameter calculations as well as `CAN broadcast` rather than simply sending messages all the time. The error mode is quite self explanatory. This mode is used to send a special `CAN` message with included information about faults in the battery module. The debug lights are also used to display the faulted state.

In its current implementation, this is all the main task does as most of the vital processes slave runs are activated by `osKernalStart()` rather than the state machine itself. Therefore, the main task serves to put the slave module into the sleep state at the appropriate time, as well as broadcast faults gathered by the rest of the module.

The code for this portion is quite simple. Therefore, no in depth analysis will be given.  State machines are easy to follow by nature as they progress in a (typically) logical pattern. The code for the main task can be found on line 211 of `bms.c` at the time of writing.

### _**task_VSTACK()**_

`task_VSTACK` is, quite possibly, the most imporatant task the slave module runs. This is due to the fact that the job of the task is to monitor cell voltages and raise error flags when a parameter is violated. It does this via the `SPI` communication protocol which communicates with an `LTC6811-2 BMIC`. The datasheet for this part can be found [here](https://www.analog.com/media/en/technical-documentation/data-sheets/68111fb.pdf). There are a few major things completed by this function. First, the funciton begins commmunication with the BMIC by bringing the chip out of standby state. Then, it writes the configuration registers for over/under-volt testing by sending the chip a `WRCFGA` command. This is done through the `init_LTC6811()` function. Following the initialization period, the slave module and the BMIC enter a loop of back and forth communication. First, the slave module sends a broadcast command teslling the BIMC to poll cells 1 and 7 for conversion. This is done to ensure that the multiplexer on the BMIC has time to settle prior to reading all cells. After waiting for a specified amount of time (given in the datasheet), the BMICs are instructed to measure all cells using the `ADCV` command. This command is given in broadcast mode so that both BMICs on the board will do complete the requested polling. Following the cell polling, the cell block registers are read one by one to gather cell voltages. This command is issued in address mode as each block needs to be read individually. Following the reading of the cells, diagnostics are performed every 10 cycles. The diagnostics are run using both the `RDSTATB` and `ADOW` commands. The `RDSTATB` requests the values in the status b register group. In this group are values telling whether or not each cell has over/under-volted during operation. Also, the group contains a value telling whether or not the multiplexer has faulted. Using these two sets of values, most faults with the BMS slave can be determined. The `ADOW` command is used to determine if there is an open wire anywhere in the system. If there is, the voltages should be marked as unknown, and an error must occur. In essence, `task_VSTACK` simply requests voltages from the BMIC, stores those voltages, and checks for faults in the system. All other processing (such as sending over CAN and the actual generation of fault flags) is done elsewhere.

### _**task_txCan()**_

`task_txCan` is the task responsible for putting messages on the `CAN bus` for the master module to read and process.

> Note: `task_txCan()` **DOES NOT** package messages. The messages are packaged in their respective tasks.

The first thing that the task does is declare a CAN message typedef. This will allow the information to be copied into a header message for sending. Following this, the current tick count is gathered to allow for proper delaying during the task. The task checks the BMS queue to determine if the queue has something waiting to send prior to actually sending the message. If there is something waiting to send, the task will take the message out of the queue and begin copying the packaged message into a header "frame". A mailbox is then created for the sending of the CAN frame and checked to ensure it is not full. Then, the message is officially sent using `HAL_CAN_AddTxMessage`.

### _**task_CanProcess()**_

`task_CanProcess()` is the means by which CAN frames are processed after recieving them. The task does not run as an interrupt handler. Rather, it constantly checks a queue that is filled by `HAL_CAN_RxFifo0MsgPendingCallback` which is called by an interrupt when a CAN frame is recieved. First, the task checks if the queue has a message to process. If there is something in the queue, the task will take it out and begin a switch case statement to determine what to do with the message. If the CAN ID of the message is `ID_MAS_POW_CMD`, the BMS slave must either exit or enter low power state. The first bit in the data field of the recieved message determines what the slave is to do. If this bit is `POWER_ON`, the state of the finite state machine described above is set to `INIT`, an acknowledgement of command reception is sent back to the master module, and the slave module stores is connection status as connected. If the first bit of the data field is set to `POWER_OFF`, the finite state machine is set to `SHUTDOWN`, and an acknowledgement of the command is sent back to the master module. If the CAN ID of the message is `ID_MAS_PASSIVE`, the slave module will attempt to balance the cells of the module. In its current implementation, this CAN ID will not make the slave do anything. If the CAN ID is `ID_MAS_WDAWG`, the master module is attempting to tell the slave that it is alive and well. In this scenario, the slave module simply sends an acknowledgement back to main to let it know that the message was properly recieved. Finally, if the CAN ID is `ID_MAS_CONFIG`, the slave will attempt to set voltage and temperature parameters based on the information in the message. Using this, master has the ability to change the message rates for both the voltage and temperature measurements.

### _**task_heartbeat()**_

The heartbeat task literally just blinks an LED... That's all... Simple and easy!

### _**task_Master_WDawg()**_

The master watchdog task is closely related to the `ID_MAS_WDAWG` CAN ID discussed above. Slave expects master to respond with a watchdog signal at a certain frequency. If this message is not recieved in the proper time, the slave module knows that there is either a fault with the CAN bus or the master module. In this case, the task will indicate to the other running tasks that an error has been encountered. In this case, `task_error_checker()`, a task that will be discussed later, will notice the fault and officially force the finite state machine into the error state which will lead to a shutdown of the system. Other than this, there is nothing this task is actually responsible for.

### _***task_acquire_temp()***_

`task_acquire_temp()` does exactly what the name suggests... Acquires temperatures! To start its time, the task initializes the [`LTC2497`](https://www.mouser.com/datasheet/2/609/2497fb-1267645.pdf) by attempting to communicate over I2C (this will be implemented in the future). Follwing this, the task communicates with the chip and requests ADC values to be collected. When the I2C bus becomes ready again following the read, the task requests the values and starts handling the data.The ADC value from the chip is extracted from the I2C return using `adc_extract()`, and the ADC value is converted to a real temperature using `adc2temp()`. This value is then stored in its appropriate location in the BMS struct. This task is very dependent on knowledge of the datasheet, so it is recommended that the datasheet be read in full prior to work on this task.

### _***task_broadcast()***_

`task_broadcast()` is in charge of the regular flow of data out of the slave module via the CAN bus. It checks to make sure that the master module is ready to recieve both voltage and temperature information by checking to see if the enable flags are in the `ASSERTED` state. Following this, the task will use `send_volt_msg()` and `send_temp_msg()` to handle the sending of all values. These functions are quite straightforward. A CAN frame is generated with the required ID, length, and data in the required positions and added to the back of the transmit queue for `task_txCan()` to send to the master module.

### _***task_error_check()***_

The final task is `task_error_check()`. This task, as the name suggests, checks to see if any error flags have been raised. If there is a temperature connection error, a cell goes over/under-voltage, the BMIC mux fails, a BMIC wire becomes open, or the master and slave stop responding to each other, the task will trigger the error state which will lead to the shutdown of the slave module if master and slave are not connected or the sending of faults to master if the two are still communicating properly.

### Final Notes

Functions not mentioned in this document are quite simple in nature. They all do a simple task and don't require much thought to digest. If you can understand the makeup of the tasks above and how they operate together, then you understand what the slave module truly does. If you still have questions about parts specific to an IC (such as all of vstack.c or temp_adc.c), the respective datasheets will help a lot to decipher what is going on.
