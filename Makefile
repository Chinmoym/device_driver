ifneq ($(KERNELRELEASE),)

obj-m := gpio_ir.o

else
KDIR  := /lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)

default:
	make -C $(KDIR) M=$(PWD) modules
clean:
	-rm -fr *.mod.c *.o .*.cmd Module.symvers modules.order .tmp_versions || :

.PHONY: clean

endif

