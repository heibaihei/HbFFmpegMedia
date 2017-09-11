//
//  CSVStream.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/11.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSVStream.h"
#include "CSUtil.h"

namespace HBMedia {
    
CSVStream::CSVStream(){
    mImageParam = nullptr;
    mCodec = nullptr;
}

CSVStream::~CSVStream(){
    
}

void CSVStream::EchoStreamInfo() {
}

int CSVStream::sendRawData(uint8_t* pData, long DataSize, int64_t TimeStamp) {
    return HB_OK;
}

int CSVStream::bindOpaque(void *handle) {
    return HB_OK;
}

int CSVStream::setEncoder(const char *CodecName) {
    mCodec = avcodec_find_encoder_by_name(CodecName);
    if (mCodec == NULL) {
        LOGE("CSVStream find encoder failed !");
        return HB_ERROR;
    }
    
    return HB_OK;
}

void CSVStream::setVideoParam(ImageParams* param) {
    mImageParam = param;
}

}
