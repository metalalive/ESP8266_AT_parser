# ESP-AT-Parser Library Integration Guide

This guide provides a quick overview for integrating the ESP-AT-parser library into your embedded system applications. The library is designed to communicate with ESP series modules (e.g., ESP8266) using AT commands, abstracting the complexities of the AT command protocol.

## Overview
The ESP-AT-parser library provides a framework for interacting with ESP modules via their AT-command firmware. It handles command sending, response parsing, event management, and provides a high-level API for common Wi-Fi and network operations (e.g., connecting to Access Point, TCP connections).

The library is designed with a clear separation of concerns:
- **Core Logic**: Handles AT command parsing, internal state machine, and high-level API.
- **System Layer**: Abstracts OS-specific functionalities like threads, mutexes, semaphores, and delays.
- **Low-Level Hardware Layer**: Abstracts hardware-specific functionalities like UART communication and GPIO control for the ESP module.

## Dependencies
- **Operating System (OS)**: The library is designed to be OS-agnostic but requires an OS abstraction layer. FreeRTOS is currently supported and used in the integration tests. You will need to port the system layer if you are using a different RTOS or a bare-metal environment.
- **Hardware Abstraction Layer (HAL)**: You need to provide implementations for UART communication (send/receive) and GPIO control (for ESP reset). The integration tests use STM32F4 HAL.

## Core Integration Steps
To integrate this library, it is essential to implement the system and low-level hardware abstraction layers.

### System Layer Implementation
The system layer provides OS primitives required by the library. Refer to `include/system/esp_sys.h` for the required function signatures and `include/system/esp_system_freertos.h` and `src/system/esp_system_freertos.c` for a FreeRTOS example.

Application developers need to implement (or adapt existing OS-specific implementations for) the following:

- **Thread Management**:
  - `eESPsysThreadCreate`: Create a new thread.
  - `eESPsysThreadDelete`: Delete a thread.
  - `eESPsysTskSchedulerStart`: Start the OS scheduler.
- **Synchronization Primitives**:
  - `eESPsysMtxCreate`, `eESPsysMtxDelete`, `eESPsysMtxLock`, `eESPsysMtxUnlock`: Mutex operations.
  - `eESPsysSemCreate`, `eESPsysSemDelete`, `eESPsysSemWait`, `eESPsysSemRelease`: Semaphore operations.
  - `eESPsysMboxCreate`, `eESPsysMboxDelete`, `eESPsysMboxPut`, `eESPsysMboxGet`: Mailbox operations for inter-thread communication.
- **Delay**:
  - `vESPsysDelay`: Introduce a delay in milliseconds.

#### FreeRTOS Integration Example
The `src/system/esp_system_freertos.c` file demonstrates how these functions are mapped to FreeRTOS APIs, for example:
- `eESPsysThreadCreate` uses `xTaskCreate`
- `vESPsysDelay` uses `vTaskDelay`

### Low-Level Hardware Layer Implementation
This layer handles the direct communication with the ESP module via UART and controls its reset pin. The functions signatures are defined in `include/system/esp_sys.h`.

The file `tests/integration/src/hardware/stm32f446/stm32f446_config.c` provides implementation / integration example for testing on STM32F4 platform. Application developers need to implement the following functions for other specific hardware platform:

- **Device Initialization**:
  - `eESPinit(...)` initializes the ESP library in the whole application, and internally invokes `eESPlowLvlDevInit(...)` exactly once.
  - *Refer to* `stm32f446_config.c` for `eESPlowLvlDevInit` which configures UART3 and DMA for reception, and GPIO for the reset pin.

- **Data Transmission**:
  - `eESPlowLvlSendFn(void *data, size_t len, uint32_t timeout)`: Transmit data (AT commands) to the ESP module.
  - *Refer to* `stm32f446_config.c` for `eESPlowLvlSendFn`, which uses `HAL_UART_Transmit` from STM32 library to transmit bytes oever UART.

- **Data Reception**:
  - `eESPlowLvlRecvStartFn(void)`: Start continuous reception from the ESP module. In my STM32 example this typically involves setting up a DMA-based UART reception.
  - `vESPlowLvlRecvStopFn(void)`: Stop reception.
  - *Refer to* `stm32f446_config.c` for `eESPlowLvlRecvStartFn` and `vESPlowLvlRecvStopFn` which control UART Rx using `HAL_UART_Receive_DMA` and `HAL_UART_DMAStop`.

