#!/bin/sh

PROJ_ROOT_PATH=$(pwd)/
X264_LIB_OUTPUT_PATH=${PROJ_ROOT_PATH}/../../lib/x264

cd ${PROJ_ROOT_PATH}

./configure --prefix=${X264_LIB_OUTPUT_PATH} --enable-asm --enable-static --enable-debug --enable-pic && make && make install 



