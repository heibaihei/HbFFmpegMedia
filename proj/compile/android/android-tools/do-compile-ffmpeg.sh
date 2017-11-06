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
#export AS=${CROSS_PREFIX}-as # 打开此选项后导致asm使用as编译会产生TEXTREL信息, 关闭后自动使用gcc编译asm不会产生TEXTREL
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


EXTRA_CFLAGS=
EXTRA_LDFLAGS=
DEP_LIBS=
ASM_OBJ_DIR=


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

CFG_FLAGS="$CFG_FLAGS --arch=arm --cpu=cortex-a8"
CFG_FLAGS="$CFG_FLAGS --enable-neon"
CFG_FLAGS="$CFG_FLAGS --enable-thumb"

EXTRA_CFLAGS="$EXTRA_CFLAGS -march=armv7-a -mcpu=cortex-a8 -mfpu=vfpv3-d16 -mfloat-abi=softfp -mthumb"
EXTRA_LDFLAGS="$EXTRA_LDFLAGS -Wl,--fix-cortex-a8"
ASM_OBJ_DIR="libavcodec/arm/*.o libavutil/arm/*.o libswresample/arm/*.o libswscale/arm/*.o"

elif [ "$ARCH" = "armv5" ]; then

CFG_FLAGS="$CFG_FLAGS --arch=arm"

EXTRA_CFLAGS="$EXTRA_CFLAGS -march=armv5te -mtune=arm9tdmi -msoft-float"
EXTRA_LDFLAGS="$EXTRA_LDFLAGS"

ASM_OBJ_DIR="libavcodec/arm/*.o libavutil/arm/*.o libswresample/arm/*.o"

elif [ "$ARCH" = "x86" ]; then

CFG_FLAGS="$CFG_FLAGS --arch=x86 --cpu=i686 --enable-yasm"

EXTRA_CFLAGS="$EXTRA_CFLAGS -march=atom -msse3 -ffast-math -mfpmath=sse"
EXTRA_LDFLAGS="$EXTRA_LDFLAGS"

ASM_OBJ_DIR="libavcodec/x86/*.o libavfilter/x86/*.o libavutil/x86/*.o libswresample/x86/*.o libswscale/x86/*.o"

elif [ "$ARCH" = "x86_64" ]; then

CFG_FLAGS="$CFG_FLAGS --arch=x86_64 --enable-yasm"

EXTRA_CFLAGS="$EXTRA_CFLAGS"
EXTRA_LDFLAGS="$EXTRA_LDFLAGS"

ASM_OBJ_DIR="libavcodec/x86/*.o libavfilter/x86/*.o libavutil/x86/*.o libswresample/x86/*.o libswscale/x86/*.o"

elif [ "$ARCH" = "arm64" ]; then

CFG_FLAGS="$CFG_FLAGS --arch=aarch64 --enable-yasm"

EXTRA_CFLAGS="$EXTRA_CFLAGS"
EXTRA_LDFLAGS="$EXTRA_LDFLAGS"

ASM_OBJ_DIR="libavcodec/aarch64/*.o libavutil/aarch64/*.o libswscale/aarch64/*.o libswresample/aarch64/*.o libavcodec/neon/*.o"

else
echo "unknown architecture $ARCH";
exit 1
fi

BUILD_ROOT=`pwd`
PROJECT_DIR=ffmpeg

PREFIX=$BUILD_ROOT/android-build/$PROJECT_DIR/$ARCH
mkdir -p $PREFIX

EXT_CFLAGS="-O3 -Wall -pipe \
-std=c99 \
-ffast-math \
-fstrict-aliasing -Werror=strict-aliasing \
-Wno-psabi -Wa,--noexecstack "

export COMMON_FF_CFG_FLAGS=
. $BUILD_ROOT/ffmpeg_module_android.sh
CFG_FLAGS="$CFG_FLAGS $COMMON_FF_CFG_FLAGS"

CFG_FLAGS="$CFG_FLAGS --prefix=${PREFIX}"
CFG_FLAGS="$CFG_FLAGS --cross-prefix=${CROSS_PREFIX}-"

CFG_FLAGS="$CFG_FLAGS --disable-doc"
CFG_FLAGS="$CFG_FLAGS --enable-cross-compile"
CFG_FLAGS="$CFG_FLAGS --target-os=linux"
CFG_FLAGS="$CFG_FLAGS --enable-pic"
CFG_FLAGS="$CFG_FLAGS --enable-asm"
CFG_FLAGS="$CFG_FLAGS --disable-xlib"
#CFG_FLAGS="$CFG_FLAGS --disable-zlib"
CFG_FLAGS="$CFG_FLAGS --disable-debug"
CFG_FLAGS="$CFG_FLAGS --enable-small"


#CFG_FLAGS="$CFG_FLAGS --enable-jni"        # android hardware decode
#CFG_FLAGS="$CFG_FLAGS --enable-mediacodec"

CFG_FLAGS="$CFG_FLAGS --enable-inline-asm"
CFG_FLAGS="$CFG_FLAGS --enable-gpl"        # for x264
CFG_FLAGS="$CFG_FLAGS --enable-nonfree"    # for fdk_aac

