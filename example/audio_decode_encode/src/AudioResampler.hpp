//
//  AudioResampler.hpp
//  audioTranscode
//
//  Created by meitu on 2017/7/18.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef AudioResampler_hpp
#define AudioResampler_hpp

#include <stdio.h>
#include "Common.h"
#include "AudioParam.h"

class AudioResampler {
public:
    AudioResampler();
    ~AudioResampler();
    void setInAudioParam(AudioParam_t *inParam);
    void setOutAudioParam(AudioParam_t *outParam);
    
    void setInAudioParam(int channels, int inFormat, int inSampleRate);
    void setOutAudioParam(int channels, int inFormat, int inSampleRate);
    void setFrameSamples(int samples);
    void setWantedSamples(int samples);
    bool needResample();
    int initAudioResampler();
    int initAudioResampler(int64_t outChannels, int outFormat, int outSampleRate,
                           int64_t inChannels, int inFormat, int inSampleRate);
    int audioConvert(uint8_t **outBuf, int outSize, const uint8_t **inBuf, int inSize);
    
    int set_compensation(int sample_delta, int compensation_distance);
    
    int writeDecodedData(void **data, int samples);
    
    int readDecodedData(void **data, int samples);
private:
    AudioParam_t inParam;
    AudioParam_t outParam;
    int inChannels;
    int outChannels;
    int inFormat;
    int outFormat;
    int sampleRate;
    int inSampleRate;
    int outSampleRate;
    int inSampleSize;
    int outSampleSize;
    int wantedSamples;
    int frameSamples;
    unsigned int audio_buf_size;
    bool isResample;
    struct SwrContext *swr_ctx;
};

#endif /* AudioResampler_hpp */
