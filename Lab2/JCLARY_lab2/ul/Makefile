CROSS_COMPILE	?= arm-linux-gnueabihf-
CC		:= $(CROSS_COMPILE)gcc
STRIP		:= $(CROSS_COMPILE)strip

WORKSPACE	?= $(HOME)/EC535/lab2
ROOTFS		:= $(WORKSPACE)/rootfs

TARGET	:= ktimer
SRC	:= ktimer.c

CFLAGS	?= -O2 -Wall -Wextra -std=c11
LDFLAGS	?= -static

.PHONY: all clean install-root elf

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)
	$(STRIP) --strip-unneeded $@

clean:
	$(RM) $(TARGET)
