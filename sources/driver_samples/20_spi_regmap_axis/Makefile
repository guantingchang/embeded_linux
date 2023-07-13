CURRENT_PATH := $(shell pwd)
KERNELDIR := $(CURRENT_PATH)/../../linux/alientek_linux
obj-m := icm20608_remap_drv.o
build: kernel_modules
kernel_modules: 
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	$(shell rm -rf icm20608_remap_app ) 
app:
	$(shell arm-none-linux-gnueabihf-gcc icm20608_remap_app.c -o icm20608_remap_app)
	$(shell cp  icm20608_remap_drv.ko ${CURRENT_PATH}/../../../images/rootfs/buildroot/mnt) 
	$(shell cp icm20608_remap_app  ${CURRENT_PATH}/../../../images/rootfs/buildroot/mnt)	                                      
	