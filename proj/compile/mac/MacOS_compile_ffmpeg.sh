#!/bin/sh

source ./ffmpeg_module_config.cfg

# directories
SOURCE="ffmpeg-3.0"
FAT="FFmpeg-iOS"


CUR_WORK_ROOT_DIR=$(pwd)
TARGET_LIBRARY_DIR=$CUR_WORK_ROOT_DIR/../../../lib
DEPEND_THIRDPARTY_LIBRARY_DIR=${TARGET_LIBRARY_DIR}
FFMPEG_OUTPUT_TARGET_DIR=$TARGET_LIBRARY_DIR/ffmpeg
FFMPEG_PROJ_SRC_ROOT_PATH=${CUR_WORK_ROOT_DIR}/../../../ffmpeg-3.2/

SCRATCH="scratch"
ARCHS="MacOS"

CONFIGURE_FLAGS="--enable-static --disable-shared --disable-optimizations --disable-asm --disable-stripping --enable-debug=3 --disable-doc --enable-pic"
## =====================================>>>

# X264
CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-gpl --enable-libx264"
CONFIGURE_FLAGS="$CONFIGURE_FLAGS --extra-cflags=-I${DEPEND_THIRDPARTY_LIBRARY_DIR}/x264/include"
CONFIGURE_FLAGS="$CONFIGURE_FLAGS --extra-ldflags=-L${DEPEND_THIRDPARTY_LIBRARY_DIR}/x264/lib"

# FDK_AAC
CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-libfdk-aac --enable-nonfree --enable-encoder=aac"
CONFIGURE_FLAGS="$CONFIGURE_FLAGS --extra-cflags=-I${DEPEND_THIRDPARTY_LIBRARY_DIR}/fdk-aac/include"
CONFIGURE_FLAGS="$CONFIGURE_FLAGS --extra-ldflags=-L${DEPEND_THIRDPARTY_LIBRARY_DIR}/fdk-aac/lib"

# libmp3lame
CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-libmp3lame"
CONFIGURE_FLAGS="$CONFIGURE_FLAGS --extra-cflags=-I${DEPEND_THIRDPARTY_LIBRARY_DIR}/lame/include"
CONFIGURE_FLAGS="$CONFIGURE_FLAGS --extra-ldflags=-L${DEPEND_THIRDPARTY_LIBRARY_DIR}/lame/lib"

# openjpeg
CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-libopenjpeg"
CONFIGURE_FLAGS="$CONFIGURE_FLAGS --extra-cflags=-I${DEPEND_THIRDPARTY_LIBRARY_DIR}/openjpeg/include"
CONFIGURE_FLAGS="$CONFIGURE_FLAGS --extra-ldflags=-L${DEPEND_THIRDPARTY_LIBRARY_DIR}/openjpeg/lib"

# 模块
CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-avresample"
## =====================================>>>


#ARCHS="arm64 armv7 x86_64 i386"

echo "building $ARCHS..."
mkdir -p "$FFMPEG_OUTPUT_TARGET_DIR/$ARCHS"

cd ${FFMPEG_PROJ_SRC_ROOT_PATH} \
&& ./configure \
	${COMMON_FF_CFG_FLAGS} \
	$CONFIGURE_FLAGS \
	--prefix="$FFMPEG_OUTPUT_TARGET_DIR/$ARCHS" \
	|| exit 1
make clean 
make -j8 install $EXPORT || exit 1
cd $CWD


echo Done
