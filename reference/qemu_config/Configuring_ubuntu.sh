#!/bin/bash
# -------------------------------------------------------------------------------
# Filename:    Configuring_Ubuntu.sh
# Revision:    1.1
# Date:        2020/01/10
# Author:      hceng
# Author:      li
# Email:       huangcheng.job@foxmail.com
# Website:     www.100ask.net
# Function:    Configuring the Ubuntu host environment.
# Notes:       Currently only supports Ubuntu16 and Ubuntu18.
# -------------------------------------------------------------------------------
#
# Description:
#1.Check env.
#  1.1 check network
#  1.2 check use root
#  1.3 check set name
#2.Install common software and configuration.
#  2.1 configure vim
#  2.2 configure tftp
#  2.3 configure nfs
#  2.4 configure samba
#3.Install system tool for Linux or Android
#
# -------------------------------------------------------------------------------

#define echo print color.
RED_COLOR='\E[1;31m'
PINK_COLOR='\E[1;35m'
YELOW_COLOR='\E[1;33m'
BLUE_COLOR='\E[1;34m'
GREEN_COLOR='\E[1;32m'
END_COLOR='\E[0m'
PLAIN='\033[0m'
#Set linux host user name.
user_name=book
# Check network.
check_network() {
    ping -c 1 www.baidu.com > /dev/null 2>&1
    if [ $? -eq 0 ];then
        echo -e "${BLUE_COLOR}Network OK.${END_COLOR}"
    else
        echo -e "${RED_COLOR}Network failure!${END_COLOR}"
        exit 1
    fi
}

