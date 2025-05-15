
RTOS_HW_BUILD_PATH =

DEBUG = 1

export ESP_PROJ_HOME = $(shell pwd)

REFMT_SRC_DIRS := ./include/ ./src  ./tests/integration/src/ ./tests/integration/include/

REFMT_EXTENSIONS := c h

REFMT_SRC_FILES := $(shell find $(REFMT_SRC_DIRS) -type f \( $(foreach ext,$(REFMT_EXTENSIONS),-name '*.$(ext)' -o ) -false \))


#######################################
# paths
#######################################
BUILD_DIR = $(ESP_PROJ_HOME)/build


integration_test:  tests/integration/build.mk  $(RTOS_HW_BUILD_PATH)  $(TOOLCHAIN_BASEPATH)
	@make -C $(RTOS_HW_BUILD_PATH)  startbuild \
		DEBUG=$(DEBUG)  BUILD_DIR=$(BUILD_DIR)  \
		APPCFG_PATH=$(ESP_PROJ_HOME)/tests/integration  \
		APP_NAME=$(APP_NAME) HW_PLATFORM=$(HW_PLATFORM) \
		OS=$(OS) TOOLCHAIN_BASEPATH=$(TOOLCHAIN_BASEPATH)

dbg_server: $(RTOS_HW_BUILD_PATH)
	@make -C $(RTOS_HW_BUILD_PATH)  dbg_server

dbg_client: $(RTOS_HW_BUILD_PATH)
	@make -C $(RTOS_HW_BUILD_PATH)  dbg_client \
		DBG_CUSTOM_SCRIPT_PATH=$(ESP_PROJ_HOME)/test-utility.gdb \
		TOOLCHAIN_BASEPATH=$(TOOLCHAIN_BASEPATH)


reformat:
	@clang-format-18 -i --style=file  $(REFMT_SRC_FILES)

clean:
	@rm -rf $(BUILD_DIR)

#######################################
# help documentation
#######################################
help:
	@echo "                                                      ";
	@echo " ---------------- Help Documentation -----------------";
	@echo "                                                      ";
	@echo " Options for building image, running, and debugging   ";
	@echo "                                                      ";

	@echo " * make INTEGRATION_TEST=yes TESTNAME=<test_name> OS_NAME=<os_name>  PLATFORM=<hw_platform_name> ";
	@echo "   Build image to run specified integration tests.    ";
	@echo "   where <test_name> can be 'ping', 'http_server', or ";
	@echo "   'mqtt_client'                                      ";
	@echo "                                                      ";
	@echo "   <os_name> : ";
	@echo "   so far we only integrate FreeRTOS to the ESP parser";
	@echo "   please refer to src/system/esp_system_freertos.c   ";
	@echo "   in order to port the ESP AT parser within your OS. ";
	@echo "                                                      ";
	@echo "   <hw_platform_name> : ";
	@echo "   so far this ESP parser is verified in STM32F4xx board, ";
	@echo "   in other words, ARM Cortex-M4 MCU platform, the default";
	@echo "   value is \"STM32F4\", but anyone is welcome to contribute  ";
	@echo "   to this repository with other hardware platforms that ";
	@echo "   haven't been implemented. ";
	@echo "                                                      ";
	@echo " * make dbg_server OPENOCD_HOME=/PATH/TO/YOUR_OPENOCD ";
	@echo "   launch debug server, we use OpenOCD (v0.10.0) here ";
	@echo "   . Note that superuser permission would be required ";
	@echo "   when running openOCD, the command differs & depends";
	@echo "   on your working Operating System.                  ";
	@echo "                                                      ";
	@echo " * make dbg_client                                    ";
	@echo "   launch GDB client to load image, set breakpoints,  ";
	@echo "   watchpoints for execution. We use gdb-multiarch    ";
	@echo "   (v7.7.1 or later) at here.                         ";
	@echo "                                                      ";
	@echo " * make clean                                         ";
	@echo "   clean up the built image                           ";
	@echo "                                                      ";
	@echo "                                                      ";


# *** EOF ***
