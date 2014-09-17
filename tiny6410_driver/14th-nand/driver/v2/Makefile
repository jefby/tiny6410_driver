ifneq ($(KERNELRELEASE),)
	obj-m := nand.o
else
	KERNELDIR ?= /opt/FriendlyARM/mini6410/linux/linux-2.6.38
	PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	rm -rf *.ko *.o *.order *~ *symvers *.mod.c *.bak
endif

 