# Check user must root.
check_root() {
    if [ $(id -u) != "0" ]; then
        echo -e "${RED_COLOR}Error: This script must be run as root!${END_COLOR}"
        exit 1
    fi
}
# Check set linux host user name.
check_user_name() {
    cat /etc/passwd|grep $user_name
    if [ $? -eq 0 ];then
        echo -e "${BLUE_COLOR}Check the set user name OK.${END_COLOR}"
        echo -e "123456\n123456" | sudo passwd root
    else
    	sudo   useradd -m $user_name   -G root -p 123456
		echo -e "123456\n123456" | sudo passwd $user_name
		sudo sh -c "echo \"$user_name ALL=(ALL:ALL) ALL\" >> /etc/sudoers"
        echo -e "${RED_COLOR}Add book user !${END_COLOR}"
    fi
}
# Check the results of the operation.
check_status() {
    ret=$?
    if [ "$ret" -ne "0" ]; then
        echo -e "${RED_COLOR}Failed setup, aborting..${END_COLOR}"
        exit 1
    fi
}
# Get the code name of the Linux host release to the caller.
get_host_type() {
    local  __host_type=$1
    local  the_host=`lsb_release -a 2>/dev/null | grep Codename: | awk {'print $2'}`
    eval $__host_type="'$the_host'"
}
# Select menu
menu() {
cat <<EOF
`echo -e "\E[1;33mPlease select the host use:\E[0m"`
`echo -e "\E[1;33m    1. Configuring for Harmony OS development \E[0m"`
`echo -e "\E[1;33m    2. Configuring for Linux development\E[0m"`
`echo -e "\E[1;33m    3. Configuring for Android development\E[0m"`
`echo -e "\E[1;33m    4. Quit\E[0m"`
EOF
}
#Set Ubuntu Source list address for aliyun 
SetUbuntuSourceList(){
        get_host_type host_release
        if [[ -f /etc/apt/sources.list.bak ]]; then
                echo -e " ${GREEN_COLOR}sources.list.bak exists${PINK_COLOR}"
        else
                mv /etc/apt/sources.list{,.bak}
        fi
        [ -f /etc/apt/sources.list ] && rm /etc/apt/sources.list
		
        echo "deb http://mirrors.aliyun.com/ubuntu/ bionic main restricted universe multiverse" >>/etc/apt/sources.list
        echo "deb http://mirrors.aliyun.com/ubuntu/ bionic-security main restricted universe multiverse" >>/etc/apt/sources.list
        echo "deb http://mirrors.aliyun.com/ubuntu/ bionic-updates main restricted universe multiverse" >>/etc/apt/sources.list
        echo "deb http://mirrors.aliyun.com/ubuntu/ bionic-backports main restricted universe multiverse" >>/etc/apt/sources.list
        echo "deb http://mirrors.aliyun.com/ubuntu/ bionic-proposed main restricted universe multiverse" >>/etc/apt/sources.list
        echo "deb-src http://mirrors.aliyun.com/ubuntu/ bionic main restricted universe multiverse" >>/etc/apt/sources.list
        echo "deb-src http://mirrors.aliyun.com/ubuntu/ bionic-security main restricted universe multiverse" >>/etc/apt/sources.list
        echo "deb-src http://mirrors.aliyun.com/ubuntu/ bionic-updates main restricted universe multiverse" >>/etc/apt/sources.list
        echo "deb-src http://mirrors.aliyun.com/ubuntu/ bionic-backports main restricted universe multiverse" >>/etc/apt/sources.list
        echo "deb-src http://mirrors.aliyun.com/ubuntu/ bionic-proposed main restricted universe multiverse" >>/etc/apt/sources.list

        [ "$host_release" == "$ubuntu16" ] && sed -i 's/bionic/xenial/'g /etc/apt/sources.list
		
        [ "$host_release" == "$ubuntu18" ] && echo -n ""
        sleep 1
        apt-get update
}
# Configure vim form github.
vim_configure() {
    git clone --depth=1 https://github.com/amix/vimrc.git /home/$user_name/.vim_runtime

    touch /home/$user_name/.vim_runtime/my_configs.vim
    echo "let g:go_version_warning = 0" > /home/$user_name/.vim_runtime/my_configs.vim

    chown -R $user_name /home/$user_name/.vim_runtime
    chmod u+x /home/$user_name/.vim_runtime/install_awesome_vimrc.sh
    su - $user_name -s /home/$user_name/.vim_runtime/install_awesome_vimrc.sh
}
# Configure tftp.
tftp_configure() {
    tftp_file=/home/$user_name/tftpboot

    if [ ! -d "$tftp_file" ];then
        mkdir -p -m 777 $tftp_file
    fi

    grep "/home/book/tftpboot" /etc/default/tftpd-hpa 1>/dev/null
    if [ $? -ne 0 ];then
        sed  -i '$a\TFTP_DIRECTORY="/home/book/tftpboot"' /etc/default/tftpd-hpa
        sed  -i '$a\TFTP_OPTIONS="-l -c -s"' /etc/default/tftpd-hpa
    fi

    service tftpd-hpa restart
}
# Configure nfs.
nfs_configure() {
    nfs_file=/home/$user_name/nfs_rootfs

    if [ ! -d "$nfs_file" ];then
        mkdir -p -m 777 $nfs_file
    fi

    grep "/home/book/" /etc/exports 1>/dev/null
    if [ $? -ne 0 ];then
        sed -i '$a\/home/book/  *(rw,nohide,insecure,no_subtree_check,async,no_root_squash)' /etc/exports  #todo
    fi

    service nfs-kernel-server restart
}
# Configure samba.
samba_configure() {
    local back_file=/etc/samba/smb.conf.bakup
    if [ ! -e "$back_file" ];then
        cp /etc/samba/smb.conf $back_file
    fi
    check_status

    grep "share_directory" /etc/samba/smb.conf 1>/dev/null
    if [ $? -ne 0 ];then
        sed -i \
        '$a[share_directory]\n\
        path = \/home\/book\n\
        available = yes\n\
        public = yes\n\
        guest ok = yes\n\
        read only = no\n\
        writeable = yes\n' /etc/samba/smb.conf
    fi

    /etc/init.d/samba restart
    #chmod -R 777 /home/book/
}
# Execute an action.
FA_DoExec() {
    echo -e "${BLUE_COLOR}==> Executing: '${@}'.${END_COLOR}"
    eval $@ || exit $?
}



# Install openharmony for Linux
install_openharmony_software() {
    local ubuntu16=("xenial")
    local ubuntu18=("bionic")

    get_host_type host_release

    if [ "$host_release" = "$ubuntu16" ]
    then
       echo "This Ubuntu version is not supported"
    else
        if [ "$host_release" = "$ubuntu18" ]
        then
			wait
			sudo apt-get install python3.8  dosfstools mtools  python3-setuptools python3-pip -y
			wait
			sudo rm -f /usr/bin/python
			sudo ln -s /usr/bin/python3.8 /usr/bin/python
			wget https://repo.huaweicloud.com/harmonyos/compiler/gn/1523/linux/gn.1523.tar
			wait
			tar -xvf gn.1523.tar -C ~/
			wget https://repo.huaweicloud.com/harmonyos/compiler/ninja/1.9.0/linux/ninja.1.9.0.tar
			wait
			tar -xvf ninja.1.9.0.tar -C ~/
			wget https://repo.huaweicloud.com/harmonyos/compiler/clang/9.0.0-34042/linux/llvm-linux-9.0.0-34042.tar
			wait
			tar -xvf llvm-linux-9.0.0-34042.tar -C ~/
			wait
			wget https://repo.huaweicloud.com/harmonyos/compiler/hc-gen/0.65/linux/hc-gen-0.65-linux.tar
			wait
			tar -xvf hc-gen-0.65-linux.tar -C ~/			
			echo "export PATH=~/ninja:~/llvm/bin:~/hc-gen:~/gn:${PATH}" >> ~/.bashrc
			source ~/.bashrc
			sudo pip3 install kconfiglib
			echo "OK "
			sudo rm -rf /bin/sh
			sudo ln -s /bin/bash /bin/sh
			ls -la /bin/sh
        else
            echo "This Ubuntu version is not supported"
            exit 0
        fi
    fi
}

