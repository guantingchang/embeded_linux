KERNELDIR :=/home/share/samba/embed_linux/embeded_linux/sources/linux/alientek_linux
CURRENT_PATH := $(shell pwd)

obj-m := ap3216c_drv.o
build: kernel_modules
kernel_modules: 
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
app:
	$(shell arm-none-linux-gnueabihf-gcc ap3216c_app.c -o ap3216c_app)
	$(shell cp ap3216c_app ap3216c_drv.ko ${CURRENT_PATH}/../../../images/rootfs/mnt) 