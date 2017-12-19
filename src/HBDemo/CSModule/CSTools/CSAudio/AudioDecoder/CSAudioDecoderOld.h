//
//  CSAudioDecoder.h
//  FFmpeg
//
//  Created by zj-db0519 on 2017/8/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#if 0
#ifndef CSAudioDecoder_h
#define CSAudioDecoder_h

#include <stdio.h>
#include <stdint.h>
#include <iostream>

#include "CSAudio.h"

namespace HBMedia {
    
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

    CSAudioResample *mAudioResample;
    
    PacketQueue mPacketCacheList;
    
    /** 输出文件，只是针对不做编码的情况下，将音频数据以裸流PCM格数的方式输出 */
    char *mOutputAudioMediaFile;
    FILE *mAudioOutputFileHandle;
    
    int64_t mDecodeStateFlag;
    
private:
    
} CSAudioDecoder;
    
}

#endif /* CSAudioDecoder_h */
#endif
