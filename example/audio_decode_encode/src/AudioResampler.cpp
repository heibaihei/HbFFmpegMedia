//
//  AudioResampler.cpp
//  audioTranscode
//
//  Created by meitu on 2017/7/18.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "AudioResampler.hpp"
#include "Error.h"

AudioResampler::AudioResampler()
{
    isResample      = false;
    wantedSamples   = 0;
    frameSamples    = 0;
    swr_ctx         = NULL;
    audio_buf_size = 0;
}

AudioResampler::~AudioResampler()
{
    isResample = false;
    if (this->swr_ctx) {
        swr_close(this->swr_ctx);
        swr_free(&this->swr_ctx);
    }
}

void AudioResampler::setInAudioParam(AudioParam_t *param)
{
    memcpy(&inParam, param, sizeof(*param));
}

void AudioResampler::setOutAudioParam(AudioParam_t *param)
{
    memcpy(&outParam, param, sizeof(*param));
}

void AudioResampler::setInAudioParam(int channels, int format, int sampleRate)
{
    inChannels = channels;
    inFormat = format;
    inSampleRate = sampleRate;
}

void AudioResampler::setOutAudioParam(int channels, int format, int sampleRate)
{
    outChannels = channels;
    outFormat = format;
    outSampleRate = sampleRate;
}

void AudioResampler::setWantedSamples(int samples)
{
    wantedSamples = samples;
}

void AudioResampler::setFrameSamples(int samples)
{
    frameSamples = samples;
}

bool AudioResampler::needResample()
{
    return (inChannels != outChannels ||
            inFormat != outFormat ||
            inSampleRate != outSampleRate ||
            wantedSamples != frameSamples);
}

int AudioResampler::initAudioResampler()
{
    int ret = 0;
    
    if (this->swr_ctx) {
        swr_free(&this->swr_ctx);
    }
    
    this->swr_ctx = swr_alloc_set_opts(NULL
                                       , outChannels, (AVSampleFormat)outFormat, outSampleRate
                                       , inChannels, (AVSampleFormat)inFormat, inSampleRate, 0, NULL);
    
    if (!this->swr_ctx || swr_init(this->swr_ctx) < 0) {
        av_log(NULL, AV_LOG_ERROR,
               "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!",
               inSampleRate, av_get_sample_fmt_name((AVSampleFormat)inFormat)
               , inChannels, outSampleRate
               , av_get_sample_fmt_name((AVSampleFormat)outFormat)
               , outChannels);
        swr_free(&this->swr_ctx);
        ret = AV_MALLOC_ERR;
        
        goto TAR_OUT;
    }
    
TAR_OUT:
    
    return ret;
    
}

int AudioResampler::initAudioResampler(int64_t outChannels, int outFormat, int outSampleRate,
                                     int64_t inChannels, int inFormat, int inSampleRate)
{
    int ret;
    
    if (this->swr_ctx) {
        swr_free(&this->swr_ctx);
    }
    
    this->swr_ctx = swr_alloc_set_opts(NULL
                                       , outChannels, (AVSampleFormat)outFormat, outSampleRate
                                       , inChannels, (AVSampleFormat)inFormat, inSampleRate, 0, NULL);
    if (!this->swr_ctx || swr_init(this->swr_ctx) < 0) {
        av_log(NULL, AV_LOG_ERROR,
               "Cannot create sample rate converter for conversion of %d Hz %s %lld channels to %d Hz %s %lld channels!",
               inSampleRate, av_get_sample_fmt_name((AVSampleFormat)inFormat)
               , inChannels, outSampleRate
               , av_get_sample_fmt_name((AVSampleFormat)outFormat)
               , outChannels);
        ret = -1;
        swr_free(&this->swr_ctx);
        goto TAR_OUT;
    }
    
TAR_OUT:
    
    return 0;
}

int AudioResampler::audioConvert(uint8_t **outBuf, int outSize, const uint8_t **inBuf, int inSize)
{
    int ret = 0;
    
    av_fast_malloc(outBuf, &this->audio_buf_size, outSize);
    if (!*outBuf) {
        return AVERROR(ENOMEM);
    }
    ret = swr_convert(this->swr_ctx, outBuf, outSize, inBuf, inSize);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "swr_convert() failed");
        goto TAR_OUT;
    }
    
TAR_OUT:
    
    return ret;
}

int AudioResampler::set_compensation(int sample_delta, int compensation_distance)
{
    if (this->swr_ctx) {
        if (swr_set_compensation(this->swr_ctx, sample_delta,
                                 compensation_distance) < 0) {
            av_log(NULL, AV_LOG_ERROR, "swr_set_compensation() failed");
            return -1;
        }
        
        return 0;
    }
    
    return AV_NOT_INIT;
}
