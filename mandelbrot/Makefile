# Constants
BUILD_DIR = ./build
SRC_DIR = ./src

# Compiler Flags
CC      = gcc
MPICC   = mpicc
CFLAGS = -g -O3 -lm

# Sources
SOURCES := $(wildcard $(SRC_DIR)/mb_*.c)

# Binaries
BINS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%, $(SOURCES))

.PHONY: all clean

all: $(BINS)

$(BUILD_DIR)/mb_%: $(SRC_DIR)/mb_%.c | $(BUILD_DIR)
	$(MPICC) $(CFLAGS) $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
