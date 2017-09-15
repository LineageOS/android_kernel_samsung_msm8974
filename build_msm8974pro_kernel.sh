#!/bin/bash
# MSM8974PRO KK kernel build script v0.5

BUILD_TOP_DIR=..
BUILD_KERNEL_DIR=$(pwd)

SECURE_SCRIPT=$BUILD_TOP_DIR/../buildscript/tools/signclient.jar
BUILD_CROSS_COMPILE=../../prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin/arm-eabi-
BUILD_JOB_NUMBER=`grep processor /proc/cpuinfo|wc -l`

# Default Python version is 2.7
mkdir -p bin
ln -sf /usr/bin/python2.7 ./bin/python
export PATH=$(pwd)/bin:$PATH

KERNEL_DEFCONFIG=msm8974_sec_defconfig
DEBUG_DEFCONFIG=msm8974_sec_eng_defconfig
SELINUX_DEFCONFIG=selinux_defconfig
SELINUX_LOG_DEFCONFIG=selinux_log_defconfig

#sed -i.bak "s/CONFIG_MODVERSIONS=y/CONFIG_MODVERSIONS=n/g" ${BUILD_KERNEL_DIR}/arch/arm/configs/${KERNEL_DEFCONFIG}

while getopts "w:t:" flag; do
	case $flag in
		w)
			BUILD_OPTION_HW_REVISION=$OPTARG
			echo "-w : "$BUILD_OPTION_HW_REVISION""
			;;
		t)
			TARGET_BUILD_VARIANT=$OPTARG
			echo "-t : "$TARGET_BUILD_VARIANT""
			;;
		*)
			echo "wrong 2nd param : "$OPTARG""
			exit -1
			;;
	esac
done

shift $((OPTIND-1))

BUILD_COMMAND=$1

SEANDROID_OPTION=
SECURE_OPTION=
if [ "$2" == "-B" ]; then
	SECURE_OPTION=$2
elif [ "$2" == "-E" ]; then
	SEANDROID_OPTION=$2
else
	NO_JOB=
fi

if [ "$3" == "-B" ]; then
	SECURE_OPTION=$3
elif [ "$3" == "-E" ]; then
	SEANDROID_OPTION=$3
else
	NO_JOB=
fi

if [ "$BUILD_COMMAND" == "klte_eur" ]; then
	SIGN_MODEL=SM-G900F_EUR_XX_ROOT0
elif [ "$BUILD_COMMAND" == "klte_att" ]; then
	SIGN_MODEL=SM-G900A_NA_ATT_ROOT0
elif [ "$BUILD_COMMAND" == "klte_tmo" ]; then
	SIGN_MODEL=SM-G900T_NA_TMB_ROOT0
elif [ "$BUILD_COMMAND" == "klte_vzw" ]; then
	SIGN_MODEL=SM-G900V_NA_VZW_ROOT0
elif [ "$BUILD_COMMAND" == "klte_spr" ]; then
	SIGN_MODEL=SM-G900P_NA_SPR_ROOT0
elif [ "$BUILD_COMMAND" == "klte_usc" ]; then
	SIGN_MODEL=SM-G900R4_NA_USC_ROOT0
elif [ "$BUILD_COMMAND" == "kactivelte_att" ]; then
	SIGN_MODEL=SM-G870A_NA_ATT_ROOT0
elif [ "$BUILD_COMMAND" == "ksportslte_spr" ]; then
	SIGN_MODEL=SM-G860P_NA_SPR_ROOT0
else
	SIGN_MODEL=
fi

