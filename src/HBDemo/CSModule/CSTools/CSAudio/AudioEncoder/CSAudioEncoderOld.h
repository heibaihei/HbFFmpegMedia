//
//  CSAudioEncoder.h
//  FFmpeg
//
//  Created by zj-db0519 on 2017/8/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSAudioEncoder_h
#define CSAudioEncoder_h

#include <stdio.h>
#include <stdint.h>
#include <iostream>

#include "CSAudio.h"

namespace Hello {

typedef class CSAudioEncoder
{
public:
    CSAudioEncoder(AudioParams* targetAudioParam);
    ~CSAudioEncoder();
    
    static enum AVCodecID mTargetCodecID;
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
    int  audioEncoderInitial();
    int  audioEncoderOpen();
    int  audioEncoderClose();
    int  audioEncoderRelease();
    
    /**
     *  从外部资源获取PCM 数据
     */
    int getPcmData(uint8_t** pData, int* dataSizes);
    
    /**
     *  将得到的pcm 数据丢到缓冲区
     */
    int pushPcmDataToAudioBuffer(uint8_t* pData, int dataSizes);
    
    /**
     *  解码数据
     */
    int selectAudioFrame();
    
private:
    
protected:
    int _initialOutputFrame(AVFrame** frame, AudioParams *pAudioParam, int AudioSamples);
    
private:
    FILE *mInputAudioMediaFileHandle;
    char *mInputAudioMediaFile;
    char *mOutputAudioMediaFile;
    
    int mAudioStreamIndex;
    AVAudioFifo *mAudioFifo;
    AudioParams mTargetAudioParams;
    
    AVCodecContext* mPOutputAudioCodecCtx;
    AVCodec* mPOutputAudioCodec;
    
    HBMedia::CSAudioResample *mAudioResample;
    
    PacketQueue mPacketCacheList;
    int64_t mEncodeStateFlag;
    
    int mPerFrameBufferSizes;
    int64_t mLastAudioFramePts;
    /** 测试结构成员 */
    AVStream* mPOutputAudioStream;
    AVFormatContext* mPOutputAudioFormatCtx;
} CSAudioEncoder;
    
} /** HBMedia */

#endif /* CSAudioEncoder_h */
