#!/bin/sh

PROJ_ROOT_PATH=$(pwd)/
FAAC_LIB_OUTPUT_PATH=${PROJ_ROOT_PATH}/../../lib/fdk-aac

cd ${PROJ_ROOT_PATH}

./autogen.sh && ./configure --prefix=${FAAC_LIB_OUTPUT_PATH} --enable-static --disable-shared --with-pic && make && make install 



