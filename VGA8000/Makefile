ifneq ($(KERNELRELEASE),)
obj-m := ch7033.o
else
KERNELDIR := ~/work/svn/AM37x_EVM/kernel/linux-2.6.32-android
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif

clean:
	rm -f *.o *.ko *.mod.c modules.* Module.*
