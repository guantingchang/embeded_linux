KERNELDIR :=/home/share/samba/embed_linux/embeded_linux/sources/linux/alientek_linux
CURRENT_PATH := $(shell pwd)

obj-m := misc_beep_drv.o
build: kernel_modules
kernel_modules: 
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
app:
	$(shell arm-none-linux-gnueabihf-gcc misc_beep_app.c -o misc_beep_app)
	$(shell cp misc_beep_app misc_beep_drv.ko ${CURRENT_PATH}/../../../images/rootfs/mnt) 