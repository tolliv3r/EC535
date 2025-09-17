# Compiler
CC = gcc

# Executables
APP1 = MyBitApp
APP2 = PriorityList

# Source files
APP1_SRCS = main.c bits.c list.c
APP2_SRCS = main.c bits.c list.c

# Object files
APP1_OBJS = $(APP1_SRCS:.c=.o)
APP2_OBJS = $(APP2_SRCS:.c=.o)

# Default target
all: $(APP1) $(APP2)

$(APP1): $(APP1_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(APP2): $(APP2_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Pattern rule for .o from .c
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Dependencies
main.o: bits.h list.h
bits.o: bits.h
list.o: list.h

# clean rule
clean:
	rm -f $(APP1) $(APP2) *.o
