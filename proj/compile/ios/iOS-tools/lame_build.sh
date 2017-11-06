#! /usr/bin/env bash

FF_ALL_ARCHS_IOS6_SDK="armv7 armv7s "
FF_ALL_ARCHS_IOS7_SDK="armv7 armv7s arm64  x86_64"
FF_ALL_ARCHS_IOS8_SDK="armv7 arm64  x86_64"

FF_ALL_ARCHS=$FF_ALL_ARCHS_IOS8_SDK

#----------
UNI_BUILD_ROOT=`pwd`
UNI_TMP="$UNI_BUILD_ROOT/tmp"
UNI_TMP_LLVM_VER_FILE="$UNI_TMP/llvm.ver.txt"
FF_TARGET=$1
set -e

#----------
FF_LIBS="libmp3lame"

#----------
echo_archs() {
    echo "===================="
    echo "[*] check xcode version"
    echo "===================="
    echo "FF_ALL_ARCHS = $FF_ALL_ARCHS"
}

do_lipo () {
    LIB_FILE=$1
    LIPO_FLAGS=
    for ARCH in $FF_ALL_ARCHS
    do
        LIPO_FLAGS="$LIPO_FLAGS $UNI_BUILD_ROOT/iOS-build/lame-$ARCH/output/lib/$LIB_FILE"
    done

    xcrun lipo -create $LIPO_FLAGS -output $UNI_BUILD_ROOT/iOS-build/universal/lib/$LIB_FILE
    xcrun lipo -info $UNI_BUILD_ROOT/iOS-build/universal/lib/$LIB_FILE
}

do_lipo_all () {
    mkdir -p $UNI_BUILD_ROOT/iOS-build/universal/lib
    echo "lipo archs: $FF_ALL_ARCHS"
    for FF_LIB in $FF_LIBS
    do
        do_lipo "$FF_LIB.a";
    done

    cp -R $UNI_BUILD_ROOT/iOS-build/openssl-armv7/output/include $UNI_BUILD_ROOT/iOS-build/universal/
}


