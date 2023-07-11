KERNELDIR :=/home/share/samba/embed_linux/embeded_linux/sources/linux/alientek_linux
CURRENT_PATH := $(shell pwd)

obj-m := ramdisk_drv.o
build: kernel_modules
kernel_modules: 
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
	$(shell cp  ramdisk_drv.ko ${CURRENT_PATH}/../../../images/rootfs/mnt) 
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean


	