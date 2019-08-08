# ESP8266 AT-command parser

This ESP8266 AT-command parser is platform-independent, minimal implementation for building application-specific network projects that connect with old ESP8266 wifi modules (not like Node MCU ESP device) and resource-constraint embedded devices like host MCU boards. 


### Issues about versions of the AT firmware ...
If you program the latest version (v1.7.x) of AT firmware to the ESP device, there would be more new AT commands supported on the device, however it is not that every new AT command always works as expected, also the AT firmware requires more than 512KB flash memory space since the version v1.7.0 was released, you are NOT allowed to program the latest AT firmware to the old ESP8266 devices like ESP-01. Therefore we choose the stable version (v1.6.0) of AT firmware instead, which can work with all types of ESP8266 devices. 


### AT commands available. 
Here is the list of AT-commands available in this repository.

| AT+RST      | AT+GMR       | ATE0 / ATE1  | AT+CIPSTA    |
|-------------|--------------|--------------|--------------|
| AT+RESTORE  | AT+CWMODE    | AT+CIPSTATUS | AT+CIPAP     |
| AT+CWJAP    | AT+CWLAP     | AT+CWQAP     | AT+CIPSTATUS | 
| AT+CIPSTART | AT+CIPCLOSE  | AT+CIPSEND   | AT+CIFSR     |
|  | | | 

### Quick Start
There are 3 test cases in this repository : 

| test name      | what it does                   |
|----------------|--------------------------------|
|`ping`          |  |
|`http_server`   | | 
|`mqtt_client`   | |


For build


### Porting Guide


