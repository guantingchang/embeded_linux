KERNELDIR :=/home/share/samba/embed_linux/embeded_linux/sources/linux/alientek_linux
CURRENT_PATH := $(shell pwd)

obj-m := led_platform_dts_drv.o
build: kernel_modules
kernel_modules: 
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
app:
	$(shell arm-none-linux-gnueabihf-gcc led_platform_dts_app.c -o led_platform_dts_app)
	$(shell cp led_platform_dts_app led_platform_dts_drv.ko ${CURRENT_PATH}/../../../images/rootfs/mnt) 