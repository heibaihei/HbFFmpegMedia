//
//  CSVideoDecoder.hpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/1.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSVideoDecoder_h
#define CSVideoDecoder_h

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "CSVideo.h"

namespace HBMedia {

typedef class CSVideoDecoder
{
public:
    CSVideoDecoder(ImageParams& params);
    ~CSVideoDecoder();
    
    /**
     *  配置输入的音频文件
     */
    void setInputVideoMediaFile(char *file);
    char *getInputVideoMediaFile();
    
    /**
     *  配置输出的音频文件
     */
    void setOutputVideoMediaFile(char *file);
    char *getOutputVideoMediaFile();
    
    /**
     *  准备工作，对外接口
     */
    int prepare();
    
    int stop();
    
    /**
     *  音频读包
     *  @return HB_OK 执行正常
     *          HB_ERROR 执行发生异常
     */
    int  readVideoPacket();
    
    /**
     *  音频解帧
     */
    int  selectVideoFrame();
    
    /**
     *  将音频数据写入音频缓冲区
     */
    int  CSIOPushDataBuffer(uint8_t* data, int samples);

protected:
    int videoBaseInitial();
    
    /**
     *  解码器初始化、启动、关闭、释放
     */
    int  videoDecoderInitial();
    int  videoDecoderOpen();
    int  videoSwscalePrepare();
    int  videoDecoderClose();
    int  videoDecoderRelease();
    
    int  videoDoSwscale(uint8_t** inData, int*inDataSize);
    
private:
    /**
     *  检验音频参数的有效性
     */
    int  _checkVideoParamValid();

private:
    int mPKTSerial;
    int mVideoStreamIndex;
    ImageParams mTargetVideoParams;
    ImageParams mSrcVideoParams;
    AVFormatContext* mPInputVideoFormatCtx;
    AVCodecContext* mPInputVideoCodecCtx;
    AVCodec* mPInputVideoCodec;
    SwsContext *mPVideoConvertCtx;
    int mTargetVideoFrameBufferSize;
    uint8_t *mTargetVideoFrameBuffer;
    
    
    PacketQueue mPacketCacheList;
    PacketQueue mFrameCacheList;
    
    char *mVideoOutputFile;
    FILE *mVideoOutputFileHandle;
    
    char *mVideoInputFile;
    FILE *mVideoInputFileHandle;
    
    unsigned long long mDecodeStateFlag;
} CSVideoDecoder;

}

#endif /* CSVideoDecoder_h */
