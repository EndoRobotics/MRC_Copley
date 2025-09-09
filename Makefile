# Toolchain and flags
# CC := gcc
# CXX := g++
CXX ?= ../../../CUHK-EndoR/Software/CrossCompiler/arm-EndoROSv2-linux-gnueabihf_sdk-buildroot/bin/arm-EndoROSv2-linux-gnueabihf-g++
CC ?= $(CXX)
CXXFLAGS := -Wall -Wextra -O2 -std=c++11
INCLUDES := -Isrc -Isrc/osal -Isrc/osal/linux -Isrc/soem -Isrc/oshw/linux
LIBS := -lpcap

# Colors
GREEN := \033[1;32m
RESET := \033[0m

# Directories
SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin
TEST_DIR := test

# SOEM source files
SOEM_SRC := \
  $(SRC_DIR)/soem/ethercatbase.c \
  $(SRC_DIR)/soem/ethercatcoe.c \
  $(SRC_DIR)/soem/ethercatconfig.c \
  $(SRC_DIR)/soem/ethercatdc.c \
  $(SRC_DIR)/soem/ethercateoe.c \
  $(SRC_DIR)/soem/ethercatfoe.c \
  $(SRC_DIR)/soem/ethercatmain.c \
  $(SRC_DIR)/soem/ethercatprint.c \
  $(SRC_DIR)/soem/ethercatsoe.c \
  $(SRC_DIR)/oshw/linux/nicdrv.c \
  $(SRC_DIR)/oshw/linux/oshw.c \
  $(SRC_DIR)/osal/linux/osal.c

SOEM_OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOEM_SRC))

# Find all test .c files (like test/copley_spin/copley_spin.c)
TEST_C_FILES := $(wildcard $(TEST_DIR)/*/*.c)
TOTAL_TESTS := $(words $(TEST_C_FILES))

# Build object files for test sources
TEST_OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(TEST_C_FILES))

# Build output binary paths (e.g. test/copley_spin/copley_spin.c → bin/copley_spin)
TEST_BINS := $(patsubst $(TEST_DIR)/%.c,$(BIN_DIR)/%,$(TEST_C_FILES))

# Default target
all: $(TEST_BINS)
	@echo ""
	@echo "$(GREEN)[SUCCESS] All targets built successfully!$(RESET)"

# Compile SOEM source files
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "[Compiling] $<"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile test sources with progress count
TEST_INDEX := 0
define compile_test_template
$(BUILD_DIR)/$(TEST_DIR)/$(notdir $(basename $(1))).o: $(1)
	@mkdir -p $$(dir $$@)
	$$(eval TEST_INDEX := $$(shell echo $$(( $$(TEST_INDEX) + 1 )) ))
	@echo "[$$(TEST_INDEX)/$(TOTAL_TESTS) Compiling test] $(1)"
	$$(CC) $$(CFLAGS) $$(INCLUDES) -c $$< -o $$@
endef

$(foreach file,$(TEST_C_FILES),$(eval $(call compile_test_template,$(file))))

# Link test binaries
$(BIN_DIR)/%: $(BUILD_DIR)/$(TEST_DIR)/%.o $(SOEM_OBJ)
	@mkdir -p $(dir $@)
	@echo "[Linking] $@"
	$(CXX) -o $@ $^ $(LIBS)

# Clean all builds
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all clean
