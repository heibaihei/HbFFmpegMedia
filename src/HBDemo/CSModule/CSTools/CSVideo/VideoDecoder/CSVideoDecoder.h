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
#include <pthread.h>
#include <unistd.h>

#include "CSMediaBase.h"
#include "CSThreadContext.h"
#include "frame.h"

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
    
    virtual int start();
    
    virtual int stop();
    
    virtual int release();
    
    /**
     *  同步等待解码器退出
     *  测试接口：
     */
    virtual int syncWait();
    
protected:
    static void* ThreadFunc_Video_Decoder(void *arg);
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
    
    /**
     *  解码器初始化、启动、关闭、释放
     */
    int  _DecoderInitial();
    int  _ExportInitial();
    int  _SwscaleInitial();

    int  _DoSwscale(AVFrame *pInFrame, AVFrame **pOutFrame);

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
    
    SwsContext *mPVideoConvertCtx;
    
    uint8_t *mTargetVideoFrameBuffer;
    
    PacketQueue mPacketCacheList;
    PacketQueue mFrameCacheList;
    
    /** 解码器状态 */
    unsigned long long mDecodeStateFlag;
    
    /** 解码线程上下文 */
    ThreadContext mDecodeThreadCtx;
} CSVideoDecoder;

}

#endif /* CSVideoDecoder_h */
