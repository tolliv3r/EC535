obj-m := mytimer.o

KDIR ?= $(WORKSPACE)/stock-linux-4.19.82-ti-rt-r33
PWD  := $(shell pwd)

ARCH ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-


all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean