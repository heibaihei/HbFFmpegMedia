//
//  CSWorkContext.h
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/12.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSWorkContext_h
#define CSWorkContext_h

#include <stdio.h>
#include <vector>
#include "CSDefine.h"
#include "CSLog.h"
#include "CSDefine.h"
#include "CSCommon.h"
#include "CSUtil.h"

#include "CSFiFoQueue.h"

namespace HBMedia {

class CSIStream;
class ThreadContext;
class ThreadIPCContext;


/** 每个流的线程参数信息 */
typedef struct StreamThreadParam {
    AVCodecContext* mCodecCtx;
    
    FiFoQueue<AVFrame*> mFrameQueue;
    FiFoQueue<AVFrame*> mFrameRecycleQueue;
    
    FiFoQueue<AVPacket*> mPacketQueue;
    FiFoQueue<AVPacket*> mPacketRecycleQueue;
    
    ThreadIPCContext *mEncodeIPC;
    ThreadIPCContext *mWriteIPC;
    
    /** 指向当前流线程参数对应的线程上下文 */
    ThreadContext *mThreadCtx;
} StreamThreadParam;


/**
 *  Timeline 线程工作空间上的参数信息
 */
typedef struct WorkContextParam {
    int mThreadNum;
    int mStreamIndex;
    ThreadContext    *mWorkThread;
    ThreadIPCContext *mWorkIPCCtx;
    std::vector<StreamThreadParam *> mStreamPthreadParamList;
} WorkContextParam;


typedef class CSWorkContext {
public:
    CSWorkContext();
    ~CSWorkContext();
    
    int pushStream(CSIStream* pStream);
    
    int prepare();
    
    int start();
    
    int stop();
    
    int release();
    
    int flush();
    
protected:
    
private:
    /** 存放当前的线程环境 */
    std::vector<ThreadContext *> mThreadContextList;
    WorkContextParam   *mWorkContextParam;
} CSWorkContext;

}


#endif /* CSWorkContext_h */
