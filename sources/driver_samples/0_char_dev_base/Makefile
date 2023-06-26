KERNELDIR :=/home/share/samba/embed_linux/embeded_linux/sources/linux/alientek_linux
CURRENT_PATH := $(shell pwd)

obj-m := chrdevbase_drv.o
build: kernel_modules
kernel_modules: 
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
app:
	$(shell arm-none-linux-gnueabihf-gcc chrdevbase_app.c -o chrdevbase_app)
	$(shell cp chrdevbase_app chrdevbase_drv.ko ${CURRENT_PATH}/../../../images/rootfs/mnt) 