#!/bin/sh

PROJ_ROOT_PATH=$(pwd)/
LAME_OUTPUT_PATH=${PROJ_ROOT_PATH}/../../lib/lame

cd ${PROJ_ROOT_PATH}

./configure --prefix=${LAME_OUTPUT_PATH} --enable-static --disable-shared --with-pic && make && make install 
