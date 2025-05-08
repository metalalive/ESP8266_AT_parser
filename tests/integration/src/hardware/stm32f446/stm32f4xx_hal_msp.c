#include "hal_init.h"

/**
  * Initializes the Global MSP.
  */
void HAL_MspInit(void) {
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();
}

/**
* @brief TIM_Base MSP Initialization
* This function configures the hardware resources used in this example
* @param htim_base: TIM_Base handle pointer
* @retval None
*/
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim_base) {
  if(htim_base->Instance==TIM2) {
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_DBGMCU_FREEZE_TIM2();
    HAL_NVIC_SetPriority(TIM2_IRQn, htim_base->InterruptGrpPriority, htim_base->InterruptSubPriority);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
  }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* htim_base) {
  if(htim_base->Instance==TIM2) {
    HAL_NVIC_DisableIRQ(TIM2_IRQn);
    __HAL_DBGMCU_UNFREEZE_TIM2();
    __HAL_RCC_TIM2_CLK_DISABLE();
  }
}
