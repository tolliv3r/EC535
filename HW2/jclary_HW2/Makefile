# Makefile for EC535 HW2 - Instruction Set Simulator

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
TARGET = myISS
SOURCE = myISS.c

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

# Clean up generated files
clean:
	rm -f $(TARGET)

# Phony targets
.PHONY: all clean
