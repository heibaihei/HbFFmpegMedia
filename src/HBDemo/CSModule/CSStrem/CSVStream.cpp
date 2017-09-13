//
//  CSVStream.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/11.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSVStream.h"
#include "CSUtil.h"
#include "CSFiFoQueue.h"
#include "CSThreadIPCContext.h"

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
    long iImagePixBufferSize = mImageParam->mDataSize;
    FiFoQueue<AVFrame*> *frameQueue = mStreamThreadParam->mFrameQueue;
    FiFoQueue<AVFrame*> *frameRecycleQueue = mStreamThreadParam->mFrameRecycleQueue;
    
    if (frameQueue->queueLeft() == 0) {
        frameQueue->setQueueStat(QUEUE_OVERFLOW);
        return HB_OK;
    }
    
    ThreadIPCContext *pIpcCtx = mStreamThreadParam->mEncodeIPC;
    
    uint8_t *pOutData = NULL;
    AVFrame* pBufferFrame = nullptr;
    if (frameRecycleQueue && (pBufferFrame = frameRecycleQueue->get())) {
        pOutData = pBufferFrame->data[0];
    }
    else {
        pBufferFrame = av_frame_alloc();
        if (!pBufferFrame) {
            LOGE("Video stream alloc valid frame buffer failed !");
            return HB_ERROR;
        }
        pOutData = (uint8_t *)av_mallocz(iImagePixBufferSize);
        if (!pOutData) {
            LOGE("Video stream alloc valid data buffer failed !");
            return HB_ERROR;
        }
        pBufferFrame->opaque = pOutData;
        int HBErr = av_image_fill_arrays(pBufferFrame->data, pBufferFrame->linesize, pOutData, getImageInnerFormat(mImageParam->mPixFmt), mImageParam->mWidth, mImageParam->mHeight, mImageParam->mAlign);
        if (HBErr < 0) {
            LOGE("Image fill arry failed !");
            return HB_ERROR;
        }
        
        pBufferFrame->format = getImageInnerFormat(mImageParam->mPixFmt);
        pBufferFrame->width = mImageParam->mWidth;
        pBufferFrame->height = mImageParam->mHeight;
    }
    
    /** TODO: huangcl: 如果需要进行转换格式，需要在这里添加格式转换 */
    memcpy(pOutData, pData, DataSize);
    pBufferFrame->pts = (1/mSpeed) * av_rescale_q(TimeStamp*1000, AV_TIME_BASE_Q, mStream->time_base);
    
    int HBErr = frameQueue->push(pBufferFrame);
    if (HBErr < 0)
        LOGE("Video stream push raw data failed !");
    
    return HBErr;
}

int CSVStream::bindOpaque(void *handle) {
    if (!handle) {
        LOGE("Video stream bind opaque failed !");
        return HB_ERROR;
    }
    mFmtCtx = (AVFormatContext *)handle;
    mStreamThreadParam = (StreamThreadParam *)av_mallocz(sizeof(StreamThreadParam));
    if (!mStreamThreadParam) {
        LOGE("Video stream create stream pthread params failed !");
        return HB_ERROR;
    }
    if (!mCodec) {
        mCodec = avcodec_find_encoder_by_name("libx264");
        if (!mCodec) {
            LOGE("Video stream find encoder by name failed !");
            return HB_ERROR;
        }
    }
    
    mStream = avformat_new_stream(mFmtCtx, mCodec);
    if (!mStream) {
        LOGE("Video stream create new stream failed !");
        return HB_ERROR;
    }
    
    mStream->time_base.num = 1;
    mStream->time_base.den = 90000;
    
    mStreamThreadParam->mStreamIndex = mStream->index;
    LOGI("Video stream create success, index:%d", mStream->index);
    
    mCodecCtx = avcodec_alloc_context3(mCodec);
    if (!mCodecCtx) {
        LOGE("Video codec context alloc failed !");
        return HB_ERROR;
    }
    mCodecCtx->width = mImageParam->mWidth;
    mCodecCtx->height = mImageParam->mHeight;
    mCodecCtx->pix_fmt = getImageInnerFormat(mImageParam->mPixFmt);
    mCodecCtx->codec_id = mCodec->id;
    mCodecCtx->gop_size = 250;
    mCodecCtx->framerate.den = 30;
    mCodecCtx->framerate.num = 1;
    mCodecCtx->time_base.den = 30;
    mCodecCtx->time_base.num = 1;
    mCodecCtx->keyint_min = 30;
    if (mImageParam->mBitRate <= 100)
        mCodecCtx->bit_rate = 3 * mCodecCtx->height * mCodecCtx->width;
    else
        mCodecCtx->bit_rate = mImageParam->mBitRate;
    if  (mFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
        mCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
    
    AVDictionary *opts = NULL;
    av_dict_set(&opts, "profile", "baseline", 0);
    
    if (mCodecCtx->codec_id == AV_CODEC_ID_H264) {
        av_opt_set(mCodecCtx->priv_data, "level", "4.1", 0);
        av_opt_set(mCodecCtx->priv_data, "preset", "superfast", 0);
        av_opt_set(mCodecCtx->priv_data, "tune", "zerolatency", 0);
    }
    av_dict_set(&opts, "threads", "auto", 0);
    int HBErr = avcodec_open2(mCodecCtx, mCodec, &opts);
    if (HBErr < 0) {
        LOGE("Video stream codec open failed !<%s>", makeErrorStr(HBErr));
        return HB_ERROR;
    }
    av_dict_free(&opts);
    
    HBErr = avcodec_parameters_from_context(mStream->codecpar, mCodecCtx);
    if (HBErr < 0) {
        LOGE("Video stream copy context paramter error !");
        return HB_ERROR;
    }
    
    mStreamThreadParam->mCodecCtx = mCodecCtx;
    mStreamThreadParam->mTimeBase = mStream->time_base;
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
