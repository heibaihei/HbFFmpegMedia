//
//  CSVideoEncoder.cpp
//  Sample
//
//  Created by zj-db0519 on 2017/12/8.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSVideoEncoder.h"

namespace HBMedia {

void* CSVideoEncoder::ThreadFunc_Video_Encoder(void *arg) {
    
    return nullptr;
}

CSVideoEncoder::CSVideoEncoder() {
    
}

CSVideoEncoder::~CSVideoEncoder() {
    
}

int CSVideoEncoder::prepare() {
    return HB_OK;
}

int CSVideoEncoder::start() {
    return HB_OK;
}

int CSVideoEncoder::stop() {
    return HB_OK;
}

int CSVideoEncoder::release() {
    return HB_OK;
}

int CSVideoEncoder::sendFrame(AVFrame **OutFrame) {
    return HB_OK;
}

int CSVideoEncoder::syncWait() {
    return HB_OK;
}

int CSVideoEncoder::_InputInitial() {
    return HB_OK;
}

int CSVideoEncoder::_SwscaleInitial() {
    return HB_OK;
}

int CSVideoEncoder::_DoSwscale(AVFrame *pInFrame, AVFrame **pOutFrame) {
    return HB_OK;
}

int CSVideoEncoder::_EncoderInitial() {
    return HB_OK;
}

}
