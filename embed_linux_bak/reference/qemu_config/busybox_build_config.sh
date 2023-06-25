#!/bin/bash

# -------------------------------------------------------------------------------
# Filename:    busybox_build_next_v4.sh
# Revision:    1.1
# Date:        2020/01/11
# Author:      YobeZhou
# Email:       YobeZhou@qq.com
# Website:     www.100ask.net
# Function:    busybox_build_next_v4
# Notes:       None
# -------------------------------------------------------------------------------
#
# Description:
#	None
#
# -------------------------------------------------------------------------------

# define echo print color.
RED_COLOR='\E[1;31m'
PINK_COLOR='\E[1;35m'
YELOW_COLOR='\E[1;33m'
BLUE_COLOR='\E[1;34m'
GREEN_COLOR='\E[1;32m'
END_COLOR='\E[0m'


# 链接库文件所在的路劲
readonly LIB_PATH='/home/book/100ask_imx6ull-sdk/ToolChain/gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf/arm-linux-gnueabihf/libc/lib/'

# rootfs顶层目录
mkdir ./rootfs
cd ./rootfs
readonly TOP_DIR=`pwd`


#######################################
# 检查链接库目录中的文件
# Globals:
#   LIB_PATH
#   RED_COLOR
#   GREEN_COLOR
#   END_COLOR
# Arguments:
#   None
# Returns:
#   None/2
#######################################
check_lib_patch(){
	if [ -d  ${LIB_PATH} ];then
		echo -e  "${GREEN_COLOR}Lib directory is exist.${END_COLOR}"
		if [ -f  ${LIB_PATH}/ld-*.so ];then
			echo -e "${GREEN_COLOR}Link dynamic lib is exist.${END_COLOR}"
		else
			echo -e "${RED_COLOR}Link dynamic lib is not exist!${END_COLOR}"	
			exit 2		
		fi
	else
		echo -e "${RED_COLOR}Lib directory is not exist!${END_COLOR}"
		echo -e "${RED_COLOR}Link dynamic lib is not exist!${END_COLOR}"
		exit 2
	fi
}

#######################################
# 根据传入的参数创建相应的文件
# Globals:
#   TOP_DIR
# Arguments:
#   None
# Returns:
#   None
#######################################
populate_config_file(){
	# 创建文件 /etc/inittab 为 init 进程的配置文件
	cat > ${TOP_DIR}/etc/inittab << "EOF"
# /etc/inittab              
::sysinit:/etc/init.d/rcS    
console::askfirst:-/bin/sh   
#ttyO0::askfirst:-/bin/sh    
::ctrlaltdel:/sbin/reboot    
::shutdown:/bin/umount -a -r

EOF
	# 创建 /etc/init.d/rcS 文件并初始化
	cat > ${TOP_DIR}/etc/init.d/rcS << "EOF"
#!/bin/sh
#This file is /etc/init.d/rcS               			 
#ifconfig eth0 192.168.1.17       			 
											 
mount -a                          			 
mkdir /dev/pts                   			
mount -t devpts devpts /dev/pts          
echo /sbin/mdev > /proc/sys/kernel/hotplug
mdev -s  

#创建/lib/modules/`uname -r`/ 子目录,用于存放驱动模块文件                              
mkdir  -p  /lib/modules/`uname -r`/

if [ -x /usr/sbin/telnetd ] ;
then
		telnetd&
fi

EOF
		
	sudo   chmod  +x  ${TOP_DIR}/etc/init.d/rcS
	
	# 文件/etc/fstab指明需要挂载的文件系统
	# busybox会根据/etc/fstab中的设置项来自动创建/proc、/sys、/dev、/tmp、/var、/debugfs目录
	cat > ${TOP_DIR}/etc/fstab << "EOF"
# device   mount-point   type      options      dump  fsck  order
proc         /proc       proc      defaults      0     0         
sysfs        /sys        sysfs     defaults      0     0         
tmpfs        /dev        tmpfs     defaults      0     0         
tmpfs        /tmp        tmpfs     defaults      0     0         
#tmpfs       /var        tmpfs     defaults      0     0        
#debugfs     /debugfs    debugfs   defaults      0     0        
                                                                 
EOF
	sudo   chmod  +x  ${TOP_DIR}/etc/fstab
	
	# 创建 passwd 文件并初始化
	cat > ${TOP_DIR}/etc/passwd << "EOF"
root:x:0:0:root:/root:/bin/sh                                                                 
EOF
	
	# 创建 group 文件并初始化	
	cat > ${TOP_DIR}/etc/group << "EOF"
root:x:0:                                                                 
EOF
	sudo   chmod  777  ${TOP_DIR}/etc/group		

}

#######################################
# 将根文件系统压缩打包
# Globals:
#   TOP_DIR
#   BLUE_COLOR
#   GREEN_COLOR
#   END_COLOR
# Arguments:
#   None
# Returns:
#   None
#######################################
Compress_the_root_file_system(){
	cd ${TOP_DIR}
	echo -e  "${BLUE_COLOR}Compressing the root file system...${END_COLOR}"
	if  [ -f ./rootfs.tar.bz2 ]; then
			rm rootfs.tar.bz2
	fi

	tar -jcvf rootfs.tar.bz2  ./*
	echo -e  "${GREEN_COLOR}rootfs.tar.bz2 is ready.${END_COLOR}"
}

#######################################
# 创建目录和相关的文件
# Globals:
#   TOP_DIR
#   LIB_PATH
# Arguments:
#   None
# Returns:
#   None
#######################################
create_dir_and_files(){
	# 将编译出来的文件复制到工作目录
	sudo cp -rfd  ../../Busybox_1.30.0/_install/* ${TOP_DIR}

	# 先构建子目录(可以是空目录)
	mkdir    lib  etc  etc/init.d  dev  proc  sys  mnt  tmp  root 
	
	#复制所有的.so动态库文件,-d表示：如果原来是链接文件，仍保留链接关系
	sudo  cp  -d   ${LIB_PATH}/*.so*  ${TOP_DIR}/lib
	
	# 创建必要的文件
	populate_config_file
	
	# 配置 /dev 目录下的字符设备文件
	sudo   mknod  ${TOP_DIR}/dev/console  c  5  1
	sudo   mknod  ${TOP_DIR}/dev/null     c  1  3
}


check_lib_patch
create_dir_and_files
Compress_the_root_file_system
echo -e  "${GREEN_COLOR}All done.${END_COLOR}"
