/* USER CODE BEGIN Header */
// ------------------------------------------------------------------------------------------
// start running all integration tests, each sub-routine tasks handles one integration test.
// In each integration task, it will do the following :
//
//     * create several tasks interacting with FreeRTOS kernel objects / functions like ...
//         * 
//         * 
//         * 
//         * 
//         * 
//
//     * it also sets up a bunch of checkpoints at some places in the test code,
//       to see if the actual result matches the expected value
//
//     * Based on the checkpoints previously described, there are error flags to record
//       the mismatches between actual result value and expected value, 
//       if you're using external debugger when running the tests, you can simply set the
//       error flags as watchpoints to see whether any of the error flags is asserted
//       during integretion test
//
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __INTEGRATION_TEST_ENTRY_H
#define __INTEGRATION_TEST_ENTRY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "tests/testlogger.h"
#include "tests/integration/ESP_AT_parser/test_runner.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */
/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
/* USER CODE END ET */
/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
/* USER CODE END EC */
/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#define  vIntegrationTestISR1  TIM3_IRQHandler
#define  vIntegrationTestISR2  TIM4_IRQHandler
#define  vIntegrationTestMemManageHandler  MemManage_Handler
/* USER CODE END EM */
/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

void vIntegrationTestISR1( void );

void vIntegrationTestISR2( void );

void vIntegrationTestMemManageHandler(void);

/* USER CODE BEGIN EFP */
/* USER CODE END EFP */
/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN Private defines */
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __INTEGRATION_TEST_ENTRY_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
