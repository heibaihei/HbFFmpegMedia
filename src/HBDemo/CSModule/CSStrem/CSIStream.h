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
    void setThreadParam(StreamThreadParam *Param) { mThreadParam = Param; };
    StreamThreadParam* getThreadParam() { return mThreadParam; };
    
    virtual void EchoStreamInfo() = 0;
    
protected:
    AVFormatContext* mFmtCtx;
    AVStream*        mStream;
    AVCodecContext*  mCodecCtx;
    AVCodec*         mCodec;
    enum AVCodecID   mCodecID;
    
    float       mSpeed;
    STREAM_TYPE mStreamType;
    int mStreamIndex;
    StreamThreadParam* mThreadParam;
    ThreadIPCContext * mThreadIPCCtx;
    
private:
    
} CSIStream;
    
}

#endif /* CSIStream_h */
