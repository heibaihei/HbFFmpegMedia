//
//  CSAudioResample.h
//  FFmpeg
//
//  Created by zj-db0519 on 2017/8/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSAudioResample_h
#define CSAudioResample_h

#include <stdio.h>
#include <stdint.h>
#include <iostream>

#include "CSAudio.h"

namespace HBMedia {
    
typedef class CSAudioResample
{
public:
    CSAudioResample(AudioParams* targetAudioParam);
    ~CSAudioResample();
    
    /**
     *  初始化音频格式转换器
     *  channels|sample_fmt|sample_rate 输出原音频相关个数
     */
    int audioSetSourceParams(int channels , enum AVSampleFormat sample_fmt, int sample_rate);
    int audioSetDestParams(int channels , enum AVSampleFormat sample_fmt, int sample_rate);
    
    /**
     *  开启音频格式转换器
     */
    int audioResampleOpen();
    
    /**
     *  关闭音频解码转换器
     */
    int audioResampleClose();
    
    /**
     *  执行转码操作
     *  [备注] huangcl 如何创建内存，这个需要待确认
     *  @return 返回输出 OutData 的每个通道采样数
     */
    int  doResample(uint8_t **OutData, uint8_t **InData, int InSamples);
    
    /**
     *  检查待转码的音频参数是否于目前解码的音频参数一致
     */
    int checkSrcAudioParamValid(int channels , enum AVSampleFormat sample_fmt, int sample_rate);
    int checkDstAudioParamValid(int channels , enum AVSampleFormat sample_fmt, int sample_rate);
private:
    
protected:
    
private:
    AudioParams mSrcAudioParams;
    AudioParams mTargetAudioParams;
    struct SwrContext *mPAudioSwrCtx;
    
} CSAudioResample;

}

#endif /* CSAudioResample_h */
