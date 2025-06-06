RTOS_HW_BUILD_PATH =

DEBUG = 1

export ESP_PROJ_HOME = $(shell pwd)

UTEST_UNITY_PATH =

REFMT_SRC_DIRS := ./include/ ./src  ./tests/unit/ ./tests/integration/src/ ./tests/integration/include/

REFMT_EXTENSIONS := c h

REFMT_SRC_FILES := $(shell find $(REFMT_SRC_DIRS) -type f \( $(foreach ext,$(REFMT_EXTENSIONS),-name '*.$(ext)' -o ) -false \))


#######################################
# paths
#######################################
BUILD_DIR = $(ESP_PROJ_HOME)/build


#######################################
# targets
#######################################
unit_test:
	@make -C ./tests/unit -f build.mk \
		UTEST_UNITY_PATH="$(UTEST_UNITY_PATH)" \
		ESP_PROJ_HOME="$(ESP_PROJ_HOME)" \
		DEBUG=$(DEBUG)  BUILD_DIR="$(BUILD_DIR)"

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
	@cat ./docs/build-help-doc

# *** EOF ***
