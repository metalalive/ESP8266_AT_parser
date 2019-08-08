# ESP8266 AT-command parser

This ESP8266 AT-command parser is platform-independent, minimal implementation for building application-specific network projects , which connect with old ESP8266 wifi modules (not like NodeMCU ESP device) and resource-constraint embedded devices like host MCU boards. 


### Issues about versions of the AT firmware ...
If you program the latest version (v1.7.x) of AT firmware to the ESP device, there would be more new AT commands supported on the device, however it is not that every new AT command always works as expected, also the AT firmware requires more than 512KB flash memory space since the version v1.7.0 was released, that means you are NOT allowed to use the latest AT firmware on some old ESP8266 devices (e.g. ESP-01). 
Therefore I don't implement all of the existing AT commands at here, I also choose the stable version (v1.6.0) of AT firmware instead, which can work with all types of ESP8266 devices.  


### AT commands available. 
Here is the list of AT-commands available in this repository.

| AT+RST      | AT+GMR       | ATE0 / ATE1  | AT+CIPSTA    |
|-------------|--------------|--------------|--------------|
| AT+RESTORE  | AT+CWMODE    | AT+CIPSTATUS | AT+CIPAP     |
| AT+CWJAP    | AT+CWLAP     | AT+CWQAP     | AT+CIPSTATUS | 
| AT+CIPSTART | AT+CIPCLOSE  | AT+CIPSEND   | AT+CIFSR     |
| AT+CIPMUX   | AT+CIPSERVER | AT+CIPSTO    | AT+CIPMODE   |
| AT+PING     | AT+CIPDINFO  | |  |


### Tests
There are 3 test cases in this repository, these tests start after your ESP device switches to station mode and successfully connects to available AP.

| test name      | what it does                   |
|----------------|--------------------------------|
|`ping`          | ping a specific host           |
|`http_server`   | turn on server mode, demonstrate basic capability of HTTP/1.1 handshaking process. |
|`mqtt_client`   | start TCP connection, demonstrate publish/subscribe capability of MQTT v5.0 handshaking process. |

Note that :
* For the test `http_server`, you can send HTTP requests to the ESP server by using web browsers, related commands like `wget`, or other debugging tools
* For the test `mqtt_client`, you need to run MQTT broker (server) & subscribing software on other devices in order to run this test. (In my case, I run **Mosquitto MQTT broker** and **Paho.MQTT.C subscriber** seperately on 2 of my Raspberry PIs)


### Quick Start
For build


### Porting Guide


