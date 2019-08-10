

# included C source code, header files
C_SOURCES += \
    Src/stm32f4xx_it.c \
    Src/stm32f4xx_hal_msp.c \
    Src/stm32f4xx_hal_timebase_tim.c \
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c \
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c \
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c \
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c \
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c \
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c \
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c \
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c \
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c \
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c \
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c \
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c \
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c \
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c \
    Src/system_stm32f4xx.c 


ifeq ($(INTEGRATION_TEST), yes)
    C_SOURCES += \
            Src/tests/integration/ESP_AT_parser/port/stm32f446_nucleo_hal.c 
endif 




# ASM sources
ASM_SOURCES += startup_stm32f446xx.s


# C defines
C_DEFS +=  \
-DUSE_HAL_DRIVER \
-DSTM32F446xx 


C_INCLUDES +=  \
-IDrivers/STM32F4xx_HAL_Driver/Inc \
-IDrivers/STM32F4xx_HAL_Driver/Inc/Legacy \
-IDrivers/CMSIS/Device/ST/STM32F4xx/Include \
-IDrivers/CMSIS/Include 




# toolchain setup
C_TOOLCHAIN_PREFIX = arm-none-eabi-


#######################################
# CFLAGS
#######################################
# cpu
CPU = -mcpu=cortex-m4

# fpu
FPU = -mfpu=fpv4-sp-d16

# float-abi
FLOAT-ABI = -mfloat-abi=hard

# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)


# link script
LDSCRIPT = STM32F446RETx_FLASH.ld



# Debug
OPENOCD_CFG_FILES =  -f $(OPENOCD_HOME)/tcl/board/st_nucleo_f4.cfg  -f $(OPENOCD_HOME)/tcl/interface/stlink-v2-1.cfg 





