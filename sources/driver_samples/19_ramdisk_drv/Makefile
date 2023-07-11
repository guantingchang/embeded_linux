KERNELDIR :=/home/share/samba/embed_linux/embeded_linux/sources/linux/alientek_linux
CURRENT_PATH := $(shell pwd)

obj-m := ramdisk_request_drv.o ramdisk_norequest_drv.o
build: kernel_modules
kernel_modules: 
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
	$(shell cp  ramdisk_request_drv.ko ${CURRENT_PATH}/../../../images/rootfs/buildroot/mnt) 
	$(shell cp  ramdisk_norequest_drv.ko ${CURRENT_PATH}/../../../images/rootfs/buildroot/mnt) 
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean


	