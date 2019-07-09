#include "stm32f4xx_hal.h"
#include "esp/esp.h"

static   UART_HandleTypeDef haluart1; // for printf function
static   UART_HandleTypeDef haluart3; // for ESP device
static   DMA_HandleTypeDef  haldma_usart3_rx; // the DMA module used with UART1 (STM32F4xx board)

#define   ESP_RESP_RECV_BUF_SIZE  0x100
static    uint8_t   recv_data_buf[ ESP_RESP_RECV_BUF_SIZE ]; // get rough response data from ESP device.

// in each system port, DMA/UART ISR should specify starting offset & number of characters to copy to ESP AT software.
static  uint16_t  dma_buf_num_char_copied = 0;
static  uint16_t  dma_buf_cpy_offset_next = 0;
static  uint16_t  dma_buf_cpy_offset_curr = 0;




// In this test, the underlying hardware platform is STM32F4xx board. 
// we apply STM32 Hardware Abstraction Layer & implement the funcitons 
// that are called in STM32 HAL driver code.
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(huart->Instance==USART1)
    {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        // USART1 GPIO Configuration    
        // PA9     ------> USART1_TX
        // PA10    ------> USART1_RX 
        GPIO_InitStruct.Pin   = GPIO_PIN_9 ;
        GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull  = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
    else if(huart->Instance==USART2)
    { // TODO: figure out why USART2 of my STM32 board doesn't work
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        // USART2 GPIO Configuration    
        // PA3     ------> USART2_RX
        // PA2     ------> USART2_TX 
        GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
    else if(huart->Instance==USART3)
    {
        __HAL_RCC_USART3_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        // USART3 GPIO Configuration    
        // PC5     ------> USART3_RX
        // PB10    ------> USART3_TX 
        GPIO_InitStruct.Pin = GPIO_PIN_5;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
} // end of HAL_UART_MspInit




// implementing the funcitons that are called in our ESP AT software. 
// initialize UART, DMA driver for printf & ESP device in this test
espRes_t  eESPlowLvlDevInit(void *params)
{
    __HAL_RCC_DMA1_CLK_ENABLE();
    HAL_NVIC_SetPriority( DMA1_Stream1_IRQn, (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1), 0 );
    HAL_NVIC_EnableIRQ( DMA1_Stream1_IRQn );
    //// HAL_NVIC_SetPriority( DMA1_Stream5_IRQn, (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1), 0 );
    //// HAL_NVIC_EnableIRQ( DMA1_Stream5_IRQn );
    // ---------- initialize UART for printf debugging use ---------- 
    haluart1.Instance = USART1;
    haluart1.Init.BaudRate = 115200;
    haluart1.Init.WordLength = UART_WORDLENGTH_8B;
    haluart1.Init.StopBits = UART_STOPBITS_1;
    haluart1.Init.Parity = UART_PARITY_NONE;
    haluart1.Init.Mode = UART_MODE_TX ; // printf doesn't need Rx 
    haluart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    haluart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&haluart1) != HAL_OK) {
        return espERR;
    }
    //  ---------- initialize UART for ESP device ---------- 
    haluart3.Instance = USART3;
    //// haluart3.Instance = USART2;
    haluart3.Init.BaudRate = 115200;
    haluart3.Init.WordLength = UART_WORDLENGTH_8B;
    haluart3.Init.StopBits = UART_STOPBITS_1;
    haluart3.Init.Parity = UART_PARITY_NONE;
    haluart3.Init.Mode = UART_MODE_TX_RX;
    haluart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    haluart3.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&haluart3) != HAL_OK) {
        return espERR;
    }
    //// manually enable IDLE line detection interrupt
    __HAL_UART_ENABLE_IT( &haluart3 , UART_IT_IDLE );    
    HAL_NVIC_SetPriority( USART3_IRQn, (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1), 0 );
    HAL_NVIC_EnableIRQ( USART3_IRQn );
    //// HAL_NVIC_SetPriority( USART2_IRQn, (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1), 0 );
    //// HAL_NVIC_EnableIRQ( USART2_IRQn );
    //  ---------- initialize DMA for Rx of ESP device. ---------- 
    haldma_usart3_rx.Instance = DMA1_Stream1;
    //// haldma_usart3_rx.Instance = DMA1_Stream5;
    haldma_usart3_rx.Init.Channel = DMA_CHANNEL_4;
    haldma_usart3_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    haldma_usart3_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    haldma_usart3_rx.Init.MemInc = DMA_MINC_ENABLE;
    haldma_usart3_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    haldma_usart3_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    haldma_usart3_rx.Init.Mode = DMA_CIRCULAR;
    haldma_usart3_rx.Init.Priority = DMA_PRIORITY_LOW;
    haldma_usart3_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&haldma_usart3_rx) != HAL_OK) {
        return espERR;
    }
    __HAL_LINKDMA(&haluart3, hdmarx, haldma_usart3_rx);

    //  ---------- initialize GPIO pins  for ESP device ---------- 
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    // GPIO Ports Clock Enable 
    __HAL_RCC_GPIOB_CLK_ENABLE();
    // Configure GPIO pin Output Level
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9 | GPIO_PIN_4, GPIO_PIN_RESET);
    // Configure GPIO pins : PB10 PB4 
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    return espOK;
} // end of  eESPlowLvlDevInit


