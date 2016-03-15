ifneq ($(KERNELRELEASE),)
obj-m    := virtualchar.o
else
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)
modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD)  modules
endif
clean:
	make -C $(KERNELDIR) M=$(PWD) clean
