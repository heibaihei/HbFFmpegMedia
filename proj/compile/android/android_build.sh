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

# This script is based on projects below
# https://github.com/yixia/FFmpeg-Android
# http://git.videolan.org/?p=vlc-ports/android.git;a=summary

#----------
export BASE_PATH=`pwd`
export UNI_BUILD_ROOT=$BASE_PATH/android-build
export OUTPUT_PATH=$BASE_PATH/android-output

FF_TARGET=$1
set -e
set +x

FF_ACT_ARCHS_32="armv5 armv7a x86"
FF_ACT_ARCHS_64="armv5 armv7a arm64 x86 x86_64"
FF_ACT_ARCHS_ALL=$FF_ACT_ARCHS_64

echo_archs() {
    echo "===================="
    echo "[*] check archs"
    echo "===================="
    echo "FF_ALL_ARCHS = $FF_ACT_ARCHS_ALL"
    echo "FF_ACT_ARCHS = $*"
    echo ""
}

echo_usage() {
    echo "Usage:"
    echo "  android_build.sh armv5|armv7a|arm64|x86|x86_64"
    echo "  android_build.sh all|all32"
    echo "  android_build.sh all64"
    echo "  android_build.sh clean"
    echo "  android_build.sh check"
    exit 1
}

echo_nextstep_help() {
    echo ""
    echo "--------------------"
    echo "[*] Finished"
    echo "--------------------"
    echo "# to continue to build ffmpeg, run script below,"
    echo "sh android_build.sh "
}

# 解压源码
if [ "$TARGET" != "clean" ]; then
    mkdir -p $UNI_BUILD_ROOT
    if [ ! -d $UNI_BUILD_ROOT/ffmpeg ]; then
#        tar xvf sources/ffmpeg-2.8.4.tar.bz2 -C $UNI_BUILD_ROOT
        cp -rf sources/ffmpeg $UNI_BUILD_ROOT
#        ln -s ffmpeg-3.1.2 $UNI_BUILD_ROOT/ffmpeg
    fi

    if [ ! -d $UNI_BUILD_ROOT/x264 ]; then
        tar xvf ./sources/x264.tar.gz -C $UNI_BUILD_ROOT
        ln -s x264 $UNI_BUILD_ROOT/x264
    fi

    if [ ! -d $UNI_BUILD_ROOT/lame ]; then
        tar xvf ./sources/lame-3.99.5.tar.gz -C $UNI_BUILD_ROOT
        ln -s lame-3.99.5 $UNI_BUILD_ROOT/lame
    fi

    if [ ! -d $UNI_BUILD_ROOT/fdk-aac ]; then
        tar xvf ./sources/fdk-aac.tar.gz -C $UNI_BUILD_ROOT
    fi
fi

build_archs() {
    ARCH=$1
    sh android-tools/x264_build.sh $ARCH
    sh android-tools/lame_build.sh $ARCH
    sh android-tools/fdk-aac_build.sh $ARCH
    sh android-tools/do-compile-ffmpeg.sh $ARCH
    OUTPUT_PATH=$BASE_PATH/android-output/$ARCH


    rm -rf $OUTPUT_PATH
    mkdir -p $OUTPUT_PATH

    cp -r android-build/ffmpeg/$ARCH/include $OUTPUT_PATH
    cp -r android-build/ffmpeg/$ARCH/libffmpeg.so $OUTPUT_PATH

}

#----------
case "$FF_TARGET" in
    "")
        echo_archs armv7a
        build_archs armv7a
    ;;
    armv5|armv7a|arm64|x86|x86_64)
        echo_archs $FF_TARGET
        build_archs $FF_TARGET
        #echo_nextstep_help
    ;;
    all32)
        echo_archs $FF_ACT_ARCHS_32
        for ARCH in $FF_ACT_ARCHS_32
        do
            build_archs $ARCH
        done
        #echo_nextstep_help
    ;;
    all|all64)
        echo_archs $FF_ACT_ARCHS_64
        for ARCH in $FF_ACT_ARCHS_64
        do
            build_archs $ARCH
        done
        #echo_nextstep_help
    ;;
    clean)
        echo_archs FF_ACT_ARCHS_64
        rm -rf android-build
    ;;
    check)
        echo_archs FF_ACT_ARCHS_ALL
    ;;
    *)
        echo_usage
        exit 1
    ;;
esac
