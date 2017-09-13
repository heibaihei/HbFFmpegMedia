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

namespace HBMedia {
    
typedef class CSIStream {
public:
    CSIStream();
    ~CSIStream();
    
    virtual int bindOpaque(void *handle) = 0;
    
    virtual int sendRawData(uint8_t* pData, long DataSize, int64_t TimeStamp) = 0;
    
    /** 设置流的线程参数 */
    void setThreadParam(StreamThreadParam *Param) { mThreadParam = Param; };
    StreamThreadParam* getThreadParam() { return mThreadParam; };
    
    virtual void EchoStreamInfo() = 0;
protected:
    
private:
    StreamThreadParam* mThreadParam;
} CSIStream;
    
}

#endif /* CSIStream_h */
