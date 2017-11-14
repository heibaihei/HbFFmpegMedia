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
    
protected:
    AVFormatContext* mFmtCtx;
    AVStream*        mStream;
    AVCodecContext*  mCodecCtx;
    AVCodec*         mCodec;
    enum AVCodecID   mCodecID;
    
    float       mSpeed;
    STREAM_TYPE mStreamType;
    int mStreamIndex;
    StreamThreadParam* mStreamThreadParam;
    ThreadIPCContext * mThreadIPCCtx;
    
    /** 以阻塞的方式往内部填充数据 */
    bool mPushDataWithSyncMode;
private:
    
} CSIStream;

}

#endif /* CSIStream_h */
