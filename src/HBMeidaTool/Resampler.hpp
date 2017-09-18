//
//  Resampler.hpp
//  audioDecode
//
//  Created by meitu on 2017/5/18.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef Resampler_hpp
#define Resampler_hpp

#include "Common.h"
//#include <libswresample/swresample.h>
#include <stdio.h>

class Resampler {
public:
    Resampler();
    ~Resampler();
    /* 初始化重采样结构  */
    int initResampler(int64_t outChannelsLayout, int outFormat, int outSampleRate,
                      int64_t inChannelsLayout, int inFormat, int inSampleRate);
    
    int convert(uint8_t **outData, size_t outDataSize, const uint8_t **inData, size_t inDataSize);
    
    int compensation(int sampleDelta, int distance);
    
    /* 解初始化采样结构 */
    int deinitResampler();
    
    SwrContext *getSwrCtx();
    
private:
    SwrContext *swrCtx;
    int64_t oChLayout;  // 输出的数据分布格式
    int64_t inChLayout; // 输入的数据分布格式
    int oSampleFormat;  // 输出采样格式
    int iSampleFormat;  // 输入采样格式
    int oSampleRate;    // 输入采样率
    int iSampleRate;    // 输出采样率
    uint8_t *audioBuf;  // 音频缓存数据
    int audioSize;      // 缓存音频数据大小
    unsigned int audioSize1;     // 实际分配内存的大小
};

#endif /* Resampler_hpp */
