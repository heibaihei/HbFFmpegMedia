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
    mSwrCtx = NULL;
    audioParamInit(&mInAudioParam);
    audioParamInit(&mOutAudioParam);
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
    if (!param || param->channels <= 0 || param->pri_sample_fmt <= CS_SAMPLE_FMT_NONE
        || param->sample_rate <= 8000) {
        LOGE("Set audio resample effect input params failed !");
        return HB_ERROR;
    }
    
    memcpy(&mInAudioParam, param, sizeof(AudioParams));
    return HB_OK;
}

int CSAudioResamplerEffect::setOutParam(AudioParams *param)
{
    if (!param || param->channels <= 0 || param->pri_sample_fmt <= CS_SAMPLE_FMT_NONE
        || param->sample_rate <= 8000) {
        LOGE("Set audio resample effect output params failed !");
        return HB_ERROR;
    }
    
    memcpy(&mOutAudioParam, param, sizeof(AudioParams));
    return HB_OK;
}


int CSAudioResamplerEffect::init()
{
    AVSampleFormat inSampleFmt = getAudioInnerFormat(mInAudioParam.pri_sample_fmt);
    AVSampleFormat outSampleFmt = getAudioInnerFormat(mOutAudioParam.pri_sample_fmt);
    
    mSwrCtx = swr_alloc_set_opts(mSwrCtx,
                                av_get_default_channel_layout(mOutAudioParam.channels),
                                outSampleFmt,
                                mOutAudioParam.sample_rate,
                                av_get_default_channel_layout(mInAudioParam.channels),
                                inSampleFmt,
                                mInAudioParam.sample_rate, 0, NULL);
    
    if (!mSwrCtx || swr_init(mSwrCtx) < 0) {
        LOGE("Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!", mInAudioParam.sample_rate, av_get_sample_fmt_name(inSampleFmt) \
               , mInAudioParam.channels, mOutAudioParam.sample_rate \
               , av_get_sample_fmt_name(outSampleFmt) \
               , mOutAudioParam.channels);
        swr_free(&mSwrCtx);
        return HB_ERROR;
    }

    return HB_OK;
}

int CSAudioResamplerEffect::transfer(uint8_t *inData, int inSamples, uint8_t *outData, int outSamples)
{
    int ret;
    uint8_t *inputData[8] = {NULL};
    int lineSize[8] = {0};
    uint8_t *outputData[8] = {NULL};
    int outLinesize[8] = {0};
    
    ret = av_samples_fill_arrays(inputData, lineSize, (uint8_t *)inData, \
                        mInAudioParam.channels, (int)inSamples, getAudioInnerFormat(mInAudioParam.pri_sample_fmt), 1);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Fill sample error![%s]\n", makeErrorStr(ret));
        goto TAR_OUT;
    }
    
    ret = av_samples_fill_arrays(outputData, outLinesize, (uint8_t *)outData, mOutAudioParam.channels, (int)outSamples, getAudioInnerFormat(mOutAudioParam.pri_sample_fmt), 1);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Fill sample error![%s]\n", makeErrorStr(ret));
        goto TAR_OUT;
    }
    
    ret = swr_convert(mSwrCtx, outputData, outSamples, (const uint8_t **)inputData, inSamples);
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
    
    ret = av_samples_fill_arrays(outputData, outLinesize, (uint8_t *)outData, mOutAudioParam.channels, (int)outSamples, getAudioInnerFormat(mOutAudioParam.pri_sample_fmt), 1);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Fill sample error![%s]\n", makeErrorStr(ret));
        goto TAR_OUT;
    }
    
    ret = swr_convert(mSwrCtx, outputData, outSamples, NULL, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "swr_convert() failed [%s]\n", makeErrorStr(ret));
        goto TAR_OUT;
    }
    
TAR_OUT:
    
    return ret;
}

int CSAudioResamplerEffect::release()
{
    
    if (mSwrCtx) {
        swr_free(&mSwrCtx);
    }
    
    return 0;
}
    
}
