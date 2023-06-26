KERNELDIR :=/home/share/samba/embed_linux/embeded_linux/sources/linux/alientek_linux
CURRENT_PATH := $(shell pwd)

obj-m := key_irq_noblock_drv.o
build: kernel_modules
kernel_modules: 
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
app:
	$(shell arm-none-linux-gnueabihf-gcc key_irq_noblock_app.c -o key_irq_noblock_app)
	$(shell cp key_irq_noblock_app key_irq_noblock_drv.ko ${CURRENT_PATH}/../../../../images/rootfs/mnt) 