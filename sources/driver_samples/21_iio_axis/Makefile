CURRENT_PATH := $(shell pwd)
KERNELDIR := $(CURRENT_PATH)/../../linux/alientek_linux
obj-m := icm20608_drv.o
build: kernel_modules
kernel_modules: 
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
app:
	$(shell arm-none-linux-gnueabihf-gcc icm20608_app.c -o icm20608_app)
	$(shell cp icm20608_app icm20608_drv.ko ${CURRENT_PATH}/../../../images/rootfs/mnt) 