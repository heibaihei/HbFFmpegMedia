//
//  OutFileContext.hpp
//  audioDecode
//
//  Created by Adiya on 17/05/2017.
//  Copyright © 2017 meitu. All rights reserved.
//

#ifndef OutFileContext_hpp
#define OutFileContext_hpp

#include "AudioResampler.hpp"
#include "AudioEncoder.hpp"
#include "ringbuffer.h"
#include "AudioParam.h"

#include "Common.h"

#include <stdio.h>

class OutFileContext {
public:
    OutFileContext();
    ~OutFileContext();
    
    /*
     * @func 打开输入文件
     * @arg filename: 输出文件名
     * @return 返回0正常，其他为异常
     */
    int open(const char *filename);
    /*
     * @func 设置输出的音频格式，默认音频的通道数为1，采样率为44100， 采样格式为s16
     * @arg format: 输出编码格式的名称，目前支持 ‘aac’ 和 ‘mp3’
     * @return 返回0为正常，其他为异常
     */
    int setOutAudioFormat(const char *format);
    /*
     * @func 设置输入音频数据的格式
     * @arg inChannels: 输入音频通道数 inSampleRate： 输入音频采样率 inSampleFmt：输入音频的采样格式
     * @return 返回0为正常，其他为异常
     */
    int setInAudioFormat(int inChannels, int inSampleRate, int inSampleFmt);
    
    /*
     * @func 设置输入音频数据的格式
     * @arg param: 音频参数结构体
     * @return 返回0为正常，其他为异常
     */
    int setInAudioFormat(AudioParam_t *param);
    
    /*
     * @func 设置输出的音频格式
     * @arg format: 输出编码格式的名称，目前支持 ‘aac’ 和 ‘mp3’ param：输出音频信息结构体
     * outSampleFmt： 输出的采样格式
     * @return 0为正常，其他为异常
     */
    int setOutAudioFormat(const char *format, AudioParam_t *param);
    /*
     * @func 写入解码后的PCM数据
     * @arg inData： 输入数据， size： 数据大小
     * @return 0为正常，其他为异常
     */
    int writeAudioDecodeData(uint8_t *inData, int size);
    
    /*
     * @func 刷新编码器，清理编码器中的缓存数据
     * @arg void
     * @return void
     */
    void flushEncoders(void);
    
    /*
     * @func 关闭输出文件
     * @arg void
     * @return 0为正常，其他为异常
     */
    int close();

private:
    AVFormatContext *ofmtCtx;
    AVStream *audioStream;
    AVCodecContext *audioCtx;
    AudioEncoder *audioEnctx;
    AudioResampler *resampler;
    AVFrame *frame;
    RingBuffer *rb1;
    uint8_t *outAudiobuf;
    int outAudioBufSize;
    AVAudioFifo *audioFifo;
    AudioParam_t inParam;
    AudioParam_t outParam;
    int channels;
    int sampleRate;
    int sampleFmt;
    
    int audioIndex;
    int videoIndex;
    bool needResample;
    int sampleSize;
    double durationOfAudio;
};

#endif /* OutFileContext_hpp */
