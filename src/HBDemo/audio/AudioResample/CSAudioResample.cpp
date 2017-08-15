//
//  CSAudioResample.c
//  FFmpeg
//
//  Created by zj-db0519 on 2017/8/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSAudioResample.h"

namespace HBMedia {

CSAudioResample::CSAudioResample(AudioParams* targetAudioParam){
    if (!targetAudioParam) {
        LOGE("Initial audio decoder param failed !");
        return;
    }
    mTargetAudioParams = *targetAudioParam;
    mSrcAudioParams = *targetAudioParam;
    mPAudioSwrCtx = nullptr;
}

CSAudioResample::~CSAudioResample()
{
    audioResampleClose();
}

int CSAudioResample::audioSetSourceParams(int channels , enum AVSampleFormat sample_fmt, int sample_rate)
{
    mSrcAudioParams.channels = channels;
    mSrcAudioParams.sample_fmt = sample_fmt;
    mSrcAudioParams.sample_rate = sample_rate;
    return HB_OK;
}

int CSAudioResample::audioSetDestParams(int channels , enum AVSampleFormat sample_fmt, int sample_rate)
{
    mTargetAudioParams.channels = channels;
    mTargetAudioParams.sample_fmt = sample_fmt;
    mTargetAudioParams.sample_rate = sample_rate;
    return HB_OK;
}
    
int CSAudioResample::audioResampleOpen()
{
    audioResampleClose();
        
    mPAudioSwrCtx = swr_alloc();
    if (!mPAudioSwrCtx) {
        LOGE("Alloc audio swr failed !");
        return HB_ERROR;
    }
    
    mPAudioSwrCtx = swr_alloc_set_opts(mPAudioSwrCtx, mTargetAudioParams.channel_layout, mTargetAudioParams.sample_fmt, mTargetAudioParams.sample_rate, av_get_default_channel_layout(mSrcAudioParams.channels), mSrcAudioParams.sample_fmt, mSrcAudioParams.sample_rate, 0, NULL);
    
    swr_init(mPAudioSwrCtx);
    return HB_OK;
}
    
int CSAudioResample::audioResampleClose()
{
    if (mPAudioSwrCtx)
        swr_free(&mPAudioSwrCtx);
    
    return HB_OK;
}

int CSAudioResample::doResample(uint8_t **OutData, uint8_t **InData, int InSamples)
{
    if (!OutData || !InData) {
        LOGE("Audio resample args invalid !");
        return HB_ERROR;
    }
    
    int outputSamplesPerChannel = ((InSamples * mTargetAudioParams.sample_rate) / mSrcAudioParams.sample_rate);
    *OutData = (uint8_t*)av_mallocz(av_samples_get_buffer_size(NULL, mTargetAudioParams.channels, outputSamplesPerChannel, mTargetAudioParams.sample_fmt, 1));
    if (!(OutData)) {
        LOGE("Fast malloc output sample buffer failed !");
        return HB_ERROR;
    }
    
    if (mPAudioSwrCtx)
        return swr_convert(mPAudioSwrCtx, OutData, outputSamplesPerChannel, (const uint8_t **)InData, InSamples);
    
    return HB_ERROR;
}
    
int CSAudioResample::checkDstAudioParamValid(int channels , enum AVSampleFormat sample_fmt, int sample_rate)
{
    if (0 != swr_is_initialized(mPAudioSwrCtx)) {
        /** 非0 表示已经完成初始化，此时需要进行 audioResampleOpen 的操作 */
        if (mTargetAudioParams.channels == channels && mTargetAudioParams.sample_fmt != sample_fmt && mTargetAudioParams.sample_rate && sample_rate)
            return HB_OK;
    }
    return HB_ERROR;
}
    
int CSAudioResample::checkSrcAudioParamValid(int channels , enum AVSampleFormat sample_fmt, int sample_rate)
{
    if (0 != swr_is_initialized(mPAudioSwrCtx)) {
        /** 非0 表示已经完成初始化，此时需要进行 audioResampleOpen 的操作 */
        if (mSrcAudioParams.channels == channels && mSrcAudioParams.sample_fmt != sample_fmt && mSrcAudioParams.sample_rate && sample_rate)
            return HB_OK;
    }
    return HB_ERROR;
}
    
}
