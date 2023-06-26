KERNELDIR :=/home/share/samba/embed_linux/embeded_linux/sources/linux/alientek_linux
CURRENT_PATH := $(shell pwd)

obj-m := led_platform_drv.o
obj-m += led_platform_device.o
build: kernel_modules
kernel_modules: 
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
app:
	$(shell arm-none-linux-gnueabihf-gcc led_platform_app.c -o led_platform_app)
	$(shell cp led_platform_app led_platform_device.ko led_platform_drv.ko ${CURRENT_PATH}/../../../images/rootfs/mnt) 