
C_SOURCES +=  \
    Src/FreeRTOS/Source/portable/MemMang/heap_4.c \
    Src/FreeRTOS/Source/portable/GCC/ARM_CM4_MPU/port.c \
    Src/FreeRTOS/Source/croutine.c      \
    Src/FreeRTOS/Source/event_groups.c  \
    Src/FreeRTOS/Source/list.c          \
    Src/FreeRTOS/Source/queue.c         \
    Src/FreeRTOS/Source/stream_buffer.c \
    Src/FreeRTOS/Source/tasks.c         \
    Src/FreeRTOS/Source/timers.c        \
    Src/ESP_AT_parser/src/system/esp_system_freertos.c


C_INCLUDES +=  \
    -ISrc/FreeRTOS/Source/include \
    -ISrc/FreeRTOS/Source/portable/GCC/ARM_CM4_MPU 




