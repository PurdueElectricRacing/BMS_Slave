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

`task_VSTACK` is, quite possibly, the most imporatant task the slave module runs. This is due to the fact that the job of the task is to monitor cell voltages and raise error flags when a parameter is violated. It does this via the `SPI` communication protocol which communicates with an `LTC6811-2 BMIC`. The datasheet for this part can be found [here](https://www.analog.com/media/en/technical-documentation/data-sheets/68111fb.pdf).
