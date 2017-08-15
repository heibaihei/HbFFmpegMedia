//
//  CSAudioDecoder.h
//  FFmpeg
//
//  Created by zj-db0519 on 2017/8/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSAudioDecoder_h
#define CSAudioDecoder_h

#include <stdio.h>
#include <stdint.h>
#include <iostream>

#include "HBAudio.h"

namespace HBMedia {
    
#define INVALID_STREAM_INDEX  (-1)

#define DECODE_MODE_SEPERATE_AUDIO_WITH_VIDEO  0
    
#define DECODE_STATE_READPKT_END     0X0001
#define DECODE_STATE_DECODE_END      0X0002
#define DECODE_STATE_READPKT_ABORT   0X0004
#define DECODE_STATE_DECODE_ABORT    0X0008
#define DECODE_STATE_FLUSH_MODE      0X0010
    
/**
 *  1、解码包含有音视频流的媒体文件
 *  2、对接编码器
 */
    
typedef class CSAudioDecoder
{
public:
    CSAudioDecoder(AudioParams* targetAudioParam);
    ~CSAudioDecoder();
    
    /**
     *  配置输入的音频文件
     */
    void setInputAudioMediaFile(char *file);
    char *getInputAudioMediaFile();
    
    /**
     *  配置输出的音频文件
     */
    void setOutputAudioMediaFile(char *file);
    char *getOutputAudioMediaFile();
    
    /** 
     *  解码器初始化、启动、关闭、释放
     */
    int  audioDecoderInitial();
    int  audioDecoderOpen();
    int  audioDecoderClose();
    int  audioDecoderRelease();
    
    /**
     *  转码器的初始化、关闭、释放
     */
    int  audioDecoderSwrConvertInitial();
    int  audioDecoderSwrConvertClose();
    int  audioDecoderSwrConvertRelease();
    
    /**
     *  音频读包
     *  @return HB_OK 执行正常
     *          HB_ERROR 执行发生异常
     */
    int  readAudioPacket();
    
    /**
     *  音频解帧
     */
    int  selectAudieoFrame();
    
    /**
     *  音频数据格式转换
     *  @return HB_ERROR 发生异常; >=0 表示转换后返回的音频数据大小
     */
    int  doSampleSwrConvert(AVFrame* frame, uint8_t** pOutputSamplebuffer, int* OutputSamplebufferSize);
    
    /**
     *  将音频数据写入音频缓冲区
     */
    int  CSIOPushDataBuffer(uint8_t* data, int samples);
    
private:
    /**
     *  检验音频参数的有效性
     */
    int  _checkAudioParamValid();
    
protected:
    int mPKTSerial;
    char *mInputAudioMediaFile;
    
    int mAudioStreamIndex;
    AVAudioFifo *mAudioFifo;
    AudioParams mTargetAudioParams;
    AVFormatContext* mPInputAudioFormatCtx;
    AVCodecContext* mPInputAudioCodecCtx;
    AVCodec* mPInputAudioCodec;
    struct SwrContext *mPAudioSwrCtx;

    PacketQueue mPacketCacheList;
    
    /** 输出文件，只是针对不做编码的情况下，将音频数据以裸流PCM格数的方式输出 */
    char *mOutputAudioMediaFile;
    FILE *mAudioOutputFileHandle;
    
    int64_t mDecodeStateFlag;
    uint8_t* mTargetAudioSampleBuffer;
    int mTargetAudioSampleBufferSize;
    
private:
    
} CSAudioDecoder;
    
}

#endif /* CSAudioDecoder_h */
