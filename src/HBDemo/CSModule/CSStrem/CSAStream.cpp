//
//  CSAStream.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/11.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSAStream.h"
#include "CSAudioFifo.h"

namespace HBMedia {
    
CSAStream::CSAStream(){
    mAudioParam = nullptr;
    mInTotalOfSamples = 0;
    mOutTotalOfSamples = 0;
    mOutTotalOfFrame = 0;
}

CSAStream::~CSAStream(){
    
}

int CSAStream::bindOpaque(void *handle) {
    if (!handle) {
        LOGE("Audio stream bing opaque failed !");
        return HB_ERROR;
    }
    
    mFmtCtx = (AVFormatContext *)handle;
    mStreamThreadParam = (StreamThreadParam *)av_mallocz(sizeof(StreamThreadParam));
    if (!mStreamThreadParam) {
        LOGE("Audio stream create stream pthread params failed !");
        return HB_ERROR;
    }
    int HBErr = initialStreamThreadParams(mStreamThreadParam);
    if (HBErr != HB_OK) {
        LOGE("Initial stream thread params failed !");
        return HB_ERROR;
    }
    
    if (!mCodec) {
        mCodec = avcodec_find_encoder_by_name("libfdk_aac");
        if (!mCodec) {
            LOGE("Audio stream find encoder by name failed !");
            return HB_ERROR;
        }
    }
    
    mStream = avformat_new_stream(mFmtCtx, mCodec);
    if (!mStream) {
        LOGE("Video stream create new stream failed !");
        return HB_ERROR;
    }
    
    mStream->time_base.num = 1;
    mStream->time_base.den = mAudioParam->sample_rate;
    mStreamThreadParam->mStreamIndex = mStream->index;
    LOGI("Audio stream create success, index:%d", mStream->index);
    
    mCodecCtx = avcodec_alloc_context3(mCodec);
    if (!mCodecCtx) {
        LOGE("Audio codec context alloc failed !");
        return HB_ERROR;
    }
    
    mCodecCtx->channels = mAudioParam->channels;
    mCodecCtx->bit_rate = mAudioParam->mbitRate;
    mCodecCtx->sample_fmt = mAudioParam->sample_fmt;
    mCodecCtx->channel_layout = av_get_default_channel_layout(mAudioParam->channels);
    mCodecCtx->sample_rate = mAudioParam->sample_rate;
    
    if (mFmtCtx->oformat->flags &AVFMT_GLOBALHEADER) {
        mCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
    HBErr = avcodec_open2(mCodecCtx, mCodec, NULL);
    if (!HBErr) {
        LOGE("Audio codec context alloc failed !");
        return HB_ERROR;
    }
    
    HBErr = avcodec_parameters_from_context(mStream->codecpar, mCodecCtx);
    if (HBErr < 0) {
        LOGE("Audio stream copy context paramter error !");
        return HB_ERROR;
    }
    
    mStreamThreadParam->mCodecCtx = mCodecCtx;
    mStreamThreadParam->mTimeBase = mStream->time_base;
    
    mSrcFrame = av_frame_alloc();
    if (!mSrcFrame) {
        LOGE("Aduio stream alloc audio source frame failed !");
        return HB_ERROR;
    }
    
    HBErr = initialAudioFrameWidthParams(&mSrcFrame, mAudioParam, mCodecCtx->frame_size);
    if (HBErr != HB_OK) {
        LOGE("Audio stream source frame initial failed !");
        return HB_ERROR;
    }
    mInTotalOfSamples = 0;
    mOutTotalOfSamples = 0;
    mOutTotalOfFrame = 0;
    
    return HB_OK;
}

int CSAStream::sendRawData(uint8_t* pData, long DataSize, int64_t TimeStamp) {
    FiFoQueue<AVFrame*> *frameQueue = mStreamThreadParam->mFrameQueue;
    if (frameQueue->queueLeft() == 0) {
        LOGE("Audio stream frame queue overflow !");
        return HB_ERROR;
    }
    
    ThreadIPCContext *pEncodeThreadIpcCtx = mStreamThreadParam->mEncodeIPC;
    int iInputNumOfSamples = (int)(DataSize / av_get_bytes_per_sample(mAudioParam->sample_fmt));
    mInTotalOfSamples += iInputNumOfSamples;
    
    uint8_t* pTmpData = pData;
    uint8_t *pInputData[8] = {NULL};
    int iLineSize[8] = {0, 0};
    uint8_t *pOutputData[8] = {NULL};
    int iOutLineSize[8] = {0};
    
    int HBErr = av_samples_fill_arrays(pInputData, iLineSize, pTmpData, \
                    mAudioParam->channels, iInputNumOfSamples, mAudioParam->sample_fmt, mAudioParam->mAlign);
    if (HBErr < 0) {
        LOGE("Audio stream fille sample arrays failed !");
        return HB_ERROR;
    }
    
    /** 是否需要插入重采样的处理 */
    int actualAudioSamples = iInputNumOfSamples;
    memcpy(pOutputData, pInputData, sizeof(pOutputData));
    
    HBErr = pushSamplesToFifo(mAudioFifo, pOutputData, actualAudioSamples);
    if (HBErr < 0) {
        LOGE("Audio stream push sample into fifo failed !");
        return HB_ERROR;
    }
    
    
    int iChannels = mAudioParam->channels;
    AVSampleFormat eSampleFmt = mAudioParam->sample_fmt;
    int iSampleRate = mAudioParam->sample_rate;
    
    AVFrame* pBufferFrame = nullptr;
    FiFoQueue<AVFrame *> *pFrameRecycleQueue = mStreamThreadParam->mFrameRecycleQueue;
    while (true) {
        int iFifoSize = av_audio_fifo_size(mAudioFifo);
        if (iFifoSize < mAudioParam->frame_size) {
            break;
        }
        
        actualAudioSamples = FFMIN(iFifoSize, mAudioParam->frame_size);
        int iBufferSize = av_samples_get_buffer_size(NULL, \
                                                     iChannels, actualAudioSamples, eSampleFmt, mAudioParam->mAlign);
        if (!pFrameRecycleQueue || !(pBufferFrame = pFrameRecycleQueue->get())) {
            pBufferFrame = av_frame_alloc();
            if (!pBufferFrame) {
                LOGE("Audio stream alloc frame failed !");
                return HB_ERROR;
            }
            
            uint8_t* pOutDataBuffer = (uint8_t*)av_mallocz(iBufferSize);
            if (!pOutDataBuffer) {
                LOGE("Audio stream allock audio data failed !");
                return HB_ERROR;
            }
            
            HBErr = av_samples_fill_arrays(pBufferFrame->data, pBufferFrame->linesize, pOutDataBuffer, iChannels, actualAudioSamples, eSampleFmt, mAudioParam->mAlign);
            if (HBErr < 0) {
                LOGE("Audio stream fill sample fata error !");
                return HB_ERROR;
            }
            pBufferFrame->nb_samples = actualAudioSamples;
            pBufferFrame->opaque = pOutDataBuffer;
        }
        
        HBErr = av_audio_fifo_read(mAudioFifo, (void**)pBufferFrame->data, actualAudioSamples);
        if (HBErr <= actualAudioSamples) {
            return HB_ERROR;
        }
        mOutTotalOfSamples += HBErr;
        pBufferFrame->pts = actualAudioSamples * mOutTotalOfFrame * mAudioParam->sample_rate;
        mOutTotalOfFrame++;
        
        frameQueue->push(pBufferFrame);
        
        pEncodeThreadIpcCtx->condP();
    }
    
    return HB_OK;
}

void CSAStream::setAudioParam(AudioParams* param) {
    mAudioParam = param;
    int HBErr = initialFifo(&mAudioFifo, mAudioParam->sample_fmt, mAudioParam->channels, 1);
    if (HBErr < 0)
        LOGE("Initial fifo error !");
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
