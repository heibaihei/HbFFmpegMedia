//
//  CSIStream.hpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/11.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSIStream_h
#define CSIStream_h

#include <stdio.h>
#include "CSLog.h"
#include "CSDefine.h"
#include "CSCommon.h"
#include "CSUtil.h"
#include "CSWorkContext.h"
#include "CSThreadIPCContext.h"

namespace HBMedia {
    
typedef class CSIStream {
    friend class CSTimeline;
public:
    CSIStream();
    virtual ~CSIStream();
    
    virtual int bindOpaque(void *handle) = 0;
    
    virtual int sendRawData(uint8_t* pData, long DataSize, int64_t TimeStamp) = 0;
    
    virtual int stop() = 0;
    
    virtual int release() = 0;
    
    void setStreamIndex(int index) { mStreamIndex = index; }
    void setStreamType(STREAM_TYPE type) { mStreamType = type; }
    
    /** 设置流的线程参数 */
    StreamThreadParam* getStreamThreadParam() { return mStreamThreadParam; };
    
    virtual void EchoStreamInfo() = 0;
    
    /** 设置速度接口 */
    void setSpeed(float speed) { mSpeed = speed; }
    float getSpeed() { return mSpeed; }
    
    /*
     * @func 通过编码器的名称来配置编码器
     * @arg codecname 编码器
     * @return 0 为正常, 其他为异常
     */
    int setEncoder(const char *CodecName);
    int setEncoder(const AVCodecID CodecID);
    
protected:
    AVFormatContext* mFmtCtx;
    AVStream*        mStream;
    AVCodecContext*  mCodecCtx;
    AVCodec*         mCodec;
    enum AVCodecID   mCodecID;
    
    float       mSpeed;
    STREAM_TYPE mStreamType;
    int mStreamIndex;
    /** 当前流的媒体信息，用于线程间通信使用 */
    StreamThreadParam* mStreamThreadParam;
    ThreadIPCContext * mThreadIPCCtx;
    AVFrame     *mSrcFrame;
    
    /** 以阻塞的方式往内部填充数据 */
    bool mPushDataWithSyncMode;
private:
    
} CSIStream;

}

#endif /* CSIStream_h */
