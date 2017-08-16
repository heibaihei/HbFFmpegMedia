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

#include "HBAudio.h"

namespace HBMedia {
    
#define ENCODE_STATE_READ_DATA_END     0X0001
#define ENCODE_STATE_ENCODE_END        0X0002
#define ENCODE_STATE_READ_DATA_ABORT   0X0004
#define ENCODE_STATE_ENCODE_ABORT      0X0008
#define ENCODE_STATE_FLUSH_MODE        0X0010

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
    
    CSAudioResample *mAudioResample;
    
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
