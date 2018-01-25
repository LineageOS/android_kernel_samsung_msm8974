#!/bin/bash

#
# SmartPack-Kernel Build Script
# Edited by KazuDante for OreKazu-Kernel
# Author: sunilpaulmathew <sunil.kde@gmail.com>
#

#
# This script is licensed under the terms of the GNU General Public
# License version 2, as published by the Free Software Foundation,
# and may be copied, distributed, and modified under those terms.
#

#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#

#
# ***** ***** ***** ..How to use this script… ***** ***** ***** #
#
# For those who want to build this kernel using this script…
#
# Please note: this script is by-default designed to build only
# one variants at a time.
#

# 1. Properly locate Stock, UBER & Linaro toolchains (Line# 43, 45 & 47)
# 2. Select the preferred toolchain for building (Line# 49)
# 3. Select the 'KERNEL_VARIANT' (Line# 55)
# 4. Open Terminal, ‘cd’ to the Kernel ‘root’ folder and run ‘. build_variant-OreKazu.sh’
# 5. The output (anykernel zip) file will be generated in the ‘release_OreKazu’ folder
# 6. Enjoy your new Kernel

#
# ***** ***** *Variables to be configured manually* ***** ***** #

# Toolchains

GOOGLE='/home/kazudante/Googletoolchain/arm-linux-androideabi-4.9/bin/arm-linux-androideabi-'

UBERTC="/home/sunil/UBERTC-arm-eabi-8.0/bin/arm-linux-androideabi-"

LINARO='/home/kazudante/arm-linaro-linux-androideabi/bin/arm-linaro-linux-androideabi-' 


TOOLCHAIN="GOOGLE"	# Leave empty for using Google’s stock toolchain

ARCHITECTURE="arm"

KERNEL_NAME="OreKazu-Kernel"

KERNEL_VARIANT="klte"	# only one variant at a time

KERNEL_VERSION="Oreo-beta-v1.2"   # leave as such, if no specific version tag

KERNEL_DEFCONFIG="OreKazu_@$KERNEL_VARIANT@_defconfig"

KERNEL_DATE="$(date +"%Y%m%d")"

COMPILE_DTB="y"

NUM_CPUS=""   # number of cpu cores used for build (leave empty for auto detection)

# ***** ***** ***** ***** ***THE END*** ***** ***** ***** ***** #

COLOR_RED="\033[0;31m"
COLOR_GREEN="\033[1;32m"
COLOR_NEUTRAL="\033[0m"

export ARCH=$ARCHITECTURE

if [ -z "$TOOLCHAIN" ]; then
	echo -e $COLOR_GREEN"\n building $KERNEL_NAME v. $KERNEL_VERSION for $KERNEL_VARIANT using Google's stock toolchain\n"$COLOR_NEUTRAL
	export CROSS_COMPILE="${CCACHE} $GOOGLE"
else
	if [ "ubertc" == "$TOOLCHAIN" ]; then
	echo -e $COLOR_GREEN"\n building $KERNEL_NAME v. $KERNEL_VERSION for $KERNEL_VARIANT using UBERTC-8.x\n"$COLOR_NEUTRAL
		export CROSS_COMPILE="${CCACHE} $UBERTC"
	else
		if [ "linaro" == "$TOOLCHAIN" ]; then
		echo -e $COLOR_GREEN"\n building $KERNEL_NAME v. $KERNEL_VERSION for $KERNEL_VARIANT using Linaro-7.x toolchain\n"$COLOR_NEUTRAL
			export CROSS_COMPILE="${CCACHE} $LINARO"
		fi
	fi
fi

if [ -z "$NUM_CPUS" ]; then
	NUM_CPUS=`grep -c ^processor /proc/cpuinfo`
fi

if [ -z "$KERNEL_VARIANT" ]; then
	echo -e $COLOR_GREEN"\n Please select the variant to build... 'KERNEL_VARIANT' should not be empty...\n"$COLOR_NEUTRAL
