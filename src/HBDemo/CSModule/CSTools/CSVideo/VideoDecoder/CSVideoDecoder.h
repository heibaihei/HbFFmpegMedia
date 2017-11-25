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

#include "CSMediaBase.h"

namespace HBMedia {

/** 结构功能解析: 
 *  解码数据，如果不是以文件的方式输出，则内部默认持有数据帧缓冲队列，供外部提取，以阻塞等待的方式
 */
typedef class CSVideoDecoder : public CSMediaBase
{
public:
    CSVideoDecoder();
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
     *  媒体参数初始化，重置;
     */
    int  _mediaParamInitial();
    
private:
    int mPKTSerial;
    int mVideoStreamIndex;
    ImageParams mTargetVideoParams;
    ImageParams mSrcVideoParams;
    AVCodecContext* mPInputVideoCodecCtx;
    AVCodec* mPInputVideoCodec;
    
    /** Ture 表示需要进行转码，否则无需进行转码 */
    bool mIsNeedTransfer;
    SwsContext *mPVideoConvertCtx;
    
    int mTargetVideoFrameBufferSize;
    uint8_t *mTargetVideoFrameBuffer;
    
    
    PacketQueue mPacketCacheList;
    PacketQueue mFrameCacheList;
    
    unsigned long long mDecodeStateFlag;
} CSVideoDecoder;

}

#endif /* CSVideoDecoder_h */
