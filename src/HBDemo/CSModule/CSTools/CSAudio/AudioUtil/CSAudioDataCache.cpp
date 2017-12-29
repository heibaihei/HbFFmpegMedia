//
//  CSAudioDataCache.cpp
//  Sample
//
//  Created by zj-db0519 on 2017/12/29.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSAudioDataCache.h"

namespace HBMedia {

int CSAudioDataCache::S_MAX_BUFFER_SIZE = 0;
CSAudioDataCache::CSAudioDataCache() {
    mCacheBuffer = nullptr;
    audioParamInit(&mAudioParams);
}

CSAudioDataCache::~CSAudioDataCache() {
    release();
}
    
void CSAudioDataCache::release() {
    if (mCacheBuffer) {
        av_audio_fifo_reset(mCacheBuffer);
        av_audio_fifo_free(mCacheBuffer);
        mCacheBuffer = nullptr;
    }
    
}

int CSAudioDataCache::CacheInitial()
{
    release();
    S_MAX_BUFFER_SIZE = mAudioParams.sample_rate;
    mNextAudioFramePts = 0;
    mCacheBuffer = av_audio_fifo_alloc(getAudioInnerFormat(mAudioParams.pri_sample_fmt), \
                                mAudioParams.channels, S_MAX_BUFFER_SIZE);
    if(!mCacheBuffer) {
        LOGE("Audio Cache >>> Initial audio cache buffer failed !");
        return HB_ERROR;
    }
    return HB_OK;
}

int CSAudioDataCache::WriteDataToCache(AVFrame *pInFrame) {
    if (pInFrame)
        return 0;
    
    int HBErr = av_audio_fifo_write(mCacheBuffer, (void **)pInFrame->data, pInFrame->nb_samples);
    if(HBErr < pInFrame->nb_samples) {
        LOGE("Audio Cache >>> Write cache buffer faisled, <%d, %d> %s!", pInFrame->nb_samples, HBErr, av_err2str(HBErr));
        return 0;
    }

    return 1;
}

int CSAudioDataCache::ReadDataFromCache(AVFrame **pOutFrame) {
    if (!pOutFrame) {
        LOGE("Audio Cache >>> invalid params !");
        return -1;
    }
    
    int audioSampleBufferSize = av_audio_fifo_size(mCacheBuffer);
    if ((audioSampleBufferSize <= 0) \
        || ((audioSampleBufferSize < mAudioParams.nb_samples))) {
            LOGD("Audio Cache >>> Not whole frame, buffer:%d, Frame size:%d !", audioSampleBufferSize, mAudioParams.nb_samples);
            return 0;
        }
    
    return _ReadDataFromCache(pOutFrame, mAudioParams.nb_samples);
}
    
int CSAudioDataCache::_ReadDataFromCache(AVFrame **pOutFrame, int samples) {
    int HbError = HB_OK;
    AVFrame *pNewFrame = *pOutFrame;
    if (!pNewFrame) {
        if (!(pNewFrame = av_frame_alloc())) {
            LOGE("Audio Cache >>> malloc a valid frame failed !");
            return -1;
        }
    }
    pNewFrame->nb_samples = samples;
    pNewFrame->channels = mAudioParams.channels;
    pNewFrame->channel_layout = av_get_default_channel_layout(pNewFrame->channels);
    pNewFrame->format = getAudioInnerFormat(mAudioParams.pri_sample_fmt);
    pNewFrame->sample_rate = mAudioParams.sample_rate;
    pNewFrame->opaque = nullptr;
    
    HbError = av_samples_alloc(pNewFrame->data, &pNewFrame->linesize[0],
                    pNewFrame->channels, pNewFrame->nb_samples, (enum AVSampleFormat)(pNewFrame->format), mAudioParams.mAlign);
    if (HbError < 0) {
        LOGE("Audio Cache >>> Audio resample malloc output samples buffer failed !");
        return -1;
    }
    pNewFrame->opaque = pNewFrame->data[0];
    HbError = av_audio_fifo_read(mCacheBuffer, (void**)pNewFrame->data, pNewFrame->nb_samples);
    if (HbError < 0) {
        pNewFrame->nb_samples = 0;
        if (*pOutFrame)
            clearImageFrame(*pOutFrame);
        else
            disposeImageFrame(&pNewFrame);
        LOGE("Audio Cache >>> Read audio data from fifo buffer failed, %s!", av_err2str(HbError));
        return -1;
    }
    else if (HbError == 0) {
        pNewFrame->nb_samples = 0;
        LOGI("Audio Cache >>> Read nothing from audio fifo buffer !");
        return 0;
    }
    else {
        pNewFrame->nb_samples = HbError;
        pNewFrame->pts = mNextAudioFramePts;
        mNextAudioFramePts += pNewFrame->nb_samples;
        LOGD("Audio Cache >>> Read data from fifo size:%d, pts:%lld, %lf, duration:%lf !",  pNewFrame->nb_samples, \
             pNewFrame->pts, pNewFrame->pts * av_q2d((AVRational){1, pNewFrame->sample_rate}), \
             pNewFrame->nb_samples * av_q2d((AVRational){1, pNewFrame->sample_rate}));
        *pOutFrame = pNewFrame;
        return 1;
    }
}
    
int CSAudioDataCache::FlushDataCache(AVFrame **pOutFrame) {
    if (!pOutFrame) {
        LOGE("Audio Cache >>> invalid params !");
        return -1;
    }
    
    int audioSampleBufferSize = av_audio_fifo_size(mCacheBuffer);
    if (audioSampleBufferSize <= 0) {
        LOGD("Audio Cache >>> Not cache any data buffer !");
        return -2;
    }
    
    return _ReadDataFromCache(pOutFrame, FFMIN(audioSampleBufferSize, mAudioParams.nb_samples));
}

}
