#include "esp/esp.h"
#include "esp/esp_private.h"

// the structure to represent each item in a message box.
typedef struct {
    void* datap;
} mboxitem;


static espSysMtx_t  internal_mutex;



PRIVILEGED_FUNCTION espRes_t  eESPsysInit( void )
{
    internal_mutex = xESPsysMtxCreate();
    return espOK;
} // end of eESPsysInit



PRIVILEGED_FUNCTION espSysMtx_t  xESPsysMtxCreate( void )
{   // In FreeRTOS implementation, we apply recursive mutex as the lock to caller.
    espSysMtx_t mutex = xSemaphoreCreateRecursiveMutex() ;
    configASSERT( mutex != NULL );
    return mutex;
} // end of xESPsysMtxCreate




PRIVILEGED_FUNCTION void  vESPsysMtxDelete ( espSysMtx_t* mutex )
{
    configASSERT( (*mutex) != NULL );
    vSemaphoreDelete( *mutex );
    *mutex = NULL;
} // end of vESPsysMtxDelete




PRIVILEGED_FUNCTION espSysSem_t   xESPsysSemCreate   ( void )
{
    espSysSem_t  sem =  xSemaphoreCreateBinary();
    configASSERT( sem != NULL );
    return sem ;
} // end of xESPsysSemCreate




PRIVILEGED_FUNCTION void  vESPsysSemDelete ( espSysSem_t* sem )
{
    configASSERT( (*sem) != NULL );
    vSemaphoreDelete( *sem );
    *sem = NULL;
} // end of vESPsysSemDelete



PRIVILEGED_FUNCTION  espSysMbox_t  xESPsysMboxCreate   ( size_t length )
{
    espSysMbox_t mb = xQueueCreate( length, sizeof(mboxitem) );
    configASSERT( mb != NULL );
    return mb ;
} // end of xESPsysMboxCreate



PRIVILEGED_FUNCTION  void          vESPsysMboxDelete   ( espSysMbox_t* mb )
{
    // note that this function only delete the message box structure,
    // application writers still need to manually free the memory allocated to
    // each item of the message box.
    configASSERT( (*mb) != NULL );
    vQueueDelete( *mb );
    *mb = NULL;
} // end of vESPsysMboxDelete




PRIVILEGED_FUNCTION  espRes_t  eESPsysThreadCreate ( espSysThread_t* t, const char* name,  espSysThreFunc  thread_fn, 
          void* const arg,  size_t stack_size, espSysThreadPrio_t prio, uint8_t isPrivileged )
{
    extern  UBaseType_t  __unpriv_data_start__ [];
    extern  UBaseType_t  __unpriv_data_end__ [];
    StackType_t        *stackMemSpace ;
    BaseType_t          xState; 
    unsigned portSHORT  idx;

    stackMemSpace = (StackType_t *) pvPortMalloc( sizeof(StackType_t) * stack_size );
    configASSERT( stackMemSpace != NULL );
    // collect parameters to feed to xTaskCreateRestricted()
    TaskParameters_t tskparams = {
        thread_fn, name, stack_size, arg, prio, stackMemSpace,
        // leave MPU regions uninitialized
    };
    // default value to unused MPU regions 
    for(idx=0; idx<portNUM_CONFIGURABLE_REGIONS; idx++)
    {
        tskparams.xRegions[idx].pvBaseAddress   = NULL;
        tskparams.xRegions[idx].ulLengthInBytes = 0;
        tskparams.xRegions[idx].ulParameters    = 0;
    }
    if(isPrivileged != 0x0) {
        tskparams.uxPriority |= portPRIVILEGE_BIT ;
    }
    else {
        // add extra region for logging assertion failure in unprivileged tasks.
        tskparams.xRegions[0].pvBaseAddress   = (UBaseType_t) __unpriv_data_start__;
        tskparams.xRegions[0].ulLengthInBytes = (UBaseType_t) __unpriv_data_end__ - (UBaseType_t) __unpriv_data_start__;
        tskparams.xRegions[0].ulParameters    = portMPU_REGION_READ_WRITE | MPU_RASR_S_Msk
                                               | MPU_RASR_C_Msk | MPU_RASR_B_Msk;
    }
    xState = xTaskCreateRestricted( (const TaskParameters_t * const)&tskparams, t );
    configASSERT( xState == pdPASS );
    return  (xState == pdPASS ? espOK : espERR );
} // end of eESPsysThreadCreate