else
	if [ -e arch/arm/configs/$KERNEL_DEFCONFIG ]; then
		# creating backups
		cp scripts/mkcompile_h release_OreKazu/
		# updating kernel name
		sed -i "s;OreKazu-Kernel;$KERNEL_NAME-$KERNEL_VARIANT;" scripts/mkcompile_h;
		if [ -e output_$KERNEL_VARIANT/ ]; then
			if [ -e output_$KERNEL_VARIANT/.config ]; then
				rm -f output_$KERNEL_VARIANT/.config
				if [ -e output_$KERNEL_VARIANT/arch/arm/boot/zImage ]; then
					rm -f output_$KERNEL_VARIANT/arch/arm/boot/zImage
				fi
			fi
		else
			mkdir output_$KERNEL_VARIANT
		fi
		make -C $(pwd) O=output_$KERNEL_VARIANT $KERNEL_DEFCONFIG
		# updating kernel version
		sed -i "s;lineageos;$KERNEL_VERSION;" output_$KERNEL_VARIANT/.config;
		make -j$NUM_CPUS -C $(pwd) O=output_$KERNEL_VARIANT
		if [ -e output_$KERNEL_VARIANT/arch/arm/boot/zImage ]; then
			echo -e $COLOR_GREEN"\n copying zImage to anykernel directory\n"$COLOR_NEUTRAL
			cp output_$KERNEL_VARIANT/arch/arm/boot/zImage anykernel_OreKazu/
			# compile dtb if required
			if [ "y" == "$COMPILE_DTB" ]; then
				echo -e $COLOR_GREEN"\n compiling device tree blob (dtb)\n"$COLOR_NEUTRAL
				if [ -f output_$KERNEL_VARIANT/arch/arm/boot/dt.img ]; then
					rm -f output_$KERNEL_VARIANT/arch/arm/boot/dt.img
				fi
				chmod 777 tools/dtbToolCM
				tools/dtbToolCM -2 -o output_$KERNEL_VARIANT/arch/arm/boot/dt.img -s 2048 -p output_$KERNEL_VARIANT/scripts/dtc/ output_$KERNEL_VARIANT/arch/arm/boot/
				# removing old dtb (if any)
				if [ -f anykernel_OreKazu/dtb ]; then
					rm -f anykernel_OreKazu/dtb
				fi
				# copying generated dtb to anykernel directory
				if [ -e output_$KERNEL_VARIANT/arch/arm/boot/dt.img ]; then
					mv -f output_$KERNEL_VARIANT/arch/arm/boot/dt.img anykernel_OreKazu/dtb
				fi
			fi
			echo -e $COLOR_GREEN"\n generating recovery flashable zip file\n"$COLOR_NEUTRAL
			cd anykernel_OreKazu/ && zip -r9 $KERNEL_NAME-$KERNEL_VARIANT-$KERNEL_VERSION-$KERNEL_DATE.zip * -x README.md $KERNEL_NAME-$KERNEL_VARIANT-$KERNEL_VERSION-$KERNEL_DATE.zip && cd ..
			echo -e $COLOR_GREEN"\n cleaning...\n"$COLOR_NEUTRAL
			rm anykernel_OreKazu/zImage && mv anykernel_OreKazu/$KERNEL_NAME* release_OreKazu/
			if [ -f anykernel_OreKazu/dtb ]; then
				rm -f anykernel_OreKazu/dtb
			fi
			# restoring backups
			mv release_OreKazu/mkcompile_h scripts/
			echo -e $COLOR_GREEN"\n everything done... please visit "release_OreKazu"...\n"$COLOR_NEUTRAL
		else
			if [ -f anykernel_OreKazu/dtb ]; then
				rm -f anykernel_OreKazu/dtb
			fi
			# restoring backups
			mv release_OreKazu/mkcompile_h scripts/
			echo -e $COLOR_GREEN"\n Building error... zImage not found...\n"$COLOR_NEUTRAL
		fi
	else
		echo -e $COLOR_GREEN"\n '$KERNEL_VARIANT' is not a supported variant... please check...\n"$COLOR_NEUTRAL
	fi
fi
