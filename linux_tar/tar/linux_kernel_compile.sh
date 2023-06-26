
#! /bin/sh

APP_PATH=$(pwd)
echo APP_PATH=$APP_PATH
LINUX_KERNEL_SOURCE_PATH=${APP_PATH}/../../sources/linux/alientek_linux
echo UBOOT_SOURCE_PATH = $LINUX_KERNEL_SOURCE_PATH


compile_linux_kernel_source(){
	if [ ! -d  ${LINUX_KERNEL_SOURCE_PATH} ];then
		echo "start to create linux kernel"
		mkdir -p ${LINUX_KERNEL_SOURCE_PATH}
		cd ${APP_PATH}
		tar -xvf linux-5.4.31-g8c3068500-v1.8.tar.bz2 -C ${LINUX_KERNEL_SOURCE_PATH}		
	fi
	echo "alientek linux kernel compile" 
	cd ${LINUX_KERNEL_SOURCE_PATH}/
	echo $(pwd)
	make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- distclean
	make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- stm32mp1_atk_defconfig
	make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- menuconfig
	make V=1 ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- uImage dtbs LOADADDR=0xc2000040 -j8
}

cp_atk_linux_kernel_images(){
	if [ -f  ${LINUX_KERNEL_SOURCE_PATH}/arch/arm/boot/uImage ];then
		echo "cp linux_kernel bin"
		cp ${LINUX_KERNEL_SOURCE_PATH}/arch/arm/boot/uImage ${APP_PATH}/../../images/
	fi
	if [ -f  ${LINUX_KERNEL_SOURCE_PATH}/arch/arm/boot/dts/stm32mp157d-atk.dtb ];then
		echo "cp linux dtb"
		cp ${LINUX_KERNEL_SOURCE_PATH}/arch/arm/boot/dts/stm32mp157d-atk.dtb ${APP_PATH}/../../images/
	fi
}


compile_linux_kernel_source
cp_atk_linux_kernel_images
exit 0
#nfs c2000000 192.168.3.29:/home/share/samba/embed_linux/embeded_linux/images/uImage