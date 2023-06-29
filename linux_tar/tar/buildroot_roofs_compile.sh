
#! /bin/sh

set -e
cd `dirname $0`

APP_PATH=$(pwd)
echo APP_PATH=$APP_PATH
BUILDROOT_SOURCE_PATH=${APP_PATH}/../../sources/rootfs/buildroot
BUILDROOT_BUILD_PATH=${APP_PATH}/../../images/rootfs/buildroot

# 链接库文件所在的路劲
readonly LIB_PATH=$(pwd)/../tools/compile_tool/gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf/arm-none-linux-gnueabihf/libc/lib/
echo LIB_PATH=$LIB_PATH
echo BUILDROOT_BUILD_PATH = $BUILDROOT_BUILD_PATH


compile_linux_bildroot_source(){	
	if [ ! -d  ${BUILDROOT_SOURCE_PATH} ];then
		echo "start to create buildroot"
		mkdir -p ${BUILDROOT_SOURCE_PATH}
		cd ${APP_PATH}
		tar -vxjf buildroot-2020.02.6.tar.bz2 -C ${BUILDROOT_SOURCE_PATH}		
	fi
	echo "alientek linux buildroot compile" 
	cd ${BUILDROOT_SOURCE_PATH}/buildroot-2020.02.6/
	echo $(pwd)
	#make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- distclean
	if [ ! -f  stm32mp1_atk_defconfig ];then
		echo "menuconfig"
		make menuconfig
		sudo make make FORCE_UNSAFE_CONFIGURE=1
	else
		echo "atk_defconfig"
		#make stm32mp1_atk_defconfig
		make menuconfig
		sudo make FORCE_UNSAFE_CONFIGURE=1
	fi 
}

cp_linux_buildroot_images(){
	if [ -d  ${BUILDROOT_BUILD_PATH} ];then	
		cd ${BUILDROOT_BUILD_PATH}/../
		mv buildroot buildroot_bak2
	fi
	
	mkdir -p ${BUILDROOT_BUILD_PATH}
	echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
	echo ${BUILDROOT_BUILD_PATH}
	tar -vxf ${BUILDROOT_SOURCE_PATH}/buildroot-2020.02.6/output/images/rootfs.tar -C  ${BUILDROOT_BUILD_PATH}

}
#######################################
# 根据传入的参数创建相应的文件
# Globals:
#   BUSYBOX_BUILD_PATH
# Arguments:
#   None
# Returns:
#   None
#######################################
populate_config_file(){
	# 创建 /etc/init.d/rcS 文件并初始化
	sudo cat > ${BUILDROOT_BUILD_PATH}/etc/init.d/Sautorun << "EOF"
#!/bin/sh
#This file is /etc/init.d/Sautorun               			 
mount -t debugfs none /sys/kernel/debug
ifconfig eth0 up
ifconfig eth0 192.168.3.45 netmask 255.255.255.0
EOF
		
	sudo   chmod  +x  ${BUILDROOT_BUILD_PATH}/etc/init.d/Sautorun
	# 创建PS1环境变量
	sudo mkdir -p ${BUILDROOT_BUILD_PATH}/etc/profile.d
	sudo chmod  777 -R ${BUILDROOT_BUILD_PATH}/etc/profile.d
	sudo cat > ${BUILDROOT_BUILD_PATH}/etc/profile.d/myprofile.sh << "EOF"
#!/bin/sh
#This file is /etc/profile.d
PS1='[\u@\h]:\w$ '
export PS1
EOF
	sudo   chmod  +x  ${BUILDROOT_BUILD_PATH}/etc/profile.d/myprofile.sh
}

#######################################
# 创建目录和相关的文件
# Globals:
#   BUSYBOX_BUILD_PATH
#   LIB_PATH
# Arguments:
#   None
# Returns:
#   None
#######################################
buildroot_create_dir_and_files(){
	# 将编译出来的文件复制到工作目录
	#sudo cp -rfd  ../../Busybox_1.30.0/_install/* ${BUSYBOX_BUILD_PATH}
	cd ${BUILDROOT_BUILD_PATH}
	echo "rootfs dir>>>$(pwd)"

	# 创建必要的文件
	populate_config_file
}

create_rootfs_ext4(){
	cd ${BUSYBOX_BUILD_PATH}
	if [! -f  rootfs.ext4 ];then
	dd if=/dev/zero of=rootfs.ext4 bs=1M count=1024
	mkfs.ext4 -L rootfs rootfs.ext4
	fi
}
compile_linux_bildroot_source
cp_linux_buildroot_images
buildroot_create_dir_and_files
exit 0
#nfs c2000000 192.168.3.29:/home/share/samba/embed_linux/embeded_linux/images/uImage