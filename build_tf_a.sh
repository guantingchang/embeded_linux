#! /bin/sh

APP_PATH=$(pwd)
echo APP_PATH=$APP_PATH

tar -xvf en.SOURCES-stm32mp1-openstlinux-5-4-dunfell-mp1-20-06-24.tar.xz
ls -al

mkdir -p ${APP_PATH}/sources/tf-a
cp -r stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/tf-a-stm32mp-2.2.r1-r0 ${APP_PATH}/sources/tf-a
cp -r stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/optee-os-stm32mp-3.9.0.r1-r0 ${APP_PATH}/sources/tf-a
cp -r stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/tf-a-stm32mp-ssp-2.2.r1-r0 ${APP_PATH}/sources/tf-a

cd ${APP_PATH}/sources/tf-a/tf-a-stm32mp-2.2.r1-r0
ls -al
tar -vxf tf-a-stm32mp-2.2.r1-r0.tar.gz
cd tf-a-stm32mp-2.2.r1
for p in `ls -1 ../*.patch`; do patch -p1 < $p; done
echo "stm32 tf-a patch success!"


echo "start to compile tf-a"
sudo apt-get install device-tree-compiler
mkdir -p ${APP_PATH}/sources/alientek_tf-a
echo $(pwd)
cd ${APP_PATH}

tar -xvf tf-a-stm32mp-2.2.r1-gaa9f87c-v1.5.tar.bz2
echo "alientek tf-a create"

mkdir -p ${APP_PATH}/sources/alientek_tf-a/
cp -a ${APP_PATH}/tf-a-stm32mp-2.2.r1  ${APP_PATH}/sources/alientek_tf-a
cp -a ${APP_PATH}/Makefile.sdk   ${APP_PATH}/sources/alientek_tf-a
rm -rf ${APP_PATH}/tf-a-stm32mp-2.2.r1
rm -rf ${APP_PATH}/Makefile.sdk

cd ${APP_PATH}/sources/alientek_tf-a/tf-a-stm32mp-2.2.r1
find ${APP_PATH}/sources/alientek_tf-a -name Makefile.sdk | xargs sed -i 's/CROSS_COMPILE=arm-ostl-linux-gnueabi-/CROSS_COMPILE=arm-none-linux-gnueabihf-/g'
make -f ../Makefile.sdk all
exit 0
