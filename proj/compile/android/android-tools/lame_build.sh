#!/bin/bash

UNAME_S=$(uname -s)
UNAME_SM=$(uname -sm)
echo "build on $UNAME_SM"

echo "ANDROID_SDK=$ANDROID_SDK"
echo "ANDROID_NDK=$ANDROID_NDK"

if [ -z "$ANDROID_NDK" -o -z "$ANDROID_SDK" ]; then
    echo "You must define ANDROID_NDK, ANDROID_SDK before starting."
    echo "They must point to your NDK and SDK directories."
    echo ""
    exit 1
fi

#--------------------
# common defines
ARCH=$1
if [ -z "$ARCH" ]; then
    echo "You must specific an architecture 'arm, armv7a, x86, ...'."
    echo ""
    exit 1
fi

source ./android-tools/android-toolchain-build.sh $ARCH
SYSROOT=$TOOLCHAIN_PATH/sysroot

echo $TOOLCHAIN_PATH
echo $ANDROID_PLATFORM
echo $CROSS_PREFIX

export PATH=$TOOLCHAIN_PATH/bin:$TOOLCHAIN_PATH/$CROSS_PREFIX/bin:$PATH
echo $PATH

export CC="${CROSS_PREFIX}-gcc --sysroot=$SYSROOT"
export ADDR2LINE=${CROSS_PREFIX}-addr2line
export AR=${CROSS_PREFIX}-ar
export AS=${CROSS_PREFIX}-as
export CPP=${CROSS_PREFIX}-cpp
export DWP=${CROSS_PREFIX}-dwp
export ELFEDIT=${CROSS_PREFIX}-elfedit
export CXX=${CROSS_PREFIX}-g++
export GCOV=${CROSS_PREFIX}-gcov
export GDB=${CROSS_PREFIX}-gdb
export GPROF=${CROSS_PREFIX}-gprof
export LD=${CROSS_PREFIX}-ld
export NM=${CROSS_PREFIX}-nm
export OBJCOPY=${CROSS_PREFIX}-objcopy
export OBJDUMP=${CROSS_PREFIX}-objdump
export RANLIB=${CROSS_PREFIX}-ranlib
export READELF=${CROSS_PREFIX}-readelf
export SIZE=${CROSS_PREFIX}-size
export STRINGS=${CROSS_PREFIX}-strings
export STRIP=${CROSS_PREFIX}-strip


MAKEFLAGS=
if which nproc >/dev/null
then
    MAKEFLAGS=-j`nproc`
elif [ "$UNAMES" = "Darwin" ] && which sysctl >/dev/null
then
    MAKEFLAGS=-j`sysctl -n machdep.cpu.thread_count`
fi

#----- armv7a begin -----
if [ "$ARCH" = "armv7a" ]; then
    HOST=arm-linux
    CFLAGS="$CFLAGS -march=armv7-a -mcpu=cortex-a8 -mfpu=vfpv3-d16 -mfloat-abi=softfp -mthumb"
    LDFLAGS="$LDFLAGS -Wl,--fix-cortex-a8"
elif [ "$ARCH" = "armv5" ]; then
    HOST=arm-linux
elif [ "$ARCH" = "x86" ]; then
    HOST=i686-linux
elif [ "$ARCH" = "x86_64" ]; then
    HOST=x86_64
elif [ "$ARCH" = "arm64" ]; then
#    CFLAGS="$CFLAGS -march=arm64 -mthumb"

    HOST=arm-linux
else
    echo "unknown architecture $ARCH";
    exit 1
fi

BUILD_ROOT=`pwd`
PROJECT_DIR=lame
PREFIX=$BUILD_ROOT/android-build/$PROJECT_DIR/$ARCH
mkdir -p $PREFIX

CFG_FLAGS="$CFG_FLAGS --prefix=$PREFIX"
CFG_FLAGS="$CFG_FLAGS --host=$HOST"
CFG_FLAGS="$CFG_FLAGS --with-pic"
CFG_FLAGS="$CFG_FLAGS --enable-static"
CFG_FLAGS="$CFG_FLAGS --disable-shared"
CFG_FLAGS="$CFG_FLAGS --with-gnu-ld"
CFG_FLAGS="$CFG_FLAGS --enable-nasm"
CFG_FLAGS="$CFG_FLAGS  --target=aarch64-linux"

export CFLAGS=$CFLAGS
export LDFLAGS=$LDFLAGS

echo "**$CFG_FLAGS** PROJECT_DIR :$PROJECT_DIR"

cd $BUILD_ROOT/android-build/$PROJECT_DIR
DIR=`pwd`
echo $DIR
./configure $CFG_FLAGS

#--------------------
echo ""
echo "--------------------"
echo "[*] compile $PROJECT_DIR"
echo "--------------------"
make clean
make $MAKEFLAGS -j8 
make install

