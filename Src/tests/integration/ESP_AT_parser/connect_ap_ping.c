#include "tests/integration/ESP_AT_parser/connect_ap_ping.h"

#define  TASK_MIN_STACK_SIZE      (( unsigned portSHORT ) 0x7e)



static void vESPtestConnAPtask(void *params)
{
    espRes_t  response ;
    uint8_t   devPresent ;
    const uint8_t  waitUntilConnected = 0x0;

    for(;;)
    {
        // Set device is present 
        devPresent = 0x1;
        eESPdeviceSetPresent( devPresent, NULL, NULL );

        // Connect to access point.
        // Try unlimited time until access point accepts up.
        // Check for station_manager.c to define preferred access points ESP should connect to
        eESPtestConnAP( waitUntilConnected );
        vESPsysDelay(20000);

        // Set device is NOT present 
        devPresent = 0x0;
        eESPdeviceSetPresent( devPresent, NULL, NULL );
        vESPsysDelay(5000);
    } // end of outer infinite loop
} // end of vESPtestConnAPtask




static void vESPtestPingTask(void *params)
{
    espRes_t  response ;
    uint32_t  pingresptime = 0;
    for(;;)
    {
        pingresptime = 0;
        eESPping( "stackoverflow.com", sizeof("stackoverflow.com"), &pingresptime, 
                   NULL, NULL, ESP_AT_CMD_BLOCKING );
        vESPsysDelay( 1000 );
    } // end of outer infinite loop
} // end of vESPtestPingTask





static espRes_t vESPinitCallBack( espEvt_t*  evt )
{
    uint8_t   devPresent ;
    espRes_t  response ;
    switch( evt->type )
    {
        case ESP_EVT_INIT_FINISH :
            printf("[INFO] vESPinitCallBack, library initialized ... OK! \r\n");
            break;

        case ESP_EVT_RESET_DETECTED:
            printf("[INFO] vESPinitCallBack, Device reset detected \r\n");
            break;

        case ESP_EVT_RESET:
            if(evt->body.reset.res == espOK) {
                printf("[INFO] vESPinitCallBack, ESP reset sequence finished with success! \r\n");
            }
            else {
                printf("[INFO] vESPinitCallBack, ESP reset sequence error\r\n");
            }
            break;

        case ESP_EVT_WIFI_CONNECTED:
            printf("[INFO] vESPinitCallBack, Wifi connected to access point!\r\n");
            break;

        case ESP_EVT_WIFI_DISCONNECTED:
            printf("[INFO] vESPinitCallBack, Wifi disconnected from access point!\r\n");
            break;
        case ESP_EVT_PING:
            printf("[INFO] ping response time : \r\n");
            response = evt->body.ping.res;
            if ( response == espOK) {
                printf("Ping successful in %d milliseconds!\r\n", *(evt->body.ping.resptime) );
            } else {
                printf("Ping error with res: %d\r\n", (int)response);
            }
            break;

        default:
            break;
    } // end of switch statement
} // end of vESPinitCallBack





static void vESPtestInitTask(void *params)
{
    uint8_t     xState ;
    uint8_t     isPrivileged = 0x1;
    espRes_t  response =  eESPinit( vESPinitCallBack );
    if( response == espOK ) {
        // create the 2 threads for this test
        xState = eESPsysThreadCreate( NULL, "espTestPing", vESPtestPingTask, NULL ,
                                   (0x20 + TASK_MIN_STACK_SIZE), ESP_APPS_THREAD_PRIO , isPrivileged );
        xState = eESPsysThreadCreate( NULL, "espTestConnAP", vESPtestConnAPtask, NULL ,
                                   (0x20 + TASK_MIN_STACK_SIZE), (ESP_APPS_THREAD_PRIO + 1) , isPrivileged );
    }
    else {
        // failed to initialize ESP AT library
    }
    eESPsysThreadDelete( NULL );
} // end of vESPinitTask





void  vESPtestStartPingTask(void)
{
    espRes_t response ;
    uint8_t isPrivileged = 0x1;
    // the ESP initialization thread takes the smae priority as the 2 internal threads working 
    // in ESP AT software.
    response = eESPsysThreadCreate( NULL, "espInitTask", vESPtestInitTask, NULL ,
                                    TASK_MIN_STACK_SIZE, ESP_SYS_THREAD_PRIO ,  isPrivileged
                                  );
    ESP_ASSERT( response == espOK ); 
} // end of vESPtestStartPingTask



