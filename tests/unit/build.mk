include  $(ESP_PROJ_HOME)/common.mk

# Variables passed from the top-level Makefile
# UTEST_UNITY_PATH: Path to the Unity framework project home
# ESP_PROJ_HOME: Root directory of the main project
# BUILD_DIR: Main build output directory

# Unit test specific variables
UNIT_TEST_SRC_DIRS := $(ESP_PROJ_HOME)/tests/unit

UNITY_SRC_REL_DIRS := src  extras/fixture/src  extras/memory/src
UNITY_SRC_DIRS :=  $(addprefix $(UTEST_UNITY_PATH)/, $(UNITY_SRC_REL_DIRS))

# Collect all C source files
_UNIT_TEST_C_SRCS_REL := $(shell find $(UNIT_TEST_SRC_DIRS) -name '*.c')
_UNITY_C_SRCS_REL := $(shell find $(UNITY_SRC_DIRS) -name '*.c')

# Convert relative source paths to absolute paths
ALL_ESP_LIB_SRCS_ABS := $(ESP_C_SOURCES)
ALL_APP_C_SRCS_ABS := $(abspath $(_UNIT_TEST_C_SRCS_REL))
ALL_UNITY_C_SRCS_ABS := $(abspath $(_UNITY_C_SRCS_REL))

# Define the directory for compiled object files
UNIT_TEST_OBJ_DIR := $(BUILD_DIR)/unit_test_objs
UNITY_OBJ_SUBDIR  := $(UNIT_TEST_OBJ_DIR)/unity

# Generate the list of object files for application unit tests
# e.g., /path/to/project/build/unit_test_objs/tests/unit/my_test.o
UNIT_TEST_OBJS_APP := $(patsubst $(ESP_PROJ_HOME)/%,$(UNIT_TEST_OBJ_DIR)/%,$(ALL_APP_C_SRCS_ABS:.c=.o))

UNIT_TEST_OBJS_ESPLIB := $(patsubst $(ESP_PROJ_HOME)/%,$(UNIT_TEST_OBJ_DIR)/%,$(ALL_ESP_LIB_SRCS_ABS:.c=.o))

# Generate the list of object files for Unity framework sources
# e.g., /path/to/project/build/unit_test_objs/unity/src/unity.o
UNIT_TEST_OBJS_UNITY := $(patsubst $(UTEST_UNITY_PATH)/%,$(UNITY_OBJ_SUBDIR)/%,$(ALL_UNITY_C_SRCS_ABS:.c=.o))

# Combine all object files
UNIT_TEST_OBJS := $(UNIT_TEST_OBJS_ESPLIB) $(UNIT_TEST_OBJS_APP) $(UNIT_TEST_OBJS_UNITY)

# Define the name of the final unit test executable
UNIT_TEST_EXEC := $(BUILD_DIR)/unit_test_runner

# Host compiler and flags for unit tests
CC := gcc
CFLAGS_UNIT_TEST := -Wall -Wextra -g
# Add include paths: project common headers, Unity headers, and unit test specific headers
CFLAGS_UNIT_TEST += -I$(ESP_PROJ_HOME)/include $(addprefix -I, $(UNITY_SRC_DIRS)) -I$(ESP_PROJ_HOME)/tests/unit
LDFLAGS_UNIT_TEST := -lm # Link with math library, common for C projects


#######################################
# targets
#######################################
run_unit_test: $(UNIT_TEST_OBJ_DIR) $(UNITY_OBJ_SUBDIR) $(UNIT_TEST_EXEC)
	@$(UNIT_TEST_EXEC) -v

# Rule to link the final unit test executable
$(UNIT_TEST_EXEC): $(UNIT_TEST_OBJS)
	@echo "Linking unit test executable: $@"
	@mkdir -p $(@D)
	$(CC) $(UNIT_TEST_OBJS) -o $@ $(LDFLAGS_UNIT_TEST)

# Generic rule to compile application unit test C source files into object files
# This rule takes an object file path like build/unit_test_objs/tests/unit/my_test.o
# and derives the source file path like /home/user/project/tests/unit/my_test.c
$(UNIT_TEST_OBJ_DIR)/%.o: $(ESP_PROJ_HOME)/%.c
	@echo "Compiling application unit test source $< to $@"
	@mkdir -p $(dir $@) # Ensure the output directory exists
	$(CC) $(CFLAGS_UNIT_TEST) -c $< -o $@

# Generic rule to compile Unity framework C source files into object files
# This rule takes an object file path like build/unit_test_objs/unity/src/unity.o
# and derives the source file path like /path/to/unity/src/unity.c
$(UNITY_OBJ_SUBDIR)/%.o: $(UTEST_UNITY_PATH)/%.c
	@echo "Compiling Unity source $< to $@"
	@mkdir -p $(dir $@) # Ensure the output directory exists
	$(CC) $(CFLAGS_UNIT_TEST) -c $< -o $@

# Phony target for the object directories to ensure they are created before compilation
$(UNIT_TEST_OBJ_DIR):
	@mkdir -p $(UNIT_TEST_OBJ_DIR)

$(UNITY_OBJ_SUBDIR):
	@mkdir -p $(UNITY_OBJ_SUBDIR)

.PHONY: run_unit_test
