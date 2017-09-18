//
//  Resampler.cpp
//  audioDecode
//
//  Created by meitu on 2017/5/18.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "Resampler.hpp"
#include "Error.h"

Resampler::Resampler()
{
    swrCtx = NULL;
    audioBuf = NULL;
    audioSize1 = 0;
}

Resampler::~Resampler()
{

}


int Resampler::initResampler(int64_t outChannelsLayout, int outFormat,
                             int outSampleRate, int64_t inChannelsLayout,
                             int inFormat, int inSampleRate)
{
    int ret = 0;
    
    if (swrCtx) {
        swr_free(&swrCtx);
    }
    
    if (outChannelsLayout == inChannelsLayout &&
        outFormat == inFormat &&
        outSampleRate && inSampleRate) {
        ret = AV_STAT_ERR;
        goto TAR_OUT;
    }
    
    /* create resampler context */
    swrCtx = swr_alloc();
    if (!swrCtx) {
        fprintf(stderr, "Could not allocate resampler context\n");
        ret = AVERROR(ENOMEM);
        goto TAR_OUT;
    }
    
    /* set options */
    av_opt_set_int(swrCtx, "in_channel_layout",    inChannelsLayout, 0);
    av_opt_set_int(swrCtx, "in_sample_rate",       inSampleRate, 0);
    av_opt_set_sample_fmt(swrCtx, "in_sample_fmt", (AVSampleFormat)inFormat, 0);
    
    av_opt_set_int(swrCtx, "out_channel_layout",    outChannelsLayout, 0);
    av_opt_set_int(swrCtx, "out_sample_rate",       outSampleRate, 0);
    av_opt_set_sample_fmt(swrCtx, "out_sample_fmt", (AVSampleFormat)outFormat, 0);
    
    /* initialize the resampling context */
    if ((ret = swr_init(swrCtx)) < 0) {
        fprintf(stderr, "Failed to initialize the resampling context\n");
        goto TAR_OUT;
    }
    
    oChLayout = outChannelsLayout;
    inChLayout = inChannelsLayout;
    oSampleFormat = outFormat;
    iSampleFormat = inFormat;
    oSampleRate = outSampleRate;
    iSampleRate = inSampleRate;
    
TAR_OUT:
    
    return ret;
}

int Resampler::convert(uint8_t **outData, size_t outDataSize, const uint8_t **inData, size_t inDataSize)
{
    int ret;
    
    av_fast_malloc(outData, &audioSize1, outDataSize);
    if (!*outData) {
        return AVERROR(ENOMEM);
    }
    
    ret = swr_convert(swrCtx, outData, (int)outDataSize, inData, (int)inDataSize);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Convert data failed\n");
        goto TAR_OUT;
    }
    
TAR_OUT:
    
    return ret;
}

int Resampler::compensation(int sampleDelta, int distance)
{
    return swr_set_compensation(swrCtx, sampleDelta, distance);
}

/* 解初始化采样结构 */
int Resampler::deinitResampler()
{
    int ret = 0;
    
    if (swrCtx) {
        swr_free(&swrCtx);
    }
    
    if (audioBuf) {
        av_freep(audioBuf);
    }
    
    return ret;
}
