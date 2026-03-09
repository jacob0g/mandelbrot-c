# Constants
BUILD_DIR = ./build
INCLUDE_DIR = ./include
SRC_DIR = ./src
SERIAL_DIR = $(SRC_DIR)/serial
MPI_DIR = $(SRC_DIR)/mpi

# Compiler Flags
CC      = gcc
MPICC   = mpicc
CFLAGS = -g -O3 -lm -fsanitize=address 

# File IO Flag
ifdef FILE_IO
	CFLAGS += -DFILE_IO
endif

# Benchmark Flag
ifdef BENCHMARK
	CFLAGS += -DBENCHMARK
endif

# Append includes to CFLAGS
CFLAGS += -I$(INCLUDE_DIR)

# Sources
SERIAL_SRCS := $(wildcard $(SERIAL_DIR)/serial_*.c)
MPI_SRCS := $(wildcard $(MPI_DIR)/mpi_*.c)

# Binaries
SERIAL_BINS := $(patsubst $(SERIAL_DIR)/%.c, $(BUILD_DIR)/%, $(SERIAL_SRCS))
MPI_BINS := $(patsubst $(MPI_DIR)/%.c, $(BUILD_DIR)/%, $(MPI_SRCS))

.PHONY: all serial mpi clean

all: serial mpi

serial: $(SERIAL_BINS)

mpi: $(MPI_BINS)

$(BUILD_DIR)/serial_%: $(SERIAL_DIR)/serial_%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/mpi_%: $(MPI_DIR)/mpi_%.c | $(BUILD_DIR)
	$(MPICC) $(CFLAGS) $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) *.ppm