MODEL=${BUILD_COMMAND%%_*}
TEMP=${BUILD_COMMAND#*_}
REGION=${TEMP%%_*}
CARRIER=${TEMP##*_}

if [[ "$BUILD_COMMAND" == "klte"* ]]; then		# KLTE
	VARIANT=k${CARRIER}
	DTS_NAMES=msm8974pro-ac-sec-k-
elif [[ "$BUILD_COMMAND" == "kactivelte"* ]]; then	# KACTIVE
	VARIANT=kactive${CARRIER}
	DTS_NAMES=msm8974pro-ac-sec-kactivelte-
elif [[ "$BUILD_COMMAND" == "ksportslte"* ]]; then	# KSPORTS
	VARIANT=ksports${CARRIER}
	DTS_NAMES=msm8974pro-ac-sec-ksports-
elif [[ "$BUILD_COMMAND" == "slte"* ]]; then		# SLTE
	VARIANT=s${CARRIER}
	DTS_NAMES=msm8974pro-ac-sec-s-
elif [[ "$BUILD_COMMAND" == "pateklte"* ]]; then	# PATEKLTE
	VARIANT=patek${CARRIER}
	DTS_NAMES=msm8974pro-ac-sec-patek-
else
	DTS_NAMES=
fi

PROJECT_NAME=${VARIANT}
VARIANT_DEFCONFIG=msm8974pro_sec_${MODEL}_${CARRIER}_defconfig

CERTIFICATION=NONCERT

case $1 in
		clean)
		#echo "Clean..."
		echo "Not support... remove kernel out directory by yourself"
		#make -C $BUILD_KERNEL_DIR clean
		#make -C $BUILD_KERNEL_DIR distclean
		#rm $BUILD_KERNEL_OUT_DIR -rf
		exit 1
		;;

		*)
		BUILD_KERNEL_OUT_DIR=$BUILD_TOP_DIR/okernel/$BUILD_COMMAND
		PRODUCT_OUT=$BUILD_TOP_DIR/okernel/$BUILD_COMMAND

		BOARD_KERNEL_BASE=0x00000000
		BOARD_KERNEL_PAGESIZE=2048
		BOARD_KERNEL_TAGS_OFFSET=0x01E00000
		BOARD_RAMDISK_OFFSET=0x02000000
		BOARD_KERNEL_CMDLINE="console=ttyHSL0,115200,n8 androidboot.hardware=qcom user_debug=31 msm_rtb.filter=0x37 ehci-hcd.park=3"
		#BOARD_KERNEL_CMDLINE="console=ttyHSL0,115200,n8 androidboot.hardware=qcom user_debug=31 msm_rtb.filter=0x37 ehci-hcd.park=3"
		mkdir -p $BUILD_KERNEL_OUT_DIR
		;;

esac

KERNEL_ZIMG=$BUILD_KERNEL_OUT_DIR/arch/arm/boot/zImage
DTC=$BUILD_KERNEL_OUT_DIR/scripts/dtc/dtc

FUNC_APPEND_DTB()
{
	if ! [ -d $BUILD_KERNEL_OUT_DIR/arch/arm/boot ] ; then
		echo "error no directory : "$BUILD_KERNEL_OUT_DIR/arch/arm/boot""
		exit -1
	else
		echo "rm files in : "$BUILD_KERNEL_OUT_DIR/arch/arm/boot/*-zImage""
		rm $BUILD_KERNEL_OUT_DIR/arch/arm/boot/*-zImage
		echo "rm files in : "$BUILD_KERNEL_OUT_DIR/arch/arm/boot/*.dtb""
		rm $BUILD_KERNEL_OUT_DIR/arch/arm/boot/*.dtb
	fi
	
	for DTS_FILE in `ls ${BUILD_KERNEL_DIR}/arch/arm/boot/dts/msm8974pro/${DTS_NAMES}*.dts`
	do
		DTB_FILE=${DTS_FILE%.dts}.dtb
		DTB_FILE=$BUILD_KERNEL_OUT_DIR/arch/arm/boot/${DTB_FILE##*/}
		ZIMG_FILE=${DTB_FILE%.dtb}-zImage
		
		echo ""
		echo "dts : $DTS_FILE"
		echo "dtb : $DTB_FILE"
		echo "out : $ZIMG_FILE"
		echo ""
		
		$DTC -p 1024 -O dtb -o $DTB_FILE $DTS_FILE
		cat $KERNEL_ZIMG $DTB_FILE > $ZIMG_FILE
	done
}

INSTALLED_DTIMAGE_TARGET=${BUILD_KERNEL_OUT_DIR}/dt.img
DTBTOOL=$BUILD_TOP_DIR/kernel/tools/dtbTool

