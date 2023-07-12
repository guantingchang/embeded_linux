#!/bin/sh

set -e
cd `dirname $0`

sudo apt-get update 

sudo apt-get install lsb-core lib32stdc++6

echo $(pwd)
if [ ! -d  ./compile_tool ];then
	sudo mkdir compile_tool
	sudo tar -vxf gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf.tar.xz  -C ./compile_tool
	sudo echo export PATH=$PATH:$(pwd)/compile_tool/gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf/bin >> /etc/profile
	sudo echo export ARCH=arm >> /etc/profile
	sudo echo export CROSS_COMPILE=arm-none-linux-gnueabihf >> /etc/profile
	source /etc/profile
fi
arm-none-linux-gnueabihf-gcc -v 

# 1.stm32wrapper4dbg 工具安装
sudo apt-get install device-tree-compiler
sudo apt-get install libncurses5-dev bison flex
#安装nfs
#安装tftpy
sudo apt-get install tftp-hpa tftpd-hpa
sudo apt-get install xinetd
#menuconfig图形化配置界面支持
sudo apt-get install build-essential
sudo apt-get install libncurses5-dev
#linux 内核编译工具
sudo apt-get install lzop
sudo apt-get install libssl-dev
sudo apt-get install u-boot-tools
#linux 开发工具
sudo apt-get install iperf3