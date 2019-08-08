# Development Guide


### Brief introduction of software design of this AT-command parser

This ESP AT-command parser runs with 2 seperate tasks (threads), one of the 2 tasks *(say the request-handling task )* handles incoming AT command request sent by API calls, generates low-level command raw bytes and then send then out to ESP8266 device ; while the other task *(say the response-handling task )* processes raw response string received from ESP8266 device, extracts meaningful data from the received string, and pass them back to API function calls.

Besides the 2 tasks described above, there is an interrupt service routine (ISR in short). When the AT-command raw bytes are transmitted to ESP device, our parsing software will wait until the ESP device transmits  variable number of bytes as response of the AT command back to host CPU, as soon as number of raw bytes (part of response data) are ready in the DMA Rx buffer of the underlying hardware platform, the DMA is expected to signal interrupt to the host CPU, then the host CPU switches to the ISR, starts copying the received raw bytes from DMA Rx buffer to the buffer structure internally defined / used in this ESP AT parser, let the response-handling task process the raw bytes later.

The ISR works closely with the 2 tasks, and DMA (Direct Memory Access controller) / UART hardware modules, which are essential in your underlying platform.


### Code Structure
| path of folders | description |
|-----------------|-------------|
| `Src/ESP_AT_parser/inc/esp`  | core functions, API functions, macros, data structure are declared at here |
| `Src/ESP_AT_parser/inc/system` | system ports for underlying OS platform. |
| `Src/ESP_AT_parser/src/esp` | core functions implementation | 
| `Src/ESP_AT_parser/src/api` | API functions implementation |
| `Src/ESP_AT_parser/src/system` | system ports implementation |


### Macros to declare in each platform
xxx

### Low-level system functions to port
xxx




