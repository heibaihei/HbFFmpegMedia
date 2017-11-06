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

# try to detect NDK version
NDK_REL=$(grep -o '^r[0-9]*.*' $ANDROID_NDK/RELEASE.TXT 2>/dev/null|cut -b2-)
case "$NDK_REL" in
    14*|10*)
        # we don't use 4.4.3 because it doesn't handle threads correctly.
        if test -d ${ANDROID_NDK}/toolchains/arm-linux-androideabi-4.9
        # if gcc 4.8 is present, it's there for all the archs (x86, mips, arm)
        then
            echo "NDKr$NDK_REL detected"
        else
            echo "You need the NDKr9 or later"
            exit 1
        fi
    ;;
    *)
        echo "You need the NDKr9 or later"
        exit 1
    ;;
esac

BUILD_ROOT=`pwd`/android-toolchain
ANDROID_PLATFORM=android-9
GCC_VER=4.8
GCC_64_VER=4.9

#--------------------
echo ""
echo "--------------------"
echo "[*] make NDK standalone toolchain"
echo "--------------------"
MAKE_TOOLCHAIN_FLAGS=
case "$UNAME_S" in
    Darwin)
        MAKE_TOOLCHAIN_FLAGS="$MAKE_TOOLCHAIN_FLAGS --system=darwin-x86_64"
    ;;
    CYGWIN_NT-*)
        MAKE_TOOLCHAIN_FLAGS="$MAKE_TOOLCHAIN_FLAGS --system=windows-x86_64"

        WIN_TEMP="$(cygpath -am /tmp)"
        export TEMPDIR=$WIN_TEMP/

        echo "Cygwin temp prefix=$WIN_TEMP/"
        #CFG_FLAGS="$CFG_FLAGS --tempprefix=$WIN_TEMP/"
    ;;
esac

#----- armv7a begin -----
if [ "$ARCH" = "armv7a" ]; then
    CROSS_PREFIX=arm-linux-androideabi
    TOOLCHAIN_NAME=${CROSS_PREFIX}-${GCC_VER}

elif [ "$ARCH" = "armv5" ]; then
    CROSS_PREFIX=arm-linux-androideabi
    TOOLCHAIN_NAME=${CROSS_PREFIX}-${GCC_VER}

elif [ "$ARCH" = "x86" ]; then
    CROSS_PREFIX=i686-linux-android
    TOOLCHAIN_NAME=x86-${GCC_VER}

elif [ "$ARCH" = "x86_64" ]; then
   ANDROID_PLATFORM=android-21
   CROSS_PREFIX=x86_64-linux-android
   TOOLCHAIN_NAME=${CROSS_PREFIX}-${GCC_64_VER}

elif [ "$ARCH" = "arm64" ]; then
   ANDROID_PLATFORM=android-21
   CROSS_PREFIX=aarch64-linux-android
   TOOLCHAIN_NAME=${CROSS_PREFIX}-${GCC_64_VER}

else
    echo "unknown architecture $ARCH";
    exit 1
fi

TOOLCHAIN_PATH=$BUILD_ROOT/$ARCH
MAKE_TOOLCHAIN_FLAGS="$MAKE_TOOLCHAIN_FLAGS --install-dir=$TOOLCHAIN_PATH"
mkdir -p $TOOLCHAIN_PATH

TOOLCHAIN_TOUCH="$TOOLCHAIN_PATH/touch"
if [ ! -f "$TOOLCHAIN_TOUCH" ]; then
    $ANDROID_NDK/build/tools/make-standalone-toolchain.sh \
        $MAKE_TOOLCHAIN_FLAGS \
        --platform=$ANDROID_PLATFORM \
        --toolchain=$TOOLCHAIN_NAME
    touch $TOOLCHAIN_TOUCH;
fi

