
#! /bin/sh

APP_PATH=$(pwd)
echo APP_PATH=$APP_PATH
BUSYBOX_SOURCE_PATH=${APP_PATH}/../../sources/rootfs/busybox
BUSYBOX_BUILD_PATH=${APP_PATH}/../../images/rootfs

# 链接库文件所在的路劲
readonly LIB_PATH='/home/share/samba/mp157_linux/alientek/tools/gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf/arm-none-linux-gnueabihf/libc/lib/'
echo BUSYBOX_SOURCE_PATH = $BUSYBOX_SOURCE_PATH


compile_linux_busybox_source(){	
	if [ ! -d  ${BUSYBOX_SOURCE_PATH} ];then
		echo "start to create busybox"
		mkdir -p ${BUSYBOX_SOURCE_PATH}
		cd ${APP_PATH}
		tar -vxjf busybox-1.32.0.tar.bz2 -C ${BUSYBOX_SOURCE_PATH}		
	fi
	echo "alientek linux busybox compile" 
	cd ${BUSYBOX_SOURCE_PATH}/busybox-1.32.0/
	echo $(pwd)
	#make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- distclean
	make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- stm32mp1_atk_defconfig
	make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- menuconfig
	make V=1 ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- -j8
	make install CONFIG_PREFIX=${BUSYBOX_BUILD_PATH}
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
	# 创建文件 /etc/inittab 为 init 进程的配置文件
	sudo cat > ${BUSYBOX_BUILD_PATH}/etc/inittab << "EOF"
# /etc/inittab
#<id>:<runlevels>:<action>:<process>              
::sysinit:/etc/init.d/rcS
::restart:/sbin/init 
console::askfirst:-/bin/sh 
#ttyO0::askfirst:-/bin/sh    
::ctrlaltdel:/sbin/reboot    
::shutdown:/bin/umount -a -r
::shutdown:/sbin/swapoff -a
EOF
	# 创建 /etc/init.d/rcS 文件并初始化
	sudo cat > ${BUSYBOX_BUILD_PATH}/etc/init.d/rcS << "EOF"
#!/bin/sh
#This file is /etc/init.d/rcS               			 
#ifconfig eth0 192.168.1.17       			 
PATH=/sbin:/bin:/usr/sbin:/usr/bin:$PATH
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/lib:/usr/lib
export PATH LD_LIBRARY_PATH			

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
		
	sudo   chmod  +x  ${BUSYBOX_BUILD_PATH}/etc/init.d/rcS
	
	# 文件/etc/fstab指明需要挂载的文件系统
	# busybox会根据/etc/fstab中的设置项来自动创建/proc、/sys、/dev、/tmp、/var、/debugfs目录
	sudo cat > ${BUSYBOX_BUILD_PATH}/etc/fstab << "EOF"
# device   mount-point   type      options      dump  fsck  order
proc         /proc       proc      defaults      0     0         
sysfs        /sys        sysfs     defaults      0     0         
tmpfs        /dev        tmpfs     defaults      0     0         
tmpfs        /tmp        tmpfs     defaults      0     0         
#tmpfs       /var        tmpfs     defaults      0     0        
#debugfs     /debugfs    debugfs   defaults      0     0        
                                                                 
EOF
	sudo   chmod  +x  ${BUSYBOX_BUILD_PATH}/etc/fstab
	
	# 创建 passwd 文件并初始化
	sudo cat > ${BUSYBOX_BUILD_PATH}/etc/passwd << "EOF"
root:x:0:0:root:/root:/bin/sh                                                                 
EOF
	
	# 创建 group 文件并初始化	
	sudo cat > ${BUSYBOX_BUILD_PATH}/etc/group << "EOF"
root:x:0:                                                                 
EOF
	sudo   chmod  777  ${BUSYBOX_BUILD_PATH}/etc/group		
	# 创建dns解析文件
	sudo cat > ${BUSYBOX_BUILD_PATH}/etc/resolv.conf << "EOF"
nameserver 114.114.114.114
nameserver 192.168.3.1
EOF
	sudo chmod 777 ${BUSYBOX_BUILD_PATH}/etc/resolv.conf
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
rootfs_create_dir_and_files(){
	# 将编译出来的文件复制到工作目录
	#sudo cp -rfd  ../../Busybox_1.30.0/_install/* ${BUSYBOX_BUILD_PATH}
	cd ${BUSYBOX_BUILD_PATH}
	echo "rootfs dir>>>$(pwd)"
	# 先构建子目录(可以是空目录)
	sudo mkdir    lib  etc  etc/init.d  dev  proc  sys  mnt  tmp  root driver usr/lib
	
	#复制所有的.so动态库文件,-d表示：如果原来是链接文件，仍保留链接关系
	sudo  cp  -d   ${LIB_PATH}/*.so*  ${BUSYBOX_BUILD_PATH}/lib
	sudo  cp  -d   ${LIB_PATH}/../../lib/*.so* ${BUSYBOX_BUILD_PATH}/lib
	sudo  cp  -d   ${LIB_PATH}/../../lib/*.a ${BUSYBOX_BUILD_PATH}/lib
	sudo  cp  -d   ${LIB_PATH}/../usr/lib/*.so* ${BUSYBOX_BUILD_PATH}/usr/lib
	# 创建必要的文件
	populate_config_file
	
	# 配置 /dev 目录下的字符设备文件
	sudo   mknod  ${BUSYBOX_BUILD_PATH}/dev/console  c  5  1
	sudo   mknod  ${BUSYBOX_BUILD_PATH}/dev/null     c  1  3
}

create_rootfs_ext4(){
	cd ${BUSYBOX_BUILD_PATH}
	if [! -f  rootfs.ext4 ];then
	dd if=/dev/zero of=rootfs.ext4 bs=1M count=1024
	mkfs.ext4 -L rootfs rootfs.ext4
	fi
}
compile_linux_busybox_source
rootfs_create_dir_and_files
exit 0
#nfs c2000000 192.168.3.29:/home/share/samba/embed_linux/embeded_linux/images/uImage