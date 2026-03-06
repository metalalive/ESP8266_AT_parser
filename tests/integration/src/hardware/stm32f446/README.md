## Integration Test Hardware Setup (STM32F446 with ESP Device)

To run the integration tests successfully, set up your hardware as described below. This guide explains the essential connections between the STM32F446 MCU board and the ESP device. Refer to [the electrical schematic diagram `esp8266-for-itest-compact.svg`](./esp8266-for-itest-compact.svg) for visual details.

### Key Connections
**ESP Device Communication (UART):**
- `PC5` (USART3_RX) connects to ESP `TX` pin.
- `PB10` (USART3_TX) connects to ESP `RX` pin.
- These pins enable serial data exchange between the MCU and the ESP device.

**ESP Device Control Pins:**
- `PH0` controls ESP `CH_PD` (chip enable) and `GPIO0` (boot mode selection, pull HIGH for running mode).
- `PH1` controls the external power switch for the ESP device (e.g., MOSFET or relay).
- `PB9` connects to ESP `RST` pin for hardware reset control.

### Important Notes
- Ensure all ground connections are common between the STM32F446 board and the ESP device.
- Provide stable 3.3V power to the ESP device according to its specifications.
- The ESP `GPIO0` pin should be configured high for normal operation (running mode), along with `CH_PD`, which are both controlled by `GPIOH_PIN_0` in this setup.

This hardware configuration allows the STM32 MCU to communicate with, control, and reset the ESP device, enabling reliable execution of integration test cases.
