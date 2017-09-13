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
    
#define DEFAULT_VIDEO_CODEC_ID        (AV_CODEC_ID_H264)
#define DEFAULT_MAX_FRAME_QUEUE_SIZE  (8)

CSVStream::CSVStream(){
    mImageParam = nullptr;
    mCodecID = DEFAULT_VIDEO_CODEC_ID;
    mQueueFrameNums = DEFAULT_MAX_FRAME_QUEUE_SIZE;
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

int CSVStream::stop() {
    if (mCodecCtx)
        avcodec_close(mCodecCtx);

    return HB_OK;
}

int CSVStream::release() {
    if (mCodecCtx)
        avcodec_free_context(&mCodecCtx);
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

int CSVStream::setFrameBufferNum(int num) {
    if (num <= 0) {
        return HB_ERROR;
    }
    mQueueFrameNums = num;
    return HB_OK;
}

}