// UART Rx interrupt service routine in this test
void USART3_IRQHandler( void )
{
    HAL_UART_IRQHandler(&haluart3);
    // check if Idle flag is set, if idle line detection event leads to this interrupt.
    if ( __HAL_UART_GET_FLAG( &haluart3, UART_FLAG_IDLE ) )
    {
        // clear current IDLE-detection interrupt.
        __HAL_UART_CLEAR_IDLEFLAG( &haluart3 );
        // Disabling DMA will force transfer complete interrupt if enabled.
        // TODO: following code is likely to cause problem when DMA TC event triggers CPU, CPU executes following macro code, 
        //       , then DMA Rx is reinitialized next time. that will cause Rx fails to receive any character.
        //__HAL_DMA_DISABLE( &haldma_usart3_rx );
        dma_buf_cpy_offset_next =  ESP_RESP_RECV_BUF_SIZE  - __HAL_DMA_GET_COUNTER( &haldma_usart3_rx ); 
        dma_buf_num_char_copied  = dma_buf_cpy_offset_next -  dma_buf_cpy_offset_curr;
        eESPappendRecvRespISR( (haluart3.pRxBuffPtr + dma_buf_cpy_offset_curr), dma_buf_num_char_copied );
        dma_buf_cpy_offset_curr = dma_buf_cpy_offset_next;
    } 
} // end of USART3_IRQHandler



// DMA interrupt service routine 
void DMA1_Stream1_IRQHandler(void)
{
  HAL_DMA_IRQHandler( &haldma_usart3_rx );
} // end of DMA1_Stream1_IRQHandler





// [TODO] 
// calculate which half of the Rx buffer should be copied to ESP core function at here, (since the way to generate DMA/UART
// interrupt for HT/TC event depends on hardware platform. executed by DMA Half Transmission (HT) event interrupt
void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
} // end of HAL_UART_RxHalfCpltCallback



// executed by DMA Transmission completion (TC) event interrupt
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if( huart == &haluart3){
        //// eESPappendRecvRespISR( huart->pRxBuffPtr, huart->RxXferSize );
        dma_buf_num_char_copied  = ESP_RESP_RECV_BUF_SIZE -  dma_buf_cpy_offset_curr;
        eESPappendRecvRespISR( (huart->pRxBuffPtr + dma_buf_cpy_offset_curr), dma_buf_num_char_copied );
        dma_buf_cpy_offset_curr = 0;
    }
} // end of HAL_UART_RxCpltCallback






espRes_t   eESPlowLvlRecvStartFn()
{
    espRes_t  response = espOK;
    HAL_StatusTypeDef status_chk = HAL_ERROR;
    dma_buf_num_char_copied  = 0;
    dma_buf_cpy_offset_next  = 0;
    dma_buf_cpy_offset_curr  = 0;
    status_chk = HAL_UART_Receive_DMA( &haluart3, (uint8_t *)&recv_data_buf[0], ESP_RESP_RECV_BUF_SIZE );
    switch(status_chk) {
        case HAL_OK       : response = espOK;          break; 
        case HAL_ERROR    : response = espERR;         break; 
        case HAL_BUSY     : response = espBUSY;        break; 
        case HAL_TIMEOUT  : response = espTIMEOUT;     break; 
        default           : response = espERR;         break;
    }
    return response;
} // end of eESPlowLvlRecvStartFn



void    vESPlowLvlRecvStopFn( void )
{
    HAL_UART_DMAStop( &haluart3 );
    ESP_MEMSET( (void *)&recv_data_buf, 0x00, ESP_RESP_RECV_BUF_SIZE );
} // end of vESPlowLvlRecvStopFn




// the low-level functions that will be called by ESP core functions
espRes_t   eESPlowLvlSendFn( void* data, size_t len, uint32_t timeout )
{
    espRes_t  response = espOK;
    HAL_StatusTypeDef status_chk = HAL_ERROR;
    status_chk =  HAL_UART_Transmit( &haluart3, (uint8_t* )data, len, timeout ); 
    switch(status_chk) {
        case HAL_OK       : response = espOK;          break; 
        case HAL_ERROR    : response = espERR;         break; 
        case HAL_BUSY     : response = espBUSY;        break; 
        case HAL_TIMEOUT  : response = espTIMEOUT;     break; 
        default           : response = espERR;         break;
    }
    return response;
} // end of eESPlowLvlSendFn






// the low-level functions that will be called by ESP core functions
// for this test, PB4 (of STM32F4xx baord) is used as hardware reset pin of ESP device.
espRes_t   eESPlowLvlRstFn ( uint8_t state )
{
    // at here, state = 0 means reset assertion, non-zero value means reset de-assertion.
    GPIO_PinState pinstate = state==0x0 ? GPIO_PIN_RESET: GPIO_PIN_SET;
    HAL_GPIO_WritePin( GPIOB, GPIO_PIN_4, pinstate );
    return espOK;
} // end of eESPlowLvlRstFn




#undef   ESP_RESP_RECV_BUF_SIZE

