//
//  CSAStream.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/11.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSAStream.h"

namespace HBMedia {
    
CSAStream::CSAStream(){
    mAudioParam = nullptr;
}

CSAStream::~CSAStream(){
    
}

int CSAStream::bindOpaque(void *handle) {
    return HB_OK;
}

int CSAStream::sendRawData(uint8_t* pData, long DataSize, int64_t TimeStamp) {
    return HB_OK;
}

void CSAStream::setAudioParam(AudioParams* param) {
    mAudioParam = param;
}

int CSAStream::setEncoder(const char *CodecName) {
    mCodec = avcodec_find_encoder_by_name(CodecName);
    if (mCodec == NULL) {
        LOGE("CSVStream find encoder failed !");
        return HB_ERROR;
    }
    
    return HB_OK;
}

int CSAStream::stop() {
    return HB_OK;
}

int CSAStream::release() {
    return HB_OK;
}

void CSAStream::EchoStreamInfo() {

}

}
