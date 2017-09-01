#!/bin/sh

PROJ_ROOT_PATH=$(pwd)/

LIBYUV_LIB_OUTPUT_PATH=${PROJ_ROOT_PATH}/../../lib/libyuv

cd  ${LIBYUV_LIB_OUTPUT_PATH} \
&& cmake ${PROJ_ROOT_PATH} \
&& make