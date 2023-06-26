KERNELDIR :=/home/share/samba/embed_linux/embeded_linux/sources/linux/alientek_linux
CURRENT_PATH := $(shell pwd)

obj-m := pinctrl_beep_drv.o
build: kernel_modules
kernel_modules: 
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
app:
	$(shell arm-none-linux-gnueabihf-gcc pinctrl_beep_app.c -o pinctrl_beep_app)
	$(shell cp pinctrl_beep_app pinctrl_beep_drv.ko ${CURRENT_PATH}/../../../images/rootfs/mnt) 