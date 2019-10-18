# **BMS Slave**

## **Overview**

BMS slave is a portion of a distributed battery management system outlined [here](https://drive.google.com/drive/u/0/folders/10BhgTpdeEc__XZ9EmfNQqM7rGZ7nbBgL). The high level functionality of the board covers two main components.

- Gather voltage data from each cell in a battery module
- Relay information about each cell as well as any faults in the system to the master board of the system.

This is accomplished via the use of the `LTC6811-X` IC. This IC uses a multiplexer to take analog values of each cell's voltage in an efficient manner. Using this IC and an `STM32` microcontroller, the data required can be collected and sent to the master board for further processing. This document will outline the code written to facillitate the use of this IC, as well as describing the methods used in the code and why they are implemented on a higher level. Hopefully, this document will outline holes in the software while giving information about the processes running on the microcontroller.

## **Roadmap**

- [X] Functioning voltage readings
- [X] Functioning temperature readings
- [ ] Verification of dual IC mode
- [ ] Verification of diagnostics mode
- [ ] Fully functioning PEC for `LTC6811-2`
- [ ] Individual cell faults
- [ ] Optimization of CAN traffic for fault increase

## **Basic Structure**

The BMS slave `STM32` uses `FreeRTOS` to manage scheduling and execution of code. By doing this, the application can run multiple threads "simultaneously" allowing for easier development. Upon startup, a few tasks are spawned. In order, the functions started are:

```C
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

`task_bms_main`, as the name implies, is the main functioning task of the slave module. It is a [`finite state machine`](https://brilliant.org/wiki/finite-state-machines/) that determines the operating characteristics of the slave module. The states of slave are `low power`, `init`, `normal operation`, `error`, and `shutdown`. The main loop of the main funciton is as follows:

1. If the master module sends a `CAN` message commanding the slave to enter low power mode, the state is switched to low power mode, stopping major processing.
2. The slave is then woken at some point via in interrupt and enters the init state.
3. The init state sets debug lights and sets the default values for the slave module.
4. Following init is normal operation where the slave module gathers cell data and sends faults when required. This is the typical state for the slave module to be in.
5. If a set bound is exceeded, or if something disconnects, the slave will enter the error state which broadcasts all errors to the main module.
6. The shutdown state enters the low power sleep state.

In its current implementation, this is all the main task does as most of the vital processes slave runs are activated by `osKernalStart()` rather than the state machine itself. Therefore, the main task serves to put the slave module into the sleep state at the appropriate time, as well as broadcast faults gathered by the rest of the module.

The code for this portion is quite simple. Therefore, no in depth analysis will be given.  State machines are easy to follow by nature as they progress in a (typically) logical pattern. The code for the main task can be found on line 211 of `bms.c` at the time of writing.

### _**task_VSTACK()**_

`task_VSTACK` is, quite possibly, the most imporatant task the slave module runs. This is due to the fact that the job of the task is to monitor cell voltages and raise error flags when a parameter is violated. It does this via the `SPI` communication protocol which communicates with an `LTC6811-2 BMIC`. The datasheet for this part can be found [here](https://www.analog.com/media/en/technical-documentation/data-sheets/68111fb.pdf). The task follows a very logical set of steps during operation. These steps are as follows:

1. [B] The task initializes communication with the `BMIC` by pulsing the enable line.
2. [B] The slave module writes cell over/undervoltage values using the `WRCFGA` command.
3. [B] The `BMIC` enters a conversion mode in which cells 1 to 7 are polled for conversion.

> Not all cells are measured in this pass. The reasoning for this is to allow the multiplexer to settle prior to polling *all* cells. This note can be found in the applications sectinon of the datasheet.

4. The slave module waits for a set amount of microseconds according to the poll rate (typically 7 kHz) for the mux to settle.
5. [B] The `BMIC` enters a conversion mode in which *all* cells are polled for conversion.

> B means the command is sent in broadcast mode. A means the command is sent in address mode. The difference between the two modes can be found in the datasheet.

6. [A] The slave module requests the voltage register groups and saves the data for processing.
7. Every 10 cycles of the task runs diagnostics. These diagnostics can be found in the datasheet.

> All other processing (such as sending over CAN and the actual generation of fault flags) is done elsewhere.

### _**task_txCan()**_

`task_txCan` is the task responsible for putting messages on the `CAN bus` for the master module to read and process.

> Note: `task_txCan()` **DOES NOT** package messages. The messages are packaged in their respective tasks.

The task does a few things in order:

1. The task declares a CAN message struct to allow for the creation of a header medssage.
2. The current tick count is gathered for later delaying.
3. The task checks a queue to determine if there is a message waiting to send.
4. If there is something waiting to send, the task takes the CAN frame out of the queue and begins the formation of a header frame.
5. After the header frame is created, a mailbox is created/checked to make sure it is not full.
6. The message is officially sent using HAL, and the task runds its delay.

### _**task_CanProcess()**_

`task_CanProcess()` is the means by which CAN frames are processed after recieving them. The task does not run as an interrupt handler. Rather, it constantly checks a queue that is filled by `HAL_CAN_RxFifo0MsgPendingCallback` which is called by an interrupt when a CAN frame is recieved. The steps for sending are as follows:

1. The task checks to see if there is a message to process in the queue.
2. If there is something in the queue, the task takes it out of the queue.
3. The task enters a switch case statement with the CAN frame ID as the target.
4. If the ID is `ID_MAS_POW_CMD`, the BMS will enter or exit low power state depending on the bits in the message.

> If the first bit is `POWER_ON`, the slave state is set to `INIT`. If the first bit is `POWER_OFF`, the slave state is set to `SHUTDOWN`

5. If the ID is `ID_MAS_PASSIVE`, the BMS will attempt to balance the cells of the module (this will, more than likely, be deprecated via passive balancing)
6. If the ID is `ID_MAS_WDAWG`, the salve will send an acknowledgement to master letting it know that both boards are alive and well.
7. If the ID is `ID_MAS_CONFIG`, the slave will attempt to set voltage and temperature parameters based on the information in the measurement.

### _**task_heartbeat()**_

The heartbeat task literally just blinks an LED... That's all... Simple and easy! However, a better system might be put in place in the future.

### _**task_Master_WDawg()**_

The master watchdog task is closely related to the `ID_MAS_WDAWG` CAN ID discussed above. Slave expects master to respond with a watchdog signal at a certain frequency. If this message is not recieved in the proper time, the slave module knows that there is either a fault with the CAN bus or the master module. In this case, the task will indicate to the other running tasks that an error has been encountered. In this case, `task_error_checker()`, a task that will be discussed later, will notice the fault and officially force the finite state machine into the error state which will lead to a shutdown of the system. Other than this, there is nothing this task is actually responsible for.

### _***task_acquire_temp()***_

`task_acquire_temp()` does exactly what the name suggests... Acquires temperatures! This is done in the following manner:

1. The task initializes the [`LTC2497`](https://www.mouser.com/datasheet/2/609/2497fb-1267645.pdf) by attempting to communicate over I2C.
2. The task requests ADC values to be collected.
3. When the I2C bus becomes ready again, the task requests the ADC values and starts handling the data.
4. The ADC values from the chip are extracted from the I2C return using `adc_extract()`.
5. The ADC values from the chip are converted to real world temperatures using `adc2temp()`.
6. These values are stored in the BMS struct for further processing.

> This task (like VSTACK) is very dependent on knowledge of the datasheet, so it is recommended that the datasheet be read in full prior to work on this task.

### _***task_broadcast()***_

`task_broadcast()` is in charge of the regular flow of data out of the slave module via the CAN bus. It does this by doing the following:

1. The task checks to make sure the master module is ready to recieve both voltage and temperature information by checking to see if the enable flags are `ASSERTED`.
2. If both flags are `ASSERTED`, the task will use `send_volt_msg()` and `send_temp_msg()` to handle the sending of all values.
3. A CAN frame is generated with the required ID, length, and data in the required positions and added to the back of the transmit queue.

### _***task_error_check()***_

The final task is `task_error_check()`. This task, as the name suggests, checks to see if any error flags have been raised. If there is a temperature connection error, a cell goes over/under-voltage, the BMIC mux fails, a BMIC wire becomes open, or the master and slave stop responding to each other, the task will trigger the error state which will lead to the shutdown of the slave module if master and slave are not connected or the sending of faults to master if the two are still communicating properly.

### Final Notes

Functions not mentioned in this document are quite simple in nature. They all do a simple task and don't require much thought to digest. If you can understand the makeup of the tasks above and how they operate together, then you understand what the slave module truly does. If you still have questions about parts specific to an IC (such as all of vstack.c or temp_adc.c), the respective datasheets will help a lot to decipher what is going on.
