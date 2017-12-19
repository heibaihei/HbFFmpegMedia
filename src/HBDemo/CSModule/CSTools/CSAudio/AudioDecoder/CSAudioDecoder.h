//
//  CSAudioDecoder.h
//  FFmpeg
//
//  Created by zj-db0519 on 2017/8/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSAudioDecoder_h
#define CSAudioDecoder_h

#include <stdio.h>
#include <stdint.h>
#include <iostream>

#include "CSAudio.h"
#include "CSMediaBase.h"
#include "CSThreadContext.h"
#include "frame.h"
#include "CSFiFoQueue.h"

namespace HBMedia {
    

typedef class CSAudioDecoder : public CSMediaBase
{
public:
    static int S_MAX_BUFFER_CACHE;
    CSAudioDecoder();
    ~CSAudioDecoder();
    
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
    static void* ThreadFunc_Audio_Decoder(void *arg);
    
    /**
     *  解码器初始化、启动、关闭、释放
     */
    int  _DecoderInitial();
    int  _ExportInitial();
    int  _ResampleInitial();
    int  _DoResample(AVFrame *pInFrame, AVFrame **pOutFrame);
    
    /** 导出的帧都转换成： AV_TIME_BASE_Q 时间基 表示 */
    int  _DoExport(AVFrame **pOutFrame);

    int  _mediaParamInitial();
    
    int mAudioStreamIndex;
    AudioParams mTargetAudioParams;
    AudioParams mSrcAudioParams;
    
    AVCodecContext* mPInputAudioCodecCtx;
    AVCodec* mPInputAudioCodec;
    struct SwrContext *mPAudioResampleCtx;
    
    FiFoQueue<AVFrame *> *mTargetFrameQueue;
    ThreadIPCContext     *mTargetFrameQueueIPC;
    ThreadIPCContext     *mEmptyFrameQueueIPC;
    
    /** 解码线程上下文 */
    ThreadContext mDecodeThreadCtx;
} CSAudioDecoder;
    
}

#endif /* CSAudioDecoder_h */