FUNC_BUILD_DTIMAGE_TARGET()
{
	echo ""
	echo "================================="
	echo "START : FUNC_BUILD_DTIMAGE_TARGET"
	echo "================================="
	echo ""
	echo "DT image target : $INSTALLED_DTIMAGE_TARGET"
	
	if ! [ -e $DTBTOOL ] ; then
		if ! [ -d $BUILD_TOP_DIR/out/host/linux-x86/bin ] ; then
			mkdir -p $BUILD_TOP_DIR/out/host/linux-x86/bin
		fi
		cp $BUILD_TOP_DIR/kernel/tools/dtbTool $DTBTOOL
	fi

	echo "$DTBTOOL -o $INSTALLED_DTIMAGE_TARGET -s $BOARD_KERNEL_PAGESIZE \
		-p $BUILD_KERNEL_OUT_DIR/scripts/dtc/ $BUILD_KERNEL_OUT_DIR/arch/arm/boot/"
		$DTBTOOL -o $INSTALLED_DTIMAGE_TARGET -s $BOARD_KERNEL_PAGESIZE \
		-p $BUILD_KERNEL_OUT_DIR/scripts/dtc/ $BUILD_KERNEL_OUT_DIR/arch/arm/boot/

	chmod a+r $INSTALLED_DTIMAGE_TARGET

	echo ""
	echo "================================="
	echo "END   : FUNC_BUILD_DTIMAGE_TARGET"
	echo "================================="
	echo ""
}

FUNC_BUILD_KERNEL()
{
	echo ""
	echo "=============================================="
	echo "START : FUNC_BUILD_KERNEL"
	echo "=============================================="
	echo ""
	echo "build project="$PROJECT_NAME""
	echo "build common config="$KERNEL_DEFCONFIG ""
	echo "build variant config="$VARIANT_DEFCONFIG ""
	echo "build secure option="$SECURE_OPTION ""
	echo "build SEANDROID option="$SEANDROID_OPTION ""

	if [ "$BUILD_COMMAND" == "" ]; then
		SECFUNC_PRINT_HELP;
		exit -1;
	fi

	if ! [ -e $PRODUCT_OUT/ramdisk.img ] ; then
		echo "error no ramdisk : "$PRODUCT_OUT/ramdisk.img ""
		exit -1
	fi

	make -C $BUILD_KERNEL_DIR O=$BUILD_KERNEL_OUT_DIR -j$BUILD_JOB_NUMBER ARCH=arm \
			CROSS_COMPILE=$BUILD_CROSS_COMPILE \
			$KERNEL_DEFCONFIG VARIANT_DEFCONFIG=$VARIANT_DEFCONFIG \
			DEBUG_DEFCONFIG=$DEBUG_DEFCONFIG SELINUX_DEFCONFIG=$SELINUX_DEFCONFIG \
			SELINUX_LOG_DEFCONFIG=$SELINUX_LOG_DEFCONFIG || exit -1

	make -C $BUILD_KERNEL_DIR O=$BUILD_KERNEL_OUT_DIR -j$BUILD_JOB_NUMBER ARCH=arm \
			CROSS_COMPILE=$BUILD_CROSS_COMPILE || exit -1

	FUNC_APPEND_DTB
	FUNC_BUILD_DTIMAGE_TARGET
	
	echo ""
	echo "================================="
	echo "END   : FUNC_BUILD_KERNEL"
	echo "================================="
	echo ""
}

