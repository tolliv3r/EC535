# Makefile for EC535 HW2 - Instruction Set Simulator

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
TARGET = myISS
SOURCE = myISS.c

# Add the phony to keep overlapping files from breaking build
.PHONY: all build run profile clean

# Default target
all: build

# Build target
build: $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

# Run target - builds and runs with sample.assembly
run: build
	./$(TARGET) sample.assembly

# Profile target - builds with profiling and runs gprof
profile: $(SOURCE)
	$(CC) -std=c11 -pg -Wall -Wextra $(SOURCE) -o $(TARGET).profile
	./$(TARGET).profile sample.assembly
	gprof -p $(TARGET).profile gmon.out

# Clean up generated files
clean:
	rm -f $(TARGET) $(TARGET).profile
