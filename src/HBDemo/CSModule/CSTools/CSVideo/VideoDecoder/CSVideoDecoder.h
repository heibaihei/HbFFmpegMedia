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
#include "CSFiFoQueue.h"

namespace HBMedia {

/** 结构功能解析: 
 *  解码数据，如果不是以文件的方式输出，则内部默认持有数据帧缓冲队列，供外部提取，以阻塞等待的方式
 */
typedef class CSVideoDecoder : public CSMediaBase
{
public:
    static int S_MAX_BUFFER_CACHE;
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
     *  取帧接口
     *  导出的帧都转换成： AV_TIME_BASE_Q 时间基 表示
     */
    virtual int receiveFrame(AVFrame **OutFrame);
    
    /**
     *  同步等待解码器退出
     */
    virtual int syncWait();
    
protected:
    static void* ThreadFunc_Video_Decoder(void *arg);
    
    /**
     *  解码器初始化、启动、关闭、释放
     */
    int  _DecoderInitial();
    int  _ExportInitial();
    int  _SwscaleInitial();
    int  _DoSwscale(AVFrame *pInFrame, AVFrame **pOutFrame);
    
    /** 导出的帧都转换成： AV_TIME_BASE_Q 时间基 表示 */
    int  _DoExport(AVFrame **pOutFrame);

protected:
    /**
     *  媒体参数初始化，重置;
     */
    int  _mediaParamInitial();
    
    int mVideoStreamIndex;
    ImageParams mTargetVideoParams;
    ImageParams mSrcVideoParams;
    
    AVCodecContext* mPInputVideoCodecCtx;
    AVCodec* mPInputVideoCodec;
    SwsContext *mPVideoConvertCtx;
    
    FiFoQueue<AVFrame *> *mTargetFrameQueue;
    ThreadIPCContext     *mTargetFrameQueueIPC;
    ThreadIPCContext     *mEmptyFrameQueueIPC;
    
    /** 解码线程上下文 */
    ThreadContext mDecodeThreadCtx;
    
private:
    
} CSVideoDecoder;

}

#endif /* CSVideoDecoder_h */
