cd u-boot-stm32mp-2020.01/
for p in `ls -1 ../*.patch`;do patch -p1 < $p;done

