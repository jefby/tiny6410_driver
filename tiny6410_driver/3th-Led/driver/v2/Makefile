#如果已定义KERNELRELEASE,则说明是从内核构造系统调用的，因此可利用其内建语句
ifneq ($(KERNELRELEASE),)
	obj-m := Tiny6410_led.o
#否则，直接从命令行调用，这时要调用内核构造系统
else
	KERNELDIR ?= /opt/FriendlyARM/mini6410/linux/linux-2.6.38
	PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	rm *~ *.o *.ko *.mod.c 
endif

	










