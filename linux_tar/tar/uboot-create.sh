
#! /bin/sh

APP_PATH=$(pwd)
echo APP_PATH=$APP_PATH
UBOOT_SOURCE_PATH=${APP_PATH}/../../sources/u-boot/alientek_uboot
echo UF_A_SOURCE_PATH = $UF_A_SOURCE_PATH


compile_uboot_source(){
	if [ ! -d  ${UBOOT_SOURCE_PATH} ];then
		echo "start to compile u-boot"
		mkdir -p ${UBOOT_SOURCE_PATH}
		cd ${APP_PATH}
		tar -xvf u-boot-stm32mp-2020.01-g9c0df34f-v1.5.tar.bz2 -C ${UBOOT_SOURCE_PATH}		
	fi
	echo "alientek uboot create" 
	cd ${UBOOT_SOURCE_PATH}/
	echo $(pwd)
	make distclean
	make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- stm32mp157d_atk_defconfig
	make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- menuconfig
	make V=1 ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- DEVICE_TREE=stm32mp157d-atk all -j8
}

cp_atk_uboot_images(){
	if [ -f  ${UBOOT_SOURCE_PATH}/u-boot.stm32 ];then
		echo "cp u-boot bin"
		cp ${UBOOT_SOURCE_PATH}/u-boot.stm32 ${APP_PATH}/../../images/
	fi
}


compile_uboot_source
cp_atk_uboot_images
exit 0
#nfs c2000000 192.168.3.29:/home/share/samba/embed_linux/embeded_linux/images/uImage