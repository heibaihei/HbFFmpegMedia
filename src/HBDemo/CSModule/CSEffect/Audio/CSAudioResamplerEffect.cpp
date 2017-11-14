//
//  CSResamplerEffect.c
//  Sample
//
//  Created by zj-db0519 on 2017/11/14.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSAudioResamplerEffect.h"
#include "CSCommon.h"

namespace HBMedia {

CSAudioResamplerEffect::CSAudioResamplerEffect()
{
    swrCtx = NULL;
    memset(&inParam, 0, sizeof(AudioParams));
    memset(&outParam, 0, sizeof(AudioParams));
}

CSAudioResamplerEffect::~CSAudioResamplerEffect()
{
    
}

const char *CSAudioResamplerEffect::getDescripe()
{
    return nullptr;// Huangcl descrp;
}

int CSAudioResamplerEffect::setInParam(AudioParams *param)
{
    if (param->channels <= 0 || param->pri_sample_fmt <= CS_SAMPLE_FMT_NONE
        || param->sample_rate <= 8000) {
        return AV_PARM_ERR;
    }
    
    memcpy(&inParam, param, sizeof(AudioParams));
    
    return 0;
}

int CSAudioResamplerEffect::setOutParam(AudioParams *param)
{
    if (param->channels <= 0 || param->pri_sample_fmt <= CS_SAMPLE_FMT_NONE
        || param->sample_rate <= 8000) {
        return AV_PARM_ERR;
    }
    
    memcpy(&outParam, param, sizeof(AudioParams));
    
    return 0;
}


int CSAudioResamplerEffect::init()
{
    int64_t inChannelLayout;
    int64_t outChannelLayout;
    AVSampleFormat inSampleFmt;
    AVSampleFormat outSampleFmt;
    
    inChannelLayout = av_get_default_channel_layout(inParam.channels);
    outChannelLayout = av_get_default_channel_layout(outParam.channels);
    inSampleFmt = getAudioInnerFormat(inParam.pri_sample_fmt);
    outSampleFmt = getAudioInnerFormat(outParam.pri_sample_fmt);
    
    swrCtx = swr_alloc_set_opts(swrCtx,
                                outChannelLayout,
                                outSampleFmt,
                                outParam.sample_rate,
                                inChannelLayout,
                                inSampleFmt,
                                inParam.sample_rate, 0, NULL);
    
    if (!swrCtx || swr_init(swrCtx) < 0) {
        av_log(NULL, AV_LOG_ERROR,
               "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!",
               inParam.sample_rate, av_get_sample_fmt_name(inSampleFmt)
               , inParam.channels, outParam.sample_rate
               , av_get_sample_fmt_name(outSampleFmt)
               , outParam.channels);
        swr_free(&swrCtx);
        return AV_MALLOC_ERR;
    }

    return 0;
}

int CSAudioResamplerEffect::transfer(uint8_t *inData, int inSamples, uint8_t *outData, int outSamples)
{
    int ret;
    uint8_t *inputData[8] = {NULL};
    int lineSize[8] = {0};
    uint8_t *outputData[8] = {NULL};
    int outLinesize[8] = {0};
    
    ret = av_samples_fill_arrays(inputData, lineSize, (uint8_t *)inData, \
                        inParam.channels, (int)inSamples, getAudioInnerFormat(inParam.pri_sample_fmt), 1);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Fill sample error![%s]\n", makeErrorStr(ret));
        goto TAR_OUT;
    }
    
    ret = av_samples_fill_arrays(outputData, outLinesize, (uint8_t *)outData, outParam.channels, (int)outSamples, getAudioInnerFormat(outParam.pri_sample_fmt), 1);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Fill sample error![%s]\n", makeErrorStr(ret));
        goto TAR_OUT;
    }
    
    ret = swr_convert(swrCtx, outputData, outSamples, (const uint8_t **)inputData, inSamples);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "swr_convert() failed [%s]\n", makeErrorStr(ret));
        goto TAR_OUT;
    }
    
TAR_OUT:
    
    return ret;
}

int CSAudioResamplerEffect::flush(uint8_t *outData, int outSamples)
{
    uint8_t *outputData[8]={NULL};
    int outLinesize[8]={0};
    int ret;
    
    ret = av_samples_fill_arrays(outputData, outLinesize, (uint8_t *)outData, outParam.channels, (int)outSamples, getAudioInnerFormat(outParam.pri_sample_fmt), 1);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Fill sample error![%s]\n", makeErrorStr(ret));
        goto TAR_OUT;
    }
    
    ret = swr_convert(swrCtx, outputData, outSamples, NULL, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "swr_convert() failed [%s]\n", makeErrorStr(ret));
        goto TAR_OUT;
    }
    
TAR_OUT:
    
    return ret;
}

int CSAudioResamplerEffect::release()
{
    
    if (swrCtx) {
        swr_free(&swrCtx);
    }
    
    return 0;
}
    
}
