KERNELDIR :=/home/share/samba/mp157_linux/embed_linux/embed_linux/sources/linux/linux-stm32mp-5.4.31-r0/linux-5.4.31
CURRENT_PATH := $(shell pwd)

obj-m := chrdevbase.o
build: kernel_modules
kernel_modules: 
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
