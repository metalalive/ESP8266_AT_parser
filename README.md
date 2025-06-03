# ESP8266 AT-command parser

This ESP8266 AT-command parser is platform-independent, minimal implementation for building application-specific network projects , which include ESP8266 wifi modules (unlike NodeMCU ESP32 device) connecting with resource-constraint embedded hardware platforms such as host MCU boards. 

## Important Note about versions of AT firmware
- not all existing AT commands documented are implemented in this repository.
- newer versions of AT firmware are not backward-compatible to all older ESP devices (e.g. ESP-01)

Typically newer versions of AT firmware support richer command set, however, some new AT commands might not work as expected in old  ESP devices (e.g. ESP-01s), also newer versions require more flash memory space, e.g. v1.6.0 requires at least 512KB memory, v1.7.x requires  at least 1MB.

### Tested ESP devices
- ESP-01s with AT firmware v1.6.0
- ESP-12s with AT firmware v1.7.6

## Supported AT commands 
Here is the list of AT-commands available in this repository.

|    |   |   |   |
|-------------|--------------|--------------|--------------|
| AT+RST      | AT+GMR       | ATE0 / ATE1  | AT+CIPSTA    |
| AT+RESTORE  | AT+CWMODE    | AT+CIPSTATUS | AT+CIPAP     |
| AT+CWJAP    | AT+CWLAP     | AT+CWQAP     | AT+CIPSTATUS | 
| AT+CIPSTART | AT+CIPCLOSE  | AT+CIPSEND   | AT+CIFSR     |
| AT+CIPMUX   | AT+CIPSERVER | AT+CIPSTO    | AT+CIPMODE   |
| AT+PING     | AT+CIPDINFO  | AT+GSLP      |  |


## How To Start
Check [integration quide](./docs/INTEGRATION_GUIDE.md) and help doc for building / running test application by running the command :

```shell
make help
```

## Internal Architecture
See [developer guide](./docs/DEVELOPER.md) for detail

## LICENSE
[BSD 3-Clause](./LICENSE)

