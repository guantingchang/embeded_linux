KERNELDIR :=/home/share/samba/embed_linux/embeded_linux/sources/linux/alientek_linux
CURRENT_PATH := $(shell pwd)

obj-m := spinlock_led_drv.o
build: kernel_modules
kernel_modules: 
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
app:
	$(shell arm-none-linux-gnueabihf-gcc spinlock_led_app.c -o spinlock_led_app)
	$(shell cp spinlock_led_app spinlock_led_drv.ko ${CURRENT_PATH}/../../../../images/rootfs/mnt) 