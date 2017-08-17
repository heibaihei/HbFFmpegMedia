#!/bin/sh

PROJ_ROOT_PATH=$(pwd)/
JPEG_LIB_OUTPUT_PATH=${PROJ_ROOT_PATH}/../../lib/openjpeg

cd ${PROJ_ROOT_PATH}

./configure --prefix=${JPEG_LIB_OUTPUT_PATH} --enable-static --disable-shared --enable-debug --with-pic && make && make install 



