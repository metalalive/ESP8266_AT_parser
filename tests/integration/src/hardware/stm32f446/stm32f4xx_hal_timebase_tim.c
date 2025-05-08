#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"
#include "FreeRTOS.h"
 
TIM_HandleTypeDef  htim2;

// In STM32 HAL,  `HAL_InitTick()` is invoked before system clock initialization ,
// it is improper to overwrite the function for initializing other timers on STM32
// board

extern void Error_Handler(void);

void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /**Configure the main internal regulator output voltage */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
  /**Initializes the CPU, AHB and APB busses clocks  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }
  /**Initializes the CPU, AHB and APB busses clocks */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

HAL_StatusTypeDef  TIM2_Init(uint32_t TickPriority) {
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    RCC_ClkInitTypeDef    clkconfig;
    HAL_StatusTypeDef     result = HAL_OK;
    uint32_t   uwTimclock = 0, pFLatency = 0;
  
    HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);
    uwTimclock = HAL_RCC_GetPCLK1Freq();
   
    // in this test application, TIM2 period is the same as the interval
    // equal to time duration between every 2 consecutive ticks. 
    htim2.Instance = TIM2;
    htim2.Init.Period = (uwTimclock / configTICK_RATE_HZ) - 1;
    htim2.Init.Prescaler = 0;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.InterruptGrpPriority = TickPriority;
    htim2.InterruptSubPriority = 0;
    result = HAL_TIM_Base_Init(&htim2);
    if(result != HAL_OK) {
        return result;
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    result = HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig);
    if(result != HAL_OK) {
        return result;
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    result = HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig);
    if(result != HAL_OK) {
        return result;
    }
    return HAL_TIM_Base_Start_IT(&htim2);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2) {
    HAL_IncTick();
  }
}