# Install common software and configuration
install_common_software() {
    apt-get update
    check_status

    local install_software_list=("ssh" "git" "vim" "tftp" "nfs" "samba")
    echo -e "${BLUE_COLOR}install_software_list: ${install_software_list[*]}.${END_COLOR}"

    #install ssh
    if (echo "${install_software_list[@]}" | grep -wq "ssh");then
        apt-get -y install openssh-server && echo -e "${BLUE_COLOR}ssh install completed.${END_COLOR}"
    fi

    #install git
    if (echo "${install_software_list[@]}" | grep -wq "git");then
        apt-get -y install git && echo -e "${BLUE_COLOR}git install completed.${END_COLOR}"
    fi

    #install and configure vim
    #if (echo "${install_software_list[@]}" | grep -wq "vim");then
        #apt-get -y install vim && vim_configure && echo -e "${BLUE_COLOR}vim install completed.${END_COLOR}"
    #fi

    #install and configure tftp
    if (echo "${install_software_list[@]}" | grep -wq "tftp");then
        apt-get -y install tftp-hpa tftpd-hpa && tftp_configure && echo -e "${BLUE_COLOR}tftp install completed.${END_COLOR}"
    fi

    #install and configure nfs
    if (echo "${install_software_list[@]}" | grep -wq "nfs");then
        apt-get -y install nfs-kernel-server && nfs_configure && echo -e "${BLUE_COLOR}nfs install completed.${END_COLOR}"
    fi

    #install and configure samba
    if (echo "${install_software_list[@]}" | grep -wq "samba");then
        apt-get -y install samba && samba_configure && echo -e "${BLUE_COLOR}samba install completed.${END_COLOR}"
    fi
}

install_user_software() {
	wget  https://weidongshan.coding.net/p/DevelopmentEnvConf/d/DevelopmentEnvConf/git/raw/master/mkimage.stm32
	chmod +x  mkimage.stm32
	sudo mv  mkimage.stm32 /usr/bin/
}
# Install software for Linux
install_linux_software() {
    local ubuntu16=("xenial")
    local ubuntu18=("bionic")

    get_host_type host_release

    if [ "$host_release" = "$ubuntu16" ]
    then
        FA_DoExec apt-get install mtd-utils curl gcc make git vim python net-tools openssh-server \
        python-dev build-essential subversion \
        libncurses5-dev zlib1g-dev gawk gcc-multilib flex git-core gettext  \
        gfortran libssl-dev libpcre3-dev xlibmesa-glu-dev libglew1.5-dev \
        libftgl-dev libmysqlclient-dev libfftw3-dev libcfitsio-dev graphviz-dev \
        libavahi-compat-libdnssd-dev libldap2-dev  libxml2-dev p7zip-full \
        libkrb5-dev libgsl0-dev  u-boot-tools lzop bzr device-tree-compiler android-tools-mkbootimg -y
    else
        if [ "$host_release" = "$ubuntu18" ]
        then
            echo '* libraries/restart-without-asking boolean true' | sudo debconf-set-selections
            FA_DoExec apt-get install curl mtd-utils  gcc make git vim python net-tools openssh-server \
            python-dev build-essential subversion \
            libncurses5-dev zlib1g-dev gawk gcc-multilib flex git-core gettext  \
            gfortran libssl-dev libpcre3-dev xlibmesa-glu-dev libglew1.5-dev \
            libftgl-dev libmysqlclient-dev libfftw3-dev libcfitsio-dev graphviz-dev \
            libavahi-compat-libdnssd-dev libldap2-dev  libxml2-dev p7zip-full bzr  \
            libkrb5-dev libgsl0-dev  u-boot-tools lzop -y
        else
            echo "This Ubuntu version is not supported"
            exit 0
        fi
    fi
}

