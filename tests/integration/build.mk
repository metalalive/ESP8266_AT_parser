ITEST_REL_PATH = tests/integration
ITEST_FULLPATH = $(ESP_PROJ_HOME)/$(ITEST_REL_PATH)

include  $(ESP_PROJ_HOME)/common.mk
# TODO, parameterize for support different hardware platforms and OS
include  $(ITEST_FULLPATH)/src/hardware/stm32f446/build.mk

ITEST_C_SOURCES = \
	src/demo_apps/common.c      \
	src/entry.c

include  $(ITEST_FULLPATH)/src/demo_apps/$(APP_NAME).mk

APPCFG_C_SOURCES = \
  $(ESP_C_SOURCES) \
  $(addprefix $(ITEST_FULLPATH)/, $(ITEST_C_SOURCES)) \
  $(addprefix $(ITEST_FULLPATH)/, $(HW4TST_C_SOURCES))

APPCFG_C_INCLUDES = \
  $(ESP_C_INCLUDES) \
  $(ITEST_FULLPATH)/include \
  $(ITEST_FULLPATH)/include/os \
  $(addprefix $(ITEST_FULLPATH)/, $(HW4TST_C_INCLUDES)) \

APPCFG_ASM_SOURCES = $(addprefix $(ITEST_FULLPATH)/, $(HW4TST_ASM_SOURCES))

APP_LINK_SCRIPT = $(ITEST_FULLPATH)/$(HW4TST_LINK_SCRIPT)

