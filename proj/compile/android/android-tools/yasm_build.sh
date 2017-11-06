#!/bin/sh

export YASM_INSTALL_DIR=$UNI_BUILD_ROOT/_install_yasm

# 判断是否已经安装
if [ ! -d $YASM_INSTALL_DIR ]; then

    # 解压源码
    if [ ! -d $UNI_BUILD_ROOT/yasm ]; then
    tar xvf sources/yasm-1.3.0.tar.gz -C $UNI_BUILD_ROOT
    ln -s yasm-1.3.0 $UNI_BUILD_ROOT/yasm
    fi

    # 进入源码目录
    pushd $UNI_BUILD_ROOT/yasm

    ./configure

    make -j8
    make install DESTDIR=$YASM_INSTALL_DIR

    # 离开源码目录
    popd

fi

export PATH=$YASM_INSTALL_DIR/usr/local/bin:$PATH
export DYLD_LIBRARY_PATH=$YASM_INSTALL_DIR/usr/local/lib:$DYLD_LIBRARY_PATH
