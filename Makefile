# Constants
BUILD_DIR = build
SOURCE_DIR = src
SERIAL_DIR = $(SOURCE_DIR)/serial

# Compiler Definitions
CC = cc
CCFLAGS = -g -O3

# Source files
SOURCES = $(SERIAL_DIR)/serial_1.c \
		  $(SERIAL_DIR)/serial_2.c


# Executables
TARGETS = $(SOURCES:.c=.x)

all: $(BUILD_DIR) $(TARGETS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

%.x: %.c
	$(CC) $(CCFLAGS) -o $(BUILD_DIR)/$@ $^

.PHONY: clean
clean:
	rm -f *.x
