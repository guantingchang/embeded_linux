cd tar/atk_uboot
make clean
#make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- stm32mp157d_atk_defconfig
make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- stm32mp15_atk_trusted_defconfig
make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- menuconfig

make V=1 ARCH=arm COROSS_COMPILE=arm-none-linux-gnueabihf- DEVICE_TREE=stm32mp157d-atk all
cp u-boot.bin ../../images/
cp u-boot.stm32 ../../images/