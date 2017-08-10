#!/bin/sh

source ffmpeg_module_config.cfg

# directories
SOURCE="ffmpeg-3.0"
FAT="FFmpeg-iOS"


CUR_WORK_ROOT_DIR=$(pwd)
TARGET_LIBRARY_DIR=$CUR_WORK_ROOT_DIR/../lib
DEPEND_THIRDPARTY_LIBRARY_DIR=$CUR_WORK_ROOT_DIR/../lib
FFMPEG_OUTPUT_TARGET_DIR=$TARGET_LIBRARY_DIR/ffmpeg

SCRATCH="scratch"

# must be an absolute path
# THIN=`pwd`/"thin"
## =====================================>>>

# absolute path to x264 library
# X264=$TARGET_LIBRARY_DIR/x264

#FDK_AAC=`pwd`/fdk-aac/fdk-aac-ios

# --enable-debug  就是gcc 中添加-g选项， 3是-g的级别
# CONFIGURE_FLAGS="--enable-shared --disable-optimizations --disable-asm --disable-stripping --enable-debug --extra-cflags=-g --extra-ldflags=-g --enable-debug=3 --disable-doc --enable-pic"
CONFIGURE_FLAGS="--enable-static --disable-shared --disable-optimizations --disable-asm --disable-stripping --enable-debug=3 --disable-doc --enable-pic"
CONFIGURE_FLAGS="$CONFIGURE_FLAGS --extra-cflags=-I${DEPEND_THIRDPARTY_LIBRARY_DIR}/fdk-aac/include --extra-cflags=-I${DEPEND_THIRDPARTY_LIBRARY_DIR}/x264/include"
CONFIGURE_FLAGS="$CONFIGURE_FLAGS --extra-ldflags=-L${DEPEND_THIRDPARTY_LIBRARY_DIR}/fdk-aac/lib --extra-ldflags=-L${DEPEND_THIRDPARTY_LIBRARY_DIR}/x264/lib"
## =====================================>>>

# if [ "$X264" ]
# then
	CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-gpl --enable-libx264"
# fi

# if [ "$FDK_AAC" ]
# then
	CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-libfdk-aac --enable-nonfree --enable-encoder=aac"
# fi

# avresample
CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-avresample"
## =====================================>>>


#ARCHS="arm64 armv7 x86_64 i386"
ARCHS="MacOS"

echo "building $ARCH..."
mkdir -p "$FFMPEG_OUTPUT_TARGET_DIR/$ARCH"
cd "$FFMPEG_OUTPUT_TARGET_DIR/$ARCH"

echo "$FFMPEG_OUTPUT_TARGET_DIR/$ARCH"
TMPDIR=${TMPDIR/%\/} $CUR_WORK_ROOT_DIR/configure \
	${COMMON_FF_CFG_FLAGS} \
	$CONFIGURE_FLAGS \
	--prefix="$FFMPEG_OUTPUT_TARGET_DIR/$ARCH" \
	|| exit 1

make -j3 install $EXPORT || exit 1
cd $CWD


echo Done
