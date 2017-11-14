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
    ThreadIPCContext *pEncodeIpcCtx = mStreamThreadParam->mEncodeIPC;
    ThreadIPCContext *pQueueIpcCtx = mStreamThreadParam->mQueueIPC;
    int iQueueleftLen = frameQueue->queueLeft();
    
    if (iQueueleftLen == 0) {
        frameQueue->setQueueStat(QUEUE_OVERFLOW);
        if (mPushDataWithSyncMode) {
            while (frameQueue->queueLeft() == 0) {
                pQueueIpcCtx->condV();
            }
        } else {
            LOGF("Video frame be drop !");
            return HB_ERROR;
        }
    }
    
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
    
    char crf[4], rotate[4];
    int HBErr = HB_OK;
    AVDictionary *opts = NULL;
    mFmtCtx = (AVFormatContext *)handle;
    mStreamThreadParam = (StreamThreadParam *)av_mallocz(sizeof(StreamThreadParam));
    if (!mStreamThreadParam) {
        LOGE("Video stream create stream pthread params failed !");
        goto BIND_VIDEO_STREAM_END_LABEL;
    }
    
    HBErr = initialStreamThreadParams(mStreamThreadParam);
    if (HBErr != HB_OK) {
        LOGE("Initial stream thread params failed !");
        goto BIND_VIDEO_STREAM_END_LABEL;
    }
    
    if (!mCodec) {
        mCodec = avcodec_find_encoder_by_name("libx264");
        if (!mCodec) {
            LOGE("Video stream find encoder by name failed !");
            goto BIND_VIDEO_STREAM_END_LABEL;
        }
    }
    
    mStream = avformat_new_stream(mFmtCtx, mCodec);
    if (!mStream) {
        LOGE("Video stream create new stream failed !");
        goto BIND_VIDEO_STREAM_END_LABEL;
    }
    
    /** 默认视频流时间基采用 {1, 90000} */
    mStream->time_base.num = 1;
    mStream->time_base.den = 90000;
    
    mStreamThreadParam->mStreamIndex = mStream->index;
    LOGI("Video stream create success, index:%d", mStream->index);
    
    mCodecCtx = avcodec_alloc_context3(mCodec);
    if (!mCodecCtx) {
        LOGE("Video codec context alloc failed !");
        goto BIND_VIDEO_STREAM_END_LABEL;
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
    
    
    av_dict_set(&opts, "profile", "baseline", 0);
    
    if (mCodecCtx->codec_id == AV_CODEC_ID_H264) {
        av_opt_set(mCodecCtx->priv_data, "level", "4.1", 0);
        av_opt_set(mCodecCtx->priv_data, "preset", "superfast", 0);
        av_opt_set(mCodecCtx->priv_data, "tune", "zerolatency", 0);
    }
    av_dict_set(&opts, "threads", "auto", 0);
    HBErr = avcodec_open2(mCodecCtx, mCodec, &opts);
    if (HBErr < 0) {
        LOGE("Video stream codec open failed !<%s>", makeErrorStr(HBErr));
        goto BIND_VIDEO_STREAM_END_LABEL;
    }
    av_dict_free(&opts);
    
    HBErr = avcodec_parameters_from_context(mStream->codecpar, mCodecCtx);
    if (HBErr < 0) {
        LOGE("Video stream copy context paramter error !");
        goto BIND_VIDEO_STREAM_END_LABEL;
    }
    
    if (mImageParam->mRotate > 0) {
        HBErr = snprintf(rotate, sizeof(rotate), "%d", mImageParam->mRotate);
        if (HBErr < 0) {
            LOGE("Copy string rotate error!\n");
        }
        HBErr = av_dict_set(&mStream->metadata, "rotate", rotate, 0);
        if (HBErr < 0) {
            LOGE("Set video rotate error!\n");
        }
    }
    
    mStreamThreadParam->mCodecCtx = mCodecCtx;
    mStreamThreadParam->mTimeBase = mStream->time_base;
    mSrcFrame = av_frame_alloc();
    if (!mSrcFrame) {
        LOGE("Create buffer source frame failed !");
        goto BIND_VIDEO_STREAM_END_LABEL;
    }
    
    return HB_OK;

BIND_VIDEO_STREAM_END_LABEL:
    if (mStreamThreadParam) {
        releaseStreamThreadParams(mStreamThreadParam);
        mStreamThreadParam = nullptr;
    }
    if (mCodecCtx) {
        avcodec_free_context(&mCodecCtx);
        mCodecCtx = nullptr;
    }
    mCodec = nullptr;
    mStream = nullptr;
    if (!mSrcFrame)
        av_frame_free(&mSrcFrame);
    return HB_ERROR;
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