FUNC_MKBOOTIMG()
{
	echo ""
	echo "==================================="
	echo "START : FUNC_MKBOOTIMG"
	echo "==================================="
	echo ""
	MKBOOTIMGTOOL=$BUILD_TOP_DIR/kernel/tools/mkbootimg

	if ! [ -e $MKBOOTIMGTOOL ] ; then
		if ! [ -d $BUILD_TOP_DIR/out/host/linux-x86/bin ] ; then
			mkdir -p $BUILD_TOP_DIR/out/host/linux-x86/bin
		fi
		cp $BUILD_TOP_DIR/kernel/tools/mkbootimg $MKBOOTIMGTOOL
	fi

	echo "Making boot.img ..."
	echo "	$MKBOOTIMGTOOL --kernel $KERNEL_ZIMG \
			--ramdisk $PRODUCT_OUT/ramdisk.img \
			--output $PRODUCT_OUT/boot.img \
			--cmdline "$BOARD_KERNEL_CMDLINE" \
			--base $BOARD_KERNEL_BASE \
			--pagesize $BOARD_KERNEL_PAGESIZE \
			--ramdisk_offset $BOARD_RAMDISK_OFFSET \
			--tags_offset $BOARD_KERNEL_TAGS_OFFSET \
			--dt $INSTALLED_DTIMAGE_TARGET"
			
	$MKBOOTIMGTOOL --kernel $KERNEL_ZIMG \
			--ramdisk $PRODUCT_OUT/ramdisk.img \
			--output $PRODUCT_OUT/boot.img \
			--cmdline "$BOARD_KERNEL_CMDLINE" \
			--base $BOARD_KERNEL_BASE \
			--pagesize $BOARD_KERNEL_PAGESIZE \
			--ramdisk_offset $BOARD_RAMDISK_OFFSET \
			--tags_offset $BOARD_KERNEL_TAGS_OFFSET \
			--dt $INSTALLED_DTIMAGE_TARGET

	if [ "$SEANDROID_OPTION" == "-E" ] ; then
		FUNC_SEANDROID
	fi

	if [ "$SECURE_OPTION" == "-B" ]; then
		FUNC_SECURE_SIGNING
	fi

	cd $PRODUCT_OUT
	tar cvf boot_${MODEL}_${CARRIER}_${CERTIFICATION}.tar boot.img

	if ! [ -d $BUILD_TOP_DIR/kernel/okernel/$BUILD_COMMAND ] ; then
		mkdir -p $BUILD_TOP_DIR/kernel/okernel/$BUILD_COMMAND
	else
		rm $BUILD_TOP_DIR/kernel/okernel/$BUILD_COMMAND/boot_${MODEL}_${CARRIER}_${CERTIFICATION}.tar -f
	fi

        echo ""
	echo "================================================="
        echo "-->Note, copy to $BUILD_TOP_DIR/../output/ directory"
	echo "================================================="
	cp ../$PRODUCT_OUT/boot_${MODEL}_${CARRIER}_${CERTIFICATION}.tar $BUILD_TOP_DIR/../../output/boot_${MODEL}_${CARRIER}_${CERTIFICATION}.tar || exit -1
        cd ~

	echo ""
	echo "==================================="
	echo "END   : FUNC_MKBOOTIMG"
	echo "==================================="
	echo ""
}

FUNC_SEANDROID()
{
	echo -n "SEANDROIDENFORCE" >> $PRODUCT_OUT/boot.img
}

FUNC_SECURE_SIGNING()
{
	echo "java -jar $SECURE_SCRIPT -model $SIGN_MODEL -runtype ss_openssl_sha -input $PRODUCT_OUT/boot.img -output $PRODUCT_OUT/signed_boot.img"
	openssl dgst -sha256 -binary $PRODUCT_OUT/boot.img > sig_32
	java -jar $SECURE_SCRIPT -runtype ss_openssl_sha -model $SIGN_MODEL -input sig_32 -output sig_256
	cat $PRODUCT_OUT/boot.img sig_256 > $PRODUCT_OUT/signed_boot.img

	mv -f $PRODUCT_OUT/boot.img $PRODUCT_OUT/unsigned_boot.img
	mv -f $PRODUCT_OUT/signed_boot.img $PRODUCT_OUT/boot.img

	CERTIFICATION=CERT
}

SECFUNC_PRINT_HELP()
{
	echo -e '\E[33m'
	echo "Help"
	echo "$0 \$1 \$2 \$3"
	echo "  \$1 : "
	echo "      klte_eur"
	echo "      klte_att"
	echo "      klte_spr"
	echo "      klte_tmo"
	echo "      klte_vzw"
	echo "      klte_usc"
	echo "      kactivelte_att"
	echo "      ksportslte_spr"
	echo "      slte_att"
	echo "      pateklte_ctc"
	echo "  \$2 : "
	echo "      -B or Nothing  (-B : Secure Binary)"
	echo "  \$3 : "
	echo "      -E or Nothing  (-E : SEANDROID Binary)"
	echo -e '\E[0m'
}


# MAIN FUNCTION
rm -rf ./build.log
(
	START_TIME=`date +%s`

	FUNC_BUILD_KERNEL
	#FUNC_RAMDISK_EXTRACT_N_COPY
	FUNC_MKBOOTIMG

	END_TIME=`date +%s`

	let "ELAPSED_TIME=$END_TIME-$START_TIME"
	echo "Total compile time is $ELAPSED_TIME seconds"
) 2>&1	 | tee -a ./build.log
