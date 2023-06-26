
#! /bin/sh

APP_PATH=$(pwd)
echo APP_PATH=$APP_PATH
UF_A_SOURCE_PATH=${APP_PATH}/../../sources/atk_tf_a
UBOOT_SOURCE_PATH=${APP_PATH}/../../sources/atk_uboot
LINUX_KERNEL_PATH=${APP_PATH}/../../sources/atk_linux_kernel
echo UF_A_SOURCE_PATH = $UF_A_SOURCE_PATH

create_linux_source(){
	if [ -d  ${UF_A_SOURCE_PATH} ];then
		echo -e  "${GREEN_COLOR}uf-a source directory is exist.${END_COLOR}"
	else
		echo -e "${RED_COLOR}uf-a source directory is not exist!${END_COLOR}"
		mkdir -p ${UF_A_SOURCE_PATH}
		mkdir -p ${UBOOT_SOURCE_PATH}
		mkdir -p ${LINUX_SOURCE_PATH}
		tar -xvf en.SOURCES-stm32mp1-openstlinux-5-4-dunfell-mp1-20-06-24.tar.xz -C ${UF_A_SOURCE_PATH}/../
		cd ${UF_A_SOURCE_PATH}/../
		ls -al	
		mv  stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/tf-a-stm32mp-2.2.r1-r0 ${UF_A_SOURCE_PATH}
		mv  stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/optee-os-stm32mp-3.9.0.r1-r0 ${UF_A_SOURCE_PATH}
		mv  stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/tf-a-stm32mp-ssp-2.2.r1-r0 ${UF_A_SOURCE_PATH}	
		mv  stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/u-boot-stm32mp-2020.01-r0 ${UBOOT_SOURCE_PATH}
		mv  stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/linux-stm32mp-5.4.31-r0 ${LINUX_KERNEL_PATH}
		rmdir -p stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/
	fi
}

patch_uf_a_source(){
    cd ${UF_A_SOURCE_PATH}/tf-a-stm32mp-2.2.r1-r0
	ls -al
	if [ -f  ./tf-a-stm32mp-2.2.r1-r0.tar.gz ];then
		tar -vxf tf-a-stm32mp-2.2.r1-r0.tar.gz
		cd tf-a-stm32mp-2.2.r1
		for p in `ls -1 ../*.patch`; do patch -p1 < $p; done
		echo "stm32 tf-a patch success!"
		rm -rf ../*.patch
		rm -rf ../tf-a-stm32mp-2.2.r1-r0.tar.gz
	fi
}

compile_tf_a_source(){
	if [ ! -d  ${UF_A_SOURCE_PATH}/../tf-a/alientek_tf-a ];then
		echo "start to compile tf-a"
		sudo apt-get install device-tree-compiler
		mkdir -p ${UF_A_SOURCE_PATH}/../tf-a/alientek_tf-a
		cd ${APP_PATH}
		tar -xvf tf-a-stm32mp-2.2.r1-gaa9f87c-v1.5.tar.bz2 -C ${UF_A_SOURCE_PATH}/../tf-a/alientek_tf-a/		
	fi
	echo "alientek tf-a create" 
	cd ${UF_A_SOURCE_PATH}/../tf-a/alientek_tf-a/tf-a-stm32mp-2.2.r1
	echo $(pwd)
	find ${UF_A_SOURCE_PATH}/../tf-a/alientek_tf-a -name Makefile.sdk | xargs sed -i 's/CROSS_COMPILE=arm-ostl-linux-gnueabi-/CROSS_COMPILE=arm-none-linux-gnueabihf-/g'
	make -f ../Makefile.sdk all
}

cp_tf_a_images(){
	if [ -f  ${UF_A_SOURCE_PATH}/../tf-a/alientek_tf-a/build/trusted/tf-a-stm32mp157d-atk-trusted.stm32 ];then
		echo "start to cp atk_tf_a bin"
		cp ${UF_A_SOURCE_PATH}/../tf-a/alientek_tf-a/build/trusted/tf-a-stm32mp157d-atk-trusted.stm32 ${APP_PATH}/../../images/
	fi
}


compile_st_tf_a_source(){
		echo "st tf-a compile" 
		echo $(pwd)
		cd ${UF_A_SOURCE_PATH}/tf-a-stm32mp-2.2.r1-r0/tf-a-stm32mp-2.2.r1		
		find ${UF_A_SOURCE_PATH}/tf-a-stm32mp-2.2.r1-r0/ -name Makefile.sdk | xargs sed -i 's/CROSS_COMPILE=arm-ostl-linux-gnueabi-/CROSS_COMPILE=arm-none-linux-gnueabihf-/g'
		make -f ../Makefile.sdk all
}

cp_st_tf_a_images(){
	if [ -f  ${UF_A_SOURCE_PATH}/tf-a-stm32mp-2.2.r1-r0/build/trusted/tf-a-stm32mp157d-ev1-trusted.stm32 ];then
		echo "start to cp st_tf_a bin"
		cp ${UF_A_SOURCE_PATH}/tf-a-stm32mp-2.2.r1-r0/build/trusted/tf-a-stm32mp157d-ev1-trusted.stm32 ${APP_PATH}/../../images/
	fi
}
compile_tf_a_serialboot(){
	echo "atk serialboot bin compile"
	echo $(pwd)
	find ${UF_A_SOURCE_PATH}/../tf-a/alientek_tf-a/ -name Makefile.sdk | xargs sed -i 's/EXTRA_OEMAKE_SERIAL= STM32MP_UART_PROGRAMMER=1 STM32MP_USB_PROGRAMMER=1/EXTRA_OEMAKE_SERIAL=$(filter-out STM32MP_SDMMC=1 STM32MP_EMMC=1 STM32MP_SPI_NOR=1 STM32MP_RAW_NAND=1 STM32MP_SPI_NAND=1,$(EXTRA_OEMAKE)) STM32MP_UART_PROGRAMMER=1 STM32MP_USB_PROGRAMMER=1/g'
	cd ${UF_A_SOURCE_PATH}/../tf-a/alientek_tf-a/tf-a-stm32mp-2.2.r1
	make -f ../Makefile.sdk clean
	make -f ../Makefile.sdk TFA_DEVICETREE=stm32mp157d-atk TF_A_CONFIG=serialboot ELF_DEBUG_ENABLE='1' all
}

cp_atk_tf_a_serialboot_images(){
	if [ -f  ${UF_A_SOURCE_PATH}/../tf-a/alientek_tf-a/build/serialboot/tf-a-stm32mp157d-atk-serialboot.stm32 ];then
		echo "atk_tf_a_serialboot_images"
		cp ${UF_A_SOURCE_PATH}/../tf-a/alientek_tf-a/build/serialboot/tf-a-stm32mp157d-atk-serialboot.stm32 ${APP_PATH}/../../images/
	fi
}
create_linux_source
patch_uf_a_source
compile_tf_a_source
cp_tf_a_images
#compile_tf_a_serialboot
#cp_atk_tf_a_serialboot_images
#compile_st_tf_a_source
#cp_st_tf_a_images
exit 2