do_compile(){

    echo "===================="
    echo "[*] check host"
    echo "===================="
    set -e

    FF_XCRUN_DEVELOPER=`xcode-select -print-path`
    if [ ! -d "$FF_XCRUN_DEVELOPER" ]; then
        echo "xcode path is not set correctly $FF_XCRUN_DEVELOPER does not exist (most likely because of xcode > 4.3)"
        echo "run"
        echo "sudo xcode-select -switch <xcode path>"
        echo "for default installation:"
        echo "sudo xcode-select -switch /Applications/Xcode.app/Contents/Developer"
        exit 1
    fi

    case $FF_XCRUN_DEVELOPER in
        *\ * )
            echo "Your Xcode path contains whitespaces, which is not supported."
            exit 1
            ;;
    esac

    #--------------------
    # include

    #--------------------
    # common defines
    FF_ARCH=$1
    if [ -z "$FF_ARCH" ]; then
        echo "You must specific an architecture 'armv7, armv7s, arm64, i386, x86_64, ...'.\n"
        exit 1
    fi

    FF_BUILD_ROOT=`pwd`
    FF_TAGET_OS="darwin"

    # lame build params
    export COMMON_FF_CFG_FLAGS=

    EXT_CFG_FLAGS=
    EXT_EXTRA_CFLAGS=
    EXT_CFG_CPU=

    # i386, x86_64
    EXT_CFG_FLAGS_SIMULATOR=

    # armv7, armv7s, arm64
    EXT_CFG_FLAGS_ARM=
    EXT_CFG_FLAGS_ARM="iphoneos-cross"

    echo "build_root: $FF_BUILD_ROOT"

    #--------------------
    echo "===================="
    echo "[*] config arch $FF_ARCH"
    echo "===================="

    FF_BUILD_NAME="unknown"
    FF_XCRUN_PLATFORM="iPhoneOS"
    FF_XCRUN_OSVERSION=
    FF_GASPP_EXPORT=
    FF_XCODE_BITCODE=

    if [ "$FF_ARCH" = "i386" ]; then
        FF_BUILD_NAME="lame-i386"
        FF_XCRUN_PLATFORM="iPhoneSimulator"
        FF_XCRUN_OSVERSION="-mios-simulator-version-min=6.0"
        EXT_CFG_FLAGS="darwin-i386-cc $EXT_CFG_FLAGS"
    elif [ "$FF_ARCH" = "x86_64" ]; then
        FF_BUILD_NAME="lame-x86_64"
        FF_XCRUN_PLATFORM="iPhoneSimulator"
        FF_XCRUN_OSVERSION="-mios-simulator-version-min=7.0"
        EXT_CFG_FLAGS="darwin64-x86_64-cc $EXT_CFG_FLAGS"
    elif [ "$FF_ARCH" = "armv7" ]; then
        FF_BUILD_NAME="lame-armv7"
        FF_XCRUN_OSVERSION="-miphoneos-version-min=6.0"
        FF_XCODE_BITCODE="-fembed-bitcode"
        EXT_CFG_FLAGS="$EXT_CFG_FLAGS_ARM $EXT_CFG_FLAGS"
        HOST="arm-apple-darwin"
        TARGET="arm-apple-darwin"
        echo "EXT_CFG_FLAGS : $EXT_CFG_FLAGS"
        #    EXT_CFG_CPU="--cpu=cortex-a8"
    elif [ "$FF_ARCH" = "armv7s" ]; then
        FF_BUILD_NAME="lame-armv7s"
        EXT_CFG_CPU="--cpu=swift"
        FF_XCRUN_OSVERSION="-miphoneos-version-min=6.0"
        FF_XCODE_BITCODE="-fembed-bitcode"
        EXT_CFG_FLAGS="$EXT_CFG_FLAGS_ARM $EXT_CFG_FLAGS"
    elif [ "$FF_ARCH" = "arm64" ]; then
        FF_BUILD_NAME="lame-arm64"
        FF_XCRUN_OSVERSION="-miphoneos-version-min=7.0"
        FF_XCODE_BITCODE="-fembed-bitcode"
        EXT_CFG_FLAGS="$EXT_CFG_FLAGS_ARM $EXT_CFG_FLAGS"
        FF_GASPP_EXPORT="GASPP_FIX_XCODE5=1"
    else
        echo "unknown architecture $FF_ARCH";
        exit 1
    fi

    echo "build_name: $FF_BUILD_NAME"
    echo "platform:   $FF_XCRUN_PLATFORM"
    echo "osversion:  $FF_XCRUN_OSVERSION"

    #--------------------
    echo "===================="
    echo "[*] make ios toolchain $FF_BUILD_NAME"
    echo "===================="

    FF_BUILD_SOURCE="$FF_BUILD_ROOT/iOS-build/lame"
    FF_BUILD_PREFIX="$FF_BUILD_ROOT/iOS-build/$FF_BUILD_NAME/output"

    mkdir -p $FF_BUILD_PREFIX

    FF_XCRUN_SDK=`echo $FF_XCRUN_PLATFORM | tr '[:upper:]' '[:lower:]'`
    FF_XCRUN_SDK_PLATFORM_PATH=`xcrun -sdk $FF_XCRUN_SDK --show-sdk-platform-path`
    FF_XCRUN_SDK_PATH=`xcrun -sdk $FF_XCRUN_SDK --show-sdk-path`
    FF_XCRUN_CC="xcrun -sdk $FF_XCRUN_SDK clang"

    export CROSS_TOP="$FF_XCRUN_SDK_PLATFORM_PATH/Developer"
    export CROSS_SDK=`echo ${FF_XCRUN_SDK_PATH/#$CROSS_TOP\/SDKs\//}`
    export BUILD_TOOL="$FF_XCRUN_DEVELOPER"
    export CC="$FF_XCRUN_CC -arch $FF_ARCH $FF_XCRUN_OSVERSION"

    echo "build_source: $FF_BUILD_SOURCE"
    echo "build_prefix: $FF_BUILD_PREFIX"
    echo "CROSS_TOP: $CROSS_TOP"
    echo "CROSS_SDK: $CROSS_SDK"
    echo "BUILD_TOOL: $BUILD_TOOL"
    echo "CC: $CC"

    #--------------------
    echo "\n--------------------"
    echo "[*] configurate lame"
    echo "--------------------"

    EXT_CFG_FLAGS="$EXT_CFG_FLAGS --prefix=$FF_BUILD_PREFIX"
    EXT_CFG_FLAGS="$EXT_CFG_FLAGS --host=$HOST --target=$TARGET"
    EXT_CFG_FLAGS="$EXT_CFG_FLAGS --with-pic"
    EXT_CFG_FLAGS="$EXT_CFG_FLAGS --enable-static"
    EXT_CFG_FLAGS="$EXT_CFG_FLAGS --disable-shared"
    EXT_CFG_FLAGS="$EXT_CFG_FLAGS --with-gnu-ld"
    EXT_CFG_FLAGS="$EXT_CFG_FLAGS --enable-nasm"

    # xcode configuration
    export DEBUG_INFORMATION_FORMAT=dwarf-with-dsym

    cd $FF_BUILD_SOURCE
    echo "config: $EXT_CFG_FLAGS"
    ./Configure \
    $EXT_CFG_FLAGS
    make clean

    #--------------------
    echo "--------------------"
    echo "[*] compile lame"
    echo "--------------------"
    set +e
    make
    make install
}

#----------
if [ "$FF_TARGET" = "armv7" -o "$FF_TARGET" = "armv7s" -o "$FF_TARGET" = "arm64" ]; then
    echo_archs
    do_compile $FF_TARGET
elif [ "$FF_TARGET" = "i386" -o "$FF_TARGET" = "x86_64" ]; then
    echo_archs
    do_compile $FF_TARGET
elif [ "$FF_TARGET" = "lipo" ]; then
    echo_archs
    do_lipo_all
elif [ "$FF_TARGET" = "all" ]; then
    echo_archs
    for ARCH in $FF_ALL_ARCHS
    do
        do_compile $ARCH
    done

    do_lipo_all
elif [ "$FF_TARGET" = "check" ]; then
    echo_archs
elif [ "$FF_TARGET" = "clean" ]; then
    echo_archs
    for ARCH in $FF_ALL_ARCHS
    do
        cd lame-$ARCH && git clean -xdf && cd -
    done
else
    echo "Usage:"
    echo "  lame_build.sh armv7|arm64|i386|x86_64"
    echo "  lame_build.sh armv7s (obselete)"
    echo "  lame_build.sh lipo"
    echo "  lame_build.sh all"
    echo "  lame_build.sh clean"
    echo "  lame_build.sh check"
    exit 1
fi
