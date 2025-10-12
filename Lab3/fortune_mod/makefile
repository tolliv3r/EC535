obj-m	+= fortune.o

KDIR	?= $(EC535)/stock/linux-4.19.82-ti-rt-r33

PWD	:= $(shell pwd)

all:
	$(MAKE) -C  $(KDIR) M=$(PWD) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
