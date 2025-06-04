
HW4TST_C_SOURCES = \
    src/hardware/stm32f446/stm32f4xx_it.c \
    src/hardware/stm32f446/stm32f4xx_hal_msp.c \
    src/hardware/stm32f446/stm32f4xx_hal_timebase_tim.c \
    src/hardware/stm32f446/system_stm32f4xx.c  \
	src/hardware/stm32f446/stm32f446_config.c

HW4TST_ASM_SOURCES = src/hardware/stm32f446/startup_stm32f446xx.s

HW4TST_C_INCLUDES = include/hardware/stm32f446

HW4TST_LINK_SCRIPT = src/hardware/stm32f446/STM32F446RETx_FLASH.ld