# 3rdparty define
DEP_X264_INC=$BUILD_ROOT/android-build/x264/$ARCH/include
DEP_X264_LIB=$BUILD_ROOT/android-build/x264/$ARCH/lib
DEP_AAC_INC=$BUILD_ROOT/android-build/fdk-aac/$ARCH/include
DEP_AAC_LIB=$BUILD_ROOT/android-build/fdk-aac/$ARCH/lib
DEP_LAME_INC=$BUILD_ROOT/android-build/lame/$ARCH/include
DEP_LAME_LIB=$BUILD_ROOT/android-build/lame/$ARCH/lib

echo "DEP_X264_LIB :${DEP_X264_LIB}"
#--------------------
# with x264
if [ -f "${DEP_X264_LIB}/libx264.a" ]; then
    echo "enbale ----------libx264"
    CFG_FLAGS="$CFG_FLAGS --enable-libx264"
    CFG_FLAGS="$CFG_FLAGS --enable-encoder=libx264"

    EXT_CFLAGS="${EXT_CFLAGS} -I${DEP_X264_INC}"
    DEP_LIBS="${DEP_LIBS} -L${DEP_X264_LIB} -lx264"
fi
#--------------------

#--------------------
# with fdk-aac
if [ -f "${DEP_AAC_LIB}/libfdk-aac.a" ]; then
    echo "enbale ----------fdk-aac"
    CFG_FLAGS="$CFG_FLAGS --enable-libfdk-aac"
    CFG_FLAGS="$CFG_FLAGS --enable-encoder=libfdk_aac"

    EXT_CFLAGS="${EXT_CFLAGS} -I${DEP_AAC_INC}"
    DEP_LIBS="${DEP_LIBS} -L${DEP_AAC_LIB} -lfdk-aac"
fi
#--------------------

#--------------------
# with lame
if [ -f "${DEP_LAME_LIB}/libmp3lame.a" ]; then
    echo "enbale ----------lame"
    CFG_FLAGS="$CFG_FLAGS --enable-libmp3lame"
    CFG_FLAGS="$CFG_FLAGS --enable-encoder=libmp3lame"

    EXT_CFLAGS="${EXT_CFLAGS} -I${DEP_LAME_INC}"
    DEP_LIBS="${DEP_LIBS} -L${DEP_LAME_LIB} -lmp3lame"
fi
#--------------------


cd $BUILD_ROOT/android-build/$PROJECT_DIR
./configure $CFG_FLAGS \
--extra-cflags="$EXT_CFLAGS $EXTRA_CFLAGS" \
--extra-ldflags="$DEP_LIBS $EXTRA_LDFLAGS" \
--extra-libs="-ldl"


#--------------------
echo ""
echo "--------------------"
echo "[*] compile $PROJECT_DIR"
echo "--------------------"
make clean
make $MAKEFLAGS -j8
make install
mkdir -p $PREFIX/include/libffmpeg
cp -f config.h $PREFIX/include/libffmpeg/config.h
#--------------------

echo ""
echo "--------------------"
echo "[*] link ffmpeg"
echo "--------------------"
echo $EXTRA_LDFLAGS
$CC -lm -lz -shared --sysroot=$SYSROOT -Wl,--no-undefined -Wl,-z,noexecstack $EXTRA_LDFLAGS \
    -Wl,-soname,libffmpeg.so \
    compat/*.o \
    libavcodec/*.o \
    libavfilter/*.o \
    libavformat/*.o \
    libavutil/*.o \
    libswresample/*.o \
    libswscale/*.o \
    $ASM_OBJ_DIR \
    $DEP_LIBS \
    -o $PREFIX/libffmpeg.so

mysedi() {
    f=$1
    exp=$2
    n=`basename $f`
    cp $f /tmp/$n
    sed $exp /tmp/$n > $f
    rm /tmp/$n
}

echo ""
echo "--------------------"
echo "[*] create files for shared ffmpeg"
echo "--------------------"
rm -rf $PREFIX/shared
mkdir -p $PREFIX/shared/lib/pkgconfig
ln -s $PREFIX/include $PREFIX/shared/include
ln -s $PREFIX/libffmpeg.so $PREFIX/shared/lib/libffmpeg.so
cp $PREFIX/lib/pkgconfig/*.pc $PREFIX/shared/lib/pkgconfig
for f in $PREFIX/lib/pkgconfig/*.pc; do
    # in case empty dir
    if [ ! -f $f ]; then
        continue
    fi
    cp $f $PREFIX/shared/lib/pkgconfig
    f=$PREFIX/shared/lib/pkgconfig/`basename $f`
    # OSX sed doesn't have in-place(-i)
    mysedi $f 's/\/output/\/output\/shared/g'
    mysedi $f 's/-lavcodec/-lffmpeg/g'
    mysedi $f 's/-lavfilter/-lffmpeg/g'
    mysedi $f 's/-lavformat/-lffmpeg/g'
    mysedi $f 's/-lavutil/-lffmpeg/g'
    mysedi $f 's/-lswresample/-lffmpeg/g'
    mysedi $f 's/-lswscale/-lffmpeg/g'
done

