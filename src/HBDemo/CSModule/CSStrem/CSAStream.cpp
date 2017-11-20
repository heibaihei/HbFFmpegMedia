//
//  CSAStream.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/11.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSAStream.h"
#include "CSAudioFifo.h"
#include "CSAudioEffectFactory.h"

namespace HBMedia {
    
CSAStream::CSAStream(){
    mAudioParam = nullptr;
    mInTotalOfSamples = 0;
    mOutTotalOfSamples = 0;
    mOutTotalOfFrame = 0;
    mStreamType = CS_STREAM_TYPE_AUDIO;
}

CSAStream::~CSAStream(){
    
}

int CSAStream::bindOpaque(void *handle) {
    if (!handle) {
        LOGE("Audio stream bing opaque failed !");
        return HB_ERROR;
    }
    
    int HBErr = HB_OK;
    mOutTotalOfFrame = 0;
    mFmtCtx = (AVFormatContext *)handle;
    mStreamThreadParam = (StreamThreadParam *)av_mallocz(sizeof(StreamThreadParam));
    if (!mStreamThreadParam) {
        LOGE("Audio stream create stream pthread params failed !");
        goto BIND_AUDIO_STREAM_END_LABEL;
    }
    
    HBErr = initialStreamThreadParams(mStreamThreadParam);
    if (HBErr != HB_OK) {
        LOGE("Initial stream thread params failed !");
        goto BIND_AUDIO_STREAM_END_LABEL;
    }
    
    if (!mCodec) {
        mCodec = avcodec_find_encoder_by_name("libfdk_aac");
        if (!mCodec) {
            LOGE("Audio stream find encoder by name failed !");
            goto BIND_AUDIO_STREAM_END_LABEL;
        }
    }
    
    mStream = avformat_new_stream(mFmtCtx, mCodec);
    if (!mStream) {
        LOGE("Video stream create new stream failed !");
        goto BIND_AUDIO_STREAM_END_LABEL;
    }
    
    mStreamIndex = mStream->index;
    mStreamThreadParam->mStreamIndex = mStream->index;
    LOGI("Audio stream create success, index:%d", mStream->index);
    
    mStream->time_base.num = 1;
    mStream->time_base.den = mAudioParam->sample_rate;
    mCodecCtx = avcodec_alloc_context3(mCodec);
    if (!mCodecCtx) {
        LOGE("Audio codec context alloc failed !");
        goto BIND_AUDIO_STREAM_END_LABEL;
    }
    
    mCodecCtx->channels = mAudioParam->channels;
    mCodecCtx->channel_layout = av_get_default_channel_layout(mCodecCtx->channels);
    mCodecCtx->bit_rate = mAudioParam->mbitRate;
    mCodecCtx->sample_fmt = getAudioInnerFormat(mAudioParam->pri_sample_fmt);
    mCodecCtx->sample_rate = mAudioParam->sample_rate;
    
    if (mFmtCtx->oformat->flags &AVFMT_GLOBALHEADER) {
        mCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
    HBErr = avcodec_open2(mCodecCtx, mCodec, NULL);
    if (HBErr != 0) {
        LOGE("Audio codec context alloc failed, %s!", av_err2str(HBErr));
        goto BIND_AUDIO_STREAM_END_LABEL;
    }
    
    HBErr = avcodec_parameters_from_context(mStream->codecpar, mCodecCtx);
    if (HBErr < 0) {
        LOGE("Audio stream copy context paramter error !");
        goto BIND_AUDIO_STREAM_END_LABEL;
    }
    
    mStreamThreadParam->mTimeBase = mStream->time_base;
    mStreamThreadParam->mCodecCtx = mCodecCtx;
    
    mAudioParam->frame_size = mCodecCtx->frame_size;
    
    mSrcFrame = av_frame_alloc();
    if (!mSrcFrame) {
        LOGE("Aduio stream alloc audio source frame failed !");
        goto BIND_AUDIO_STREAM_END_LABEL;
    }
    
    HBErr = initialAudioFrameWidthParams(&mSrcFrame, mAudioParam, mCodecCtx->frame_size);
    if (HBErr != HB_OK) {
        LOGE("Audio stream source frame initial failed !");
        goto BIND_AUDIO_STREAM_END_LABEL;
    }
    mInTotalOfSamples = 0;
    mOutTotalOfSamples = 0;
    mOutTotalOfFrame = 0;

    mSrcFrame = av_frame_alloc();
    if (!mSrcFrame) {
        LOGE("Create buffer source frame failed !");
        goto BIND_AUDIO_STREAM_END_LABEL;
    }
    return HB_OK;
    
BIND_AUDIO_STREAM_END_LABEL:
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

int CSAStream::sendRawData(uint8_t* pData, long DataSize, int64_t TimeStamp) {
    FiFoQueue<AVFrame*> *pFrameQueue = mStreamThreadParam->mFrameQueue;
    FiFoQueue<AVFrame*> *pFrameRecycleQueue = mStreamThreadParam->mFrameRecycleQueue;
    ThreadIPCContext *pEncodeThreadIpcCtx = mStreamThreadParam->mEncodeIPC;
    ThreadIPCContext *pQueueThreadIpcCtx = mStreamThreadParam->mQueueIPC;

    if (pFrameQueue->queueLeft() == 0) {
        pFrameQueue->setQueueStat(QUEUE_OVERFLOW);
        if (mPushDataWithSyncMode) {
            while (pFrameQueue->queueLeft() == 0) {
                pQueueThreadIpcCtx->condV();
            }
        }
        else {
            LOGD("Queue overflow \n");
            return HB_ERROR;
        }
    }
    
    int inputSamples = (int)(DataSize / \
              (mAudioParam->channels * av_get_bytes_per_sample(getAudioInnerFormat(mAudioParam->pri_sample_fmt))));
    mInTotalOfSamples += inputSamples;
    uint8_t* pTmpData = pData;
    uint8_t *pInputData[8] = {NULL};
    int iInputDataLineSize[8] = {0, 0};
    uint8_t *pOutputData[8] = {NULL};
    int iOutputDataLineSize[8] = {0};
    
    int HBErr = av_samples_fill_arrays(pOutputData, iOutputDataLineSize, pTmpData, \
                    mAudioParam->channels, inputSamples, getAudioInnerFormat(mAudioParam->pri_sample_fmt), mAudioParam->mAlign);
    if (HBErr < 0) {
        LOGE("Audio stream fille sample arrays failed !");
        return HB_ERROR;
    }
    
    HBErr = pushSamplesToFifo(mAudioFifo, pOutputData, inputSamples);
    if (HBErr < 0) {
        LOGE("Audio stream push sample into fifo failed !");
        return HB_ERROR;
    }
    
    int iChannels = mAudioParam->channels;
    AVSampleFormat eSampleFmt = getAudioInnerFormat(mAudioParam->pri_sample_fmt);
    int  iBufferSize = 0, actualAudioSamples = 0;
    AVFrame* pBufferFrame = nullptr;
    while (true) {
        pBufferFrame = nullptr;
        int iFifoSize = av_audio_fifo_size(mAudioFifo);
        if (iFifoSize < mAudioParam->frame_size) {
            break;
        }
        
        actualAudioSamples = FFMIN(iFifoSize, mAudioParam->frame_size);
        iBufferSize = av_samples_get_buffer_size(NULL, iChannels, actualAudioSamples, eSampleFmt, mAudioParam->mAlign);
        if (iBufferSize <= 0) {
            LOGE("Get sample size error [size:%d] !\n", iBufferSize);
            break;
        }
        if (!pFrameRecycleQueue || !(pBufferFrame = pFrameRecycleQueue->get())) {
            if (!(pBufferFrame = av_frame_alloc())) {
                LOGE("Audio stream alloc frame failed !");
                HBErr = HB_ERROR;
                goto AUDIO_STREAM_END_LABEL;
            }
            pBufferFrame->opaque = nullptr;
            uint8_t* pOutDataBuffer = (uint8_t*)av_mallocz(iBufferSize);
            if (!pOutDataBuffer) {
                LOGE("Audio stream allock audio data failed !");
                HBErr = HB_ERROR;
                goto AUDIO_STREAM_END_LABEL;
            }
            
            HBErr = av_samples_fill_arrays(pBufferFrame->data, pBufferFrame->linesize, \
                        pOutDataBuffer, iChannels, actualAudioSamples, eSampleFmt, mAudioParam->mAlign);
            if (HBErr < 0) {
                LOGE("Audio stream fill sample fata error !");
                HBErr = HB_ERROR;
                goto AUDIO_STREAM_END_LABEL;
            }
            pBufferFrame->nb_samples = actualAudioSamples;
            pBufferFrame->opaque = pOutDataBuffer;
        }
        
        HBErr = av_audio_fifo_read(mAudioFifo, (void**)pBufferFrame->data, actualAudioSamples);
        if (HBErr < actualAudioSamples) {
            LOGE("Read size:%d from audio fifo buffer failed !", actualAudioSamples);
            HBErr = HB_ERROR;
            goto AUDIO_STREAM_END_LABEL;
        }
        mOutTotalOfSamples += HBErr;
        /** 计算音频帧的 pts 时间 */
        pBufferFrame->pts = actualAudioSamples * mOutTotalOfFrame * (mAudioParam->sample_rate * 1.0 / mAudioParam->sample_rate);
        mOutTotalOfFrame++;
        
        if (pFrameQueue->push(pBufferFrame) <= 0) {
            LOGE("Push frame to frame queue error!\n");
            HBErr = HB_ERROR;
            goto AUDIO_STREAM_END_LABEL;
        }
        pEncodeThreadIpcCtx->condP();
    }
    HBErr = HB_OK;
    
AUDIO_STREAM_END_LABEL:
    if (pBufferFrame) {
        if (pBufferFrame->opaque)
            av_freep(&pBufferFrame->opaque);
        av_frame_free(&pBufferFrame);
    }
    return HBErr;
}

void CSAStream::setAudioParam(AudioParams* param) {
    mAudioParam = param;
    int HBErr = initialFifo(&mAudioFifo, getAudioInnerFormat(mAudioParam->pri_sample_fmt), mAudioParam->channels, 1);
    if (HBErr < 0)
        LOGE("Initial fifo error !");
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
