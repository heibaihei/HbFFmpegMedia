#!/bin/sh

CUR_WORK_PATH=$(pwd)
JPEG_PROJ_ROOT_PATH=${CUR_WORK_PATH}
JPEG_OUTPUT_PATH=${JPEG_PROJ_ROOT_PATH}/../../lib/libjpeg

cd ${JPEG_PROJ_ROOT_PATH} \
&& ./configure --prefix=${JPEG_OUTPUT_PATH} --enable-static --disable-shared \
&& make \
&& make install 



