#!/bin/bash

export ARCH=arm

export CROSS_COMPILE='/home/kazudante/arm-linaro-linux-androideabi/bin/arm-linaro-linux-androideabi-'

mkdir output

make -C $(pwd) O=output OreKazu_@klte@_defconfig

make -j2 -C $(pwd) O=output

chmod 777 tools/dtbToolCM

tools/dtbToolCM -2 -o output/arch/arm/boot/dt.img -s 2048 -p output/scripts/dtc/ output/arch/arm/boot/
