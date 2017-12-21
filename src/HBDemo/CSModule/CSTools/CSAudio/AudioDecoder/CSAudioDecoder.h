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
    
    /**
     *  启动解码
     */
    virtual int start();
    
    /**
     *  关闭解码
     */
    virtual int stop();
    
    /**
     *  释放内部资源
     */
    virtual int release();
    
    /**
     *  取帧接口
     *  导出的帧都转换成： AV_TIME_BASE_Q 时间基 表示
     *  @return 0 成功获取到帧;
     *         -1 本次调用取帧失败;
     *         -2 已经解码结束，无帧可取;
     *         -3 当前模式不支持调用取帧;
     */
    virtual int receiveFrame(AVFrame **OutFrame);
    
    /**
     *  同步等待解码器退出
     */
    virtual int syncWait();

protected:
    /**
     *  解码函数入口
     */
    static void* ThreadFunc_Audio_Decoder(void *arg);
    
    /**
     *  刷新缓冲区
     */
    void _flush();
    
    /**
     *  解码器初始化、启动、关闭、释放
     */
    int  _DecoderInitial();
    
    /**
     *  输出初始化
     */
    int  _ExportInitial();
    
    /**
     *  重采样初始化
     */
    int  _ResampleInitial();
    
    /**
     *  @func _DoResample 执行重采样操作
     *  @param pInFrame  要重采样的目标帧；
     *  @param pOutFrame 重采样后需输出的帧；
     *  @return HB_OK 重采样; HB_ERROR 重采样失败
     */
    int  _DoResample(AVFrame *pInFrame, AVFrame **pOutFrame);
    
    /**
     *  导出的帧都转换成： AV_TIME_BASE_Q 时间基 表示
     */
    int  _DoExport(AVFrame **pOutFrame);

    /**
     *  媒体源输入输出类型检验检查
     */
    int  _mediaParamInitial();
    
    int  mAudioStreamIndex;
    
    /***/
    AVCodecContext    *mPInputAudioCodecCtx;
    AVCodec           *mPInputAudioCodec;
    struct SwrContext *mPAudioResampleCtx;
    
    FiFoQueue<AVFrame *> *mTargetFrameQueue;
    ThreadIPCContext     *mTargetFrameQueueIPC;
    ThreadIPCContext     *mEmptyFrameQueueIPC;
    ThreadContext         mDecodeThreadCtx;
} CSAudioDecoder;
    
}

#endif /* CSAudioDecoder_h */
