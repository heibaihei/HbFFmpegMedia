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

typedef class CSVideoDecoder : public CSMediaBase
{
public:
    CSVideoDecoder(ImageParams& params);
    ~CSVideoDecoder();
    
    /**
     *  准备工作，对外接口
     */
    virtual int prepare();
    
    virtual int start() { return HB_OK; };
    
    virtual int stop();
    
    virtual int release() { return HB_OK; };
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
    
    unsigned long long mDecodeStateFlag;
} CSVideoDecoder;

}

#endif /* CSVideoDecoder_h */
