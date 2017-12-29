//
//  CSAudioEncoder.h
//  Sample
//
//  Created by zj-db0519 on 2017/12/25.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSAudioEncoder_h
#define CSAudioEncoder_h

#include <stdio.h>

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
 *  编码数据，如果不是以文件的方式输出，则内部默认持有数据帧缓冲队列，供外部提取，以阻塞等待的方式
 */
typedef class CSAudioEncoder : public CSMediaBase
{
public:
    static int S_MAX_BUFFER_CACHE;
    CSAudioEncoder();
    ~CSAudioEncoder();
    
    /**
     *  准备工作，对外接口
     */
    virtual int prepare();
    
    virtual int start();
    
    virtual int stop();
    
    virtual int release();
    
    /**
     *  外部传送帧进来，
     *  如果传入的帧必须是以 AV_TIME_BASE_Q 时间基为准
     *  @return HB_OK 送帧成功; HB_ERROR 送帧失败
     */
    virtual int sendFrame(AVFrame **pSrcFrame);
    
    /**
     *  同步等待解码器退出
     */
    virtual int syncWait();
    
    void setInAudioParams(AudioParams& param) { mSrcAudioParams = param; }
    void setOutAudioParams(AudioParams& param) { mTargetAudioParams = param; }
    
protected:
    /**
     *  媒体参数初始化，重置;
     */
    int  _mediaParamInitial();
    
    static void* ThreadFunc_Audio_Encoder(void *arg);
    
    /**
     *  编码器初始化、启动、关闭、释放
     */
    int  _EncoderInitial();
    int  _InputInitial();
    int  _ResampleInitial();
    
    /**
     *  对音频进行重采样
     *  @param pInFrame  输入音频帧，欲重采样原始音频帧
     *  @param pOutFrame 输出目标音频帧，重采样后目标音频帧
     *  @return
     *     HB_OK 重采样成功, pOutFrame 指向输出音频帧;
     *     HB_ERROR 重采样失败, pOutFrame 为 NULL;
     */
    int  _DoResample(const AVFrame *pInFrame, AVFrame **pOutFrame);
    int  _DoExport(AVPacket *pPacket);
    
private:
    void _flushAudioFifo();
    void _flush();
    
private:
    /** 解码相关信息 */
    int mAudioStreamIndex;
    AVCodec* mPOutAudioCodec;
    AVCodecContext* mPOutAudioCodecCtx;
    struct SwrContext *mPAudioResampleCtx;
    
    /** 音频数据缓冲区 */
    CSAudioDataCache mAudioDataCacheObj;
    
    FiFoQueue<AVFrame *> *mSrcFrameQueue;
    ThreadIPCContext     *mSrcFrameQueueIPC;
    ThreadIPCContext     *mEmptyFrameQueueIPC;
    
    /** 编码线程上下文 */
    ThreadContext mEncodeThreadCtx;
} CSAudioEncoder;
}

#endif /* CSAudioEncoder_h */
