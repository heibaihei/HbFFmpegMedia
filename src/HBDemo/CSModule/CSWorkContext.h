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

#define MAX_FRAME_BUFFER 5
#define MAX_PACKET_BUFFER 10

/** 每个流的线程参数信息 */
typedef struct StreamThreadParam {
    AVCodecContext* mCodecCtx;
    
    FiFoQueue<AVFrame*> *mFrameQueue;
    FiFoQueue<AVFrame*> *mFrameRecycleQueue;
    
    FiFoQueue<AVPacket*> *mPacketQueue;
    FiFoQueue<AVPacket*> *mPacketRecycleQueue;
    
    ThreadIPCContext *mEncodeIPC;
    ThreadIPCContext *mWriteIPC;
    
    bool mUpdateFlag;
    AVPacket *mBufferPacket;
    int64_t mMinPacketPTS;
    
    int mStreamIndex;
    AVRational mTimeBase;
    /** 指向当前流线程参数对应的线程上下文 */
    ThreadContext *mThreadCtx;
} StreamThreadParam;


/**
 *  Timeline 线程工作空间上的参数信息
 */
typedef struct WorkContextParam {
    int mThreadNum;
    AVFormatContext  *mTargetFormatCtx;
    
    /** 数据输出线程以及对应的通信对象 */
    ThreadContext    *mWorkThread;
    ThreadIPCContext *mWorkIPCCtx;
    std::vector<StreamThreadParam *> mStreamPthreadParamList;
} WorkContextParam;

int updateQueue(StreamThreadParam *streamParam);
int clearFrameQueue(FiFoQueue<AVFrame*> *queue);
int clearPacketQueue(FiFoQueue<AVPacket*> *queue);
    
int initialStreamThreadParams(StreamThreadParam *pStreamThreadParam);
int releaseStreamThreadParams(StreamThreadParam *pStreamThreadParam);

typedef class CSWorkContext {
public:
    CSWorkContext();
    ~CSWorkContext();
    
    /** Push 流的操作等到 prepare 操作以后再执行 */
    int pushStream(CSIStream* pStream);
    
    int prepare(void *args);
    
    int start();
    
    int stop();
    
    int release();
    
    int flush();
    
protected:
    /** 创建输出线程 */
    int _createOutputWorker();
    
private:
    /** 存放当前的线程环境 */
    std::vector<ThreadContext *> mThreadContextList;
    WorkContextParam   *mWorkContextParam;
} CSWorkContext;

}


#endif /* CSWorkContext_h */
