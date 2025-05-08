#include "hal_init.h"
#include "stm32f4xx_it.h"

extern TIM_HandleTypeDef htim2;

void NMI_Handler(void) {}

__weak void MemManage_Handler(void) {
  while(1);
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  while (1) {}
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void) {
  while (1) {}
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */
  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */
  /* USER CODE END DebugMonitor_IRQn 1 */
}


void TIM2_IRQHandler(void) {
  HAL_TIM_IRQHandler(&htim2);
}