PRIVILEGED_FUNCTION  espRes_t  eESPsysThreadDelete( espSysThread_t* t )
{
    if( t != NULL ) {
        vTaskDelete( *t );
        *t = NULL;
    }
    else {
        vTaskDelete( NULL );
    }
    // in FreeRTOS implementation, if a task deletes itself,  vTaskDelete will call task yielding 
    // function and switch to other tasks, therefore the deleted task won't get here.
    return  espOK;
} // end of ucESPsysThreadDelete




espRes_t     eESPsysMtxLock     ( espSysMtx_t*  mtx )
{
    espRes_t    response;
    BaseType_t  mtxOpsStatus = xSemaphoreTakeRecursive( *mtx, portMAX_DELAY );
    if(mtxOpsStatus == pdPASS) {
        response = espOK;
    }
    else {
        response = espERR ;
    }
    return response;
}  // end of eESPsysMtxLock



espRes_t     eESPsysMtxUnlock   ( espSysMtx_t*  mtx )
{
    espRes_t  response;
    BaseType_t  mtxOpsStatus = xSemaphoreGiveRecursive( *mtx );
    if( mtxOpsStatus == pdPASS ) {
        response = espOK;
    }
    else {
        response = espERR ;
    }
    return response;
}  // end of eESPsysMtxUnlock



espRes_t   eESPsysProtect( void )
{
    return eESPsysMtxLock( &internal_mutex );
} // end of eESPsysProtect




espRes_t   eESPsysUnprotect( void )
{
    return eESPsysMtxUnlock( &internal_mutex );
} // end of eESPsysUnprotect





espRes_t  eESPsysMboxPut  ( espSysMbox_t  mb, void* msg, uint32_t block_time )
{
    configASSERT( mb != NULL );
    configASSERT( msg != NULL );
    BaseType_t QopsStatus = pdPASS;
    mboxitem item;
    item.datap = msg;
    QopsStatus = xQueueSendToBack( mb, (void *)&item, block_time );
    return (QopsStatus == pdPASS ? espOK : espERRMEM);
} // end of eESPsysMboxPut




espRes_t  eESPsysMboxGet ( espSysMbox_t  mb, void** msg, uint32_t block_time )
{
    configASSERT(  mb != NULL );
    configASSERT(  msg != NULL );
    BaseType_t QopsStatus = pdPASS;
    mboxitem item;
    QopsStatus = xQueueReceive( mb, (void *)&item, block_time );
    configASSERT( item.datap != NULL );
    *msg = item.datap;
    return (QopsStatus == pdPASS ? espOK : espERRMEM);
} // end of eESPsysMboxGet




espRes_t    eESPsysMboxPutISR   ( espSysMbox_t  mb, void*  msg )
{
    configASSERT( mb != NULL );
    configASSERT( msg != NULL );
    BaseType_t  xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t  QopsStatus = pdPASS;
    mboxitem item;
    item.datap = msg;
    QopsStatus = xQueueSendToBackFromISR( mb, (void *)&item, &xHigherPriorityTaskWoken );
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    return (QopsStatus == pdPASS ? espOK : espERRMEM);
} // end of eESPsysMboxPutISR




espRes_t      eESPsysSemWait     ( espSysSem_t  sem, uint32_t block_time )
{
    configASSERT( sem != NULL );
    BaseType_t semOpsStatus = pdPASS;
    semOpsStatus = xSemaphoreTake( sem, block_time );
    return (semOpsStatus == pdPASS ? espOK : espTIMEOUT);
} // end of eESPsysSemWait




espRes_t      eESPsysSemRelease  ( espSysSem_t  sem )
{
    configASSERT( sem != NULL );
    BaseType_t semOpsStatus = pdPASS;
    semOpsStatus = xSemaphoreGive( sem );
    return (semOpsStatus == pdPASS ? espOK : espERR);
} // end of eESPsysSemRelease




espRes_t    eESPsysThreadYield( void )
{
    taskYIELD();
    return espOK ;
} // end of eESPsysThreadYield



void   vESPsysDelay( const uint32_t ms )
{
    vTaskDelay( pdMS_TO_TICKS(ms) );
} // end of vESPsysDelay



