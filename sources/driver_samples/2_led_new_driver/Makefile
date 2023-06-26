KERNELDIR :=/home/share/samba/embed_linux/embeded_linux/sources/linux/alientek_linux
CURRENT_PATH := $(shell pwd)

obj-m := char_led_drv.o
build: kernel_modules
kernel_modules: 
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
app:
	$(shell arm-none-linux-gnueabihf-gcc char_led_app.c -o char_led_app)
	$(shell cp char_led_app char_led_drv.ko ${CURRENT_PATH}/../../../images/rootfs/mnt) 