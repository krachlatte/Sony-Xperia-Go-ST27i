#!/bin/bash

die () {
    echo >&2 "$@"
    exit 1
}

[ "$1" = "" ] && die "product{kumquat/pdp/..} has to be specified as first argument"
[ "$2" = "" ] && die "fsconfig.xml has to be specified as second argument"
[ "$3" = "" ] && die "ramdisk.img has to be specified as third argument"


case "$1" in
     pdp)
         PRODUCT_DEFCNFG=pdp_defconfig
         ;;
     kumquat)
         PRODUCT_DEFCNFG=semc_kumquat_defconfig
         ;;
     pepper)
         PRODUCT_DEFCNFG=semc_pepper_defconfig
         ;;
     pdp_r2)
         PRODUCT_DEFCNFG=pdp_r2_defconfig
         ;;
     nypon)
         PRODUCT_DEFCNFG=semc_nypon_defconfig
         ;;
     lotus)
         PRODUCT_DEFCNFG=semc_lotus_defconfig
         ;;
     *)
        echo "Error!You need to double check your product name"
        exit 1
esac

#---------------------------------------------------------------------
# Check the actual number of CPUs on the machine and adjust the number
#---------------------------------------------------------------------
CPU_COUNT=`grep "^processor" /proc/cpuinfo | wc -l`
if [ $? -eq 0 -a -n "$CPU_COUNT" ] ; then
    JOBS=`expr $CPU_COUNT + 1`
    echo Found $CPU_COUNT CPUs, building with $JOBS parallel jobs.
else
    JOBS=5
    echo Unable to determine number of CPUs, defaulting to $JOBS parallel jobs.
fi

#------------------------------------------------------------
# Need this to be able to build. Taken from kernel/Android.mk
#------------------------------------------------------------
export STERICCSON_WLAN_BUILT_IN=y

if [ $4 ] && [ $4 == "clean" ] ; then
    echo 'Cleaning build...'
    #-------------------
    # Clean up the build
    #-------------------
    ARCH=arm CROSS_COMPILE=../prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi- make mrproper
fi

if [ ! -e ".config" ] ; then
    echo 'No .config file, generating...'
    #---------------------------
    # kernel configuration setup
    #---------------------------
    ARCH=arm CROSS_COMPILE=../prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi- make $PRODUCT_DEFCNFG
fi

#------
# Build
#------
ARCH=arm CROSS_COMPILE=../prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi- make -j$JOBS

if [ $? -ne 0 ] ; then
    echo "Build error - skip signing"
    exit 1
fi

#------
# Sign
#------
FSCONFIG=$2
RAMDISK=$3

SIN_PATH=../vendor/semc/build/sin

MKELFPY=mkelf.py
type -p $MKELFPY &>/dev/null || MKELFPY=$SIN_PATH/mkelf.py
SEMCSC=semcsc.py
type -p $SEMCSC &>/dev/null || SEMCSC=$SIN_PATH/semcsc.py
CSH=create_sin_header
type -p $CSH &>/dev/null || CSH=$SIN_PATH/create_sin_header

KERNEL=arch/arm/boot/zImage

# kernel.elf
$MKELFPY -o kernel-unsigned.elf $KERNEL@0x00008000 $RAMDISK@0x1000000,ramdisk
$SEMCSC -c $FSCONFIG -p Kernel -t internal -i kernel-unsigned.elf -o kernel.elf

# kernel.si_
$CSH Kernel $FSCONFIG kernel.si_
cat kernel.elf >> kernel.si_

# kernel.sin
$SEMCSC -c $FSCONFIG -p Kernel -t external -i kernel.si_ -o kernel.sin

