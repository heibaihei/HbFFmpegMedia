//
//  CSIStream.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/11.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSIStream.h"

namespace HBMedia {

CSIStream::CSIStream(){
    mStreamThreadParam = nullptr;
    mFmtCtx = nullptr;
    mStream = nullptr;
    mCodec = nullptr;
    mCodecCtx = nullptr;
    mSrcFrame = nullptr;
    mPushDataWithSyncMode = true;
    mStreamType = CS_STREAM_TYPE_NONE;
    mStreamIndex = -1;
    mSpeed = 0.1f;
}

CSIStream::~CSIStream(){
    
}

int CSIStream::setEncoder(const char *CodecName) {
    mCodec = avcodec_find_encoder_by_name(CodecName);
    if (mCodec == NULL) {
        LOGE("Stream find encoder failed !");
        return HB_ERROR;
    }
    
    return HB_OK;
}

int CSIStream::setEncoder(const AVCodecID CodecID) {
    mCodec = avcodec_find_encoder(CodecID);
    if (mCodec == NULL) {
        LOGE("Stream find encoder failed !");
        return HB_ERROR;
    }
    
    return HB_OK;
}

}
