#! /usr/bin/env bash
#
# Copyright (C) 2013-2014 Zhang Rui <bbcallen@gmail.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#----------
# modify for your build tool

FF_ALL_ARCHS_IOS6_SDK="armv7 armv7s i386"
FF_ALL_ARCHS_IOS7_SDK="armv7 armv7s arm64 i386 x86_64"
FF_ALL_ARCHS_IOS8_SDK="armv7 arm64 i386 x86_64"
FF_ALL_ARCHS_SDK="armv7 arm64 x86_64"
FF_ALL_ARCHS=$FF_ALL_ARCHS_SDK

#----------
UNI_BUILD_ROOT=`pwd`
ARCH_OUTPUT_ROOT_PATH=$UNI_BUILD_ROOT/iOS-tools/iOS-build/
UNI_TMP="$UNI_BUILD_ROOT/tmp"
UNI_TMP_LLVM_VER_FILE="$UNI_TMP/llvm.ver.txt"
FF_TARGET=$1
set -e

#----------
echo_archs() {
    echo "===================="
    echo "[*] check xcode version"
    echo "===================="
    echo "FF_ALL_ARCHS = $FF_ALL_ARCHS"
}

#FF_LIBS="libavcodec libavfilter libavformat libavutil libswscale libswresample"
#FF_LIBS="libavcodec libavformat libavutil libswscale libswresample"
FF_LIBS="libavcodec libavformat libavutil libswscale libswresample libavfilter"
do_lipo_universal_lib () {
    # 构建输出库输出目录
    UNIVERSAL_LIB_OUTPUT_PATH=${ARCH_OUTPUT_ROOT_PATH}/universal/lib
    mkdir -p ${UNIVERSAL_LIB_OUTPUT_PATH}
    
    for FF_LIB in $FF_LIBS
    do
        LIPO_FLAGS=
        LIB_FILE="${FF_LIB}.a"
        for ARCH in $FF_ALL_ARCHS
        do
            ARCH_LIB_FILE="${ARCH_OUTPUT_ROOT_PATH}/ffmpeg-$ARCH/output/lib/$LIB_FILE"
            if [ -f "$ARCH_LIB_FILE" ]; then
                LIPO_FLAGS="$LIPO_FLAGS $ARCH_LIB_FILE"
            else
                echo "skip $LIB_FILE of $ARCH";
            fi
        done

        xcrun lipo -create $LIPO_FLAGS -output ${UNIVERSAL_LIB_OUTPUT_PATH}/$LIB_FILE
        xcrun lipo -info ${UNIVERSAL_LIB_OUTPUT_PATH}/$LIB_FILE
    done
}

do_lipo_universal_include () {
    UNI_INC_DIR="${ARCH_OUTPUT_ROOT_PATH}/universal/include"

    ANY_ARCH=
    for ARCH in $FF_ALL_ARCHS
    do
        ARCH_INC_DIR="${ARCH_OUTPUT_ROOT_PATH}/ffmpeg-$ARCH/output/include"
        if [ -d "$ARCH_INC_DIR" ]; then
            if [ -z "$ANY_ARCH" ]; then
                ANY_ARCH=$ARCH
                cp -R "$ARCH_INC_DIR" "${ARCH_OUTPUT_ROOT_PATH}/universal/"
            fi
            

            mkdir -p "$UNI_INC_DIR/libavutil/$ARCH"
            cp -f "$ARCH_INC_DIR/libavutil/avconfig.h"  "$UNI_INC_DIR/libavutil/$ARCH/avconfig.h"
            cp -f iOS-tools/avconfig.h                  "$UNI_INC_DIR/libavutil/avconfig.h"
            cp -f "$ARCH_INC_DIR/libavutil/ffversion.h" "$UNI_INC_DIR/libavutil/$ARCH/ffversion.h"
            cp -f iOS-tools/ffversion.h                 "$UNI_INC_DIR/libavutil/ffversion.h"
            mkdir -p "$UNI_INC_DIR/libffmpeg/$ARCH"
            cp -f "$ARCH_INC_DIR/libffmpeg/config.h"    "$UNI_INC_DIR/libffmpeg/$ARCH/config.h"
            cp -f iOS-tools/config.h                    "$UNI_INC_DIR/libffmpeg/config.h"
        fi
    done
    cp $UNI_BUILD_ROOT/../../../ffmpeg-3.2/libavformat/avc.h  ${UNI_INC_DIR}/libavformat/
}

do_lipo_all () {
    echo "lipo archs: $FF_ALL_ARCHS"
    do_lipo_universal_lib
    do_lipo_universal_include
}

#---------- 解析命令行选项
if [ "$FF_TARGET" = "armv7" -o "$FF_TARGET" = "armv7s" -o "$FF_TARGET" = "arm64" ]; then
    echo_archs
    sh iOS-tools/do-compile-ffmpeg.sh $FF_TARGET
    do_lipo_all
elif [ "$FF_TARGET" = "i386" -o "$FF_TARGET" = "x86_64" ]; then
    echo_archs
    sh iOS-tools/do-compile-ffmpeg.sh $FF_TARGET
    do_lipo_all
elif [ "$FF_TARGET" = "lipo" ]; then
    echo_archs
    do_lipo_all
elif [ "$FF_TARGET" = "all" ]; then
    echo_archs
    for ARCH in $FF_ALL_ARCHS
    do
        cd ${UNI_BUILD_ROOT}/iOS-tools/
        sh do-compile-ffmpeg.sh $ARCH
        cd ${UNI_BUILD_ROOT}
    done

    do_lipo_all
elif [ "$FF_TARGET" = "check" ]; then
    echo_archs
elif [ "$FF_TARGET" = "clean" ]; then
    echo_archs

    rm -rf iOS-build
else
    echo "Usage:"
    echo "  iOS_build.sh armv7|arm64|i386|x86_64"
    echo "  iOS_build.sh armv7s (obselete)"
    echo "  iOS_build.sh lipo"
    echo "  iOS_build.sh all"
    echo "  iOS_build.sh clean"
    echo "  iOS_build.sh check"
    exit 1
fi
