# Compiler
CC = gcc
# Compiler flags
CFLAGS = -fPIC
# Source files
SRCS = ../hd_utils.c ../hd_ipc.c ../hd_logger.c ../hd_service_interface.c ../cJSON.c
# Object files (from source files)
OBJS = $(SRCS:../%.c=%.o)
# Shared library name
TARGET = libhsi.so
# Directory for build
BUILD_DIR = ./build/

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(addprefix $(BUILD_DIR)/, $(OBJS))
	$(CC) -shared -o $@ $^

$(BUILD_DIR)/%.o: ../%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