- **ISR for Data Reception**
  - the library expects received data to be appended via `eESPappendRecvRespISR(uint8_t *data, uint16_t data_len)` (declared in `include/esp/esp.h`).
  - your UART receive ISR (or DMA callback) should call this function to feed data into the parser, see example usage in `USART3_IRQHandler` and `HAL_UART_RxCpltCallback` in `stm32f446_config.c`.

- **Reset Control**:
  - `eESPlowLvlRstFn(uint8_t state)`: Control the reset pin of the ESP module. `state = 0` for assertion (reset), non-zero for de-assertion (release from reset).
  - *Refer to* `stm32f446_config.c` for `eESPlowLvlRstFn` which controls pinout `PB9` (configured as reset pin of ESP device) of STM32F446 developmenet board.


### Library Initialization and Event Handling
Once the system and low-level hardware layers are implemented, you can initialize the ESP-AT-parser library.

**Initialize the ESP library**: Call `eESPinit(&ll_config, evt_cb_fn, NULL, 1)`.
- `ll_config`: A `espLLvl_t` structure containing pointers to your `send_fn` and `reset_fn` implementations, and UART baudrate.
- `evt_cb_fn`: A callback function of type `espEvtCbFn` to handle events from the library (e.g., `ESP_EVT_INIT_FINISH`, `ESP_EVT_RESET_DETECTED`, `ESP_EVT_CMD_TIMEOUT`).
- *Refer to* `vESPtestInitTask` function (declared in all test cases), it is where this initialization typically happens.

### Starting the Scheduler
After setting up all necessary tasks (including the ESP library's internal tasks, which are created during `eESPinit`), you must start the RTOS scheduler by calling `eESPsysTskSchedulerStart()`.
- *Refer to* `src/system/esp_system_freertos.c`, for `eESPsysTskSchedulerStart` which calls `vTaskStartScheduler()`.

## Example Usage Patterns
### Connecting to an Access Point (AP)
The `eESPtestConnAP` function (found in `tests/integration/src/demo_apps/common.c`, used across multiple test applications) demonstrates how to connect the ESP module to a Wi-Fi access point.

Key steps typically involve:
- Checking current connection status (`eESPstaHasIP`).
- Scanning for available APs (`eESPstaListAp`).
- Connecting to a preferred AP (`eESPstaConnect`).

## Referencing Integration Tests
The table below lists all integration test cases located at `tests/integration/src/demo_apps` in this repository.

| test name      | description  |
|----------------|--------------|
|`ap_ping`       | send a ICMP ping request to specific host  |
|`http_server`   | turn on TCP server mode, demonstrate basic capability of HTTP/1.1 handshaking process. |
|`mqtt_client`   | start TCP connection, demonstrate publishing/subscribing capability of MQTT v5.0 handshaking process. |

- Once the test program `http_server` has started successfully, use tools that support HTTP (e.g. `curl`, `wget`) to send HTTP requests to this test server running on the ESP device.
- Before starting the test program `mqtt_client`, run MQTT broker (e.g. `Eclipse Mosquitto`) and subscriber tool (e.g. `mosquitto_sub`) on other devices, in order to interact with this test MQTT client.
- For MQTT test, be sure to update correct credential and broker domain name for successful connection.

## Configuration

Application developers can configure the library's behavior via `esp_config.h` (not provided in the summary, but implied by `ESP_CFG_MAX_SSID_LEN`, `ESP_CFG_MODE_STATION`, etc.). This file typically defines:
- Maximum lengths for SSID, password, etc.
- Enabled Wi-Fi modes (Station, Access Point, or both).
- Buffer sizes for AT command communication.
- Underlying OS support selection
- Debug options.

You should review and customize this configuration file according to your application's needs and available memory.

## 6. Troubleshooting

*   **Command Timeout (`ESP_EVT_CMD_TIMEOUT`)**: If you frequently receive this event, it might indicate issues with UART communication (baud rate mismatch, wiring), ESP module responsiveness, or incorrect AT command sequences.
*   **Memory Issues**: Ensure sufficient stack size for threads and heap memory for dynamic allocations. FreeRTOS's `configMINIMAL_STACK_SIZE` and `configTOTAL_HEAP_SIZE` are relevant here.
*   **Interrupt Priorities**: Correctly configure interrupt priorities, especially for UART DMA and FreeRTOS `configMAX_SYSCALL_INTERRUPT_PRIORITY`.

This guide should provide a solid starting point. For detailed API usage, refer to the library's header files and the integration test source code.