# Install software for Android
install_android_software() {
    local ubuntu16=("xenial")
    local ubuntu18=("bionic")

    get_host_type host_release

    if [ "$host_release" = "$ubuntu16" ]
    then
        FA_DoExec add-apt-repository ppa:openjdk-r/ppa
        FA_DoExec apt-get update
        FA_DoExec apt-get install openjdk-7-jdk
        FA_DoExec update-java-alternatives -s java-1.7.0-openjdk-amd64
        FA_DoExec java -version

        FA_DoExec apt-get install -y git flex bison gperf build-essential libncurses5-dev:i386 libc6-dev-i386 \
        libx11-dev:i386 libreadline6-dev:i386 libgl1-mesa-dev g++-multilib \
        tofrodos python-markdown libxml2-utils xsltproc zlib1g-dev:i386 \
        dpkg-dev libsdl1.2-dev libesd0-dev \
        git-core gnupg flex bison gperf build-essential  \
        zip curl zlib1g-dev gcc-multilib g++-multilib \
        lib32ncurses5-dev x11proto-core-dev libx11-dev \
        lib32z-dev ccache  squashfs-tools libncurses5-dev  pngcrush schedtool libxml2\
        libgl1-mesa-dev  unzip m4 lzop libc6-dev  lib32z1-dev \
        libswitch-perl libssl1.0.0 libssl-dev

    else
        if [ "$host_release" = "$ubuntu18" ]
        then
            FA_DoExec apt-get install openjdk-8-jdk openjdk-8-jre
            FA_DoExec java -version
            FA_DoExec apt-get install m4  g++-multilib gcc-multilib \
            lib32ncurses5-dev  lib32readline6-dev lib32z1-dev flex curl bison

            FA_DoExec apt-get install libx11-dev:i386 libreadline6-dev:i386 libgl1-mesa-dev g++-multilib \
            git flex bison gperf build-essential libncurses5-dev:i386 \
            dpkg-dev libsdl1.2-dev libesd0-dev \
            git-core gnupg flex bison gperf build-essential \
            zip curl zlib1g-dev gcc-multilib g++-multilib \
            libc6-dev-i386  lib32ncurses5-dev x11proto-core-dev libx11-dev \
            libgl1-mesa-dev libxml2-utils xsltproc unzip m4 \
            lib32z1-dev ccache make  tofrodos \
            python-markdown libxml2-utils xsltproc zlib1g-dev:i386 lzop -y
        else
            echo "This Ubuntu version is not supported"
            exit 0
        fi
    fi
}
check_network
check_root
check_user_name
menu
while true
do
    read -p "please input your choice:" ch
    case $ch in
        1)
		SetUbuntuSourceList
		install_common_software
		install_linux_software
		install_openharmony_software
		install_user_software
            echo -e "${GREEN_COLOR}===================================================${END_COLOR}"
            echo -e "${GREEN_COLOR}==  Configuring for openharmony development complete!  ==${END_COLOR}"
            echo -e "${GREEN_COLOR}===================================================${END_COLOR}"
            echo -e "${BLUE_COLOR}TFTP  PATH: $tftp_file ${END_COLOR}"
            echo -e "${BLUE_COLOR}NFS   PATH: $nfs_file ${END_COLOR}"
            echo -e "${BLUE_COLOR}SAMBA PATH: /home/book/ ${END_COLOR}"
            su $user_name
            ;;
        2)
		SetUbuntuSourceList
            install_common_software
            install_linux_software
	    install_user_software
            echo -e "${GREEN_COLOR}===================================================${END_COLOR}"
            echo -e "${GREEN_COLOR}==  Configuring for Linux development complete!  ==${END_COLOR}"
            echo -e "${GREEN_COLOR}===================================================${END_COLOR}"
            echo -e "${BLUE_COLOR}TFTP  PATH: $tftp_file ${END_COLOR}"
            echo -e "${BLUE_COLOR}NFS   PATH: $nfs_file ${END_COLOR}"
            echo -e "${BLUE_COLOR}SAMBA PATH: /home/book/ ${END_COLOR}"
            su $user_name
            ;;
        3)
			SetUbuntuSourceList
            install_common_software
            install_linux_software
            install_android_software
	    install_user_software
            echo -e "${GREEN_COLOR}===================================================${END_COLOR}"
            echo -e "${GREEN_COLOR}== Configuring for Android development complete! ==${END_COLOR}"
            echo -e "${GREEN_COLOR}===================================================${END_COLOR}"
            echo -e "${BLUE_COLOR}TFTP  PATH: $tftp_file ${END_COLOR}"
            echo -e "${BLUE_COLOR}NFS   PATH: $nfs_file ${END_COLOR}"
            echo -e "${BLUE_COLOR}SAMBA PATH: /home/book/ ${END_COLOR}"
            su $user_name
            ;;
        4)
            break
            exit 0
            ;;
        *)
            clear
            echo "Sorry, wrong selection"
            exit 0
            ;;
    esac
done

exit 0
