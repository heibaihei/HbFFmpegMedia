//
//  CSVideoEncoder.h
//  Sample
//
//  Created by zj-db0519 on 2017/12/8.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSVideoEncoder_h
#define CSVideoEncoder_h

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
typedef class CSVideoEncoder : public CSMediaBase
{
public:
    CSVideoEncoder();
    ~CSVideoEncoder();
    
    /**
     *  准备工作，对外接口
     */
    virtual int prepare();
    
    virtual int start();
    
    virtual int stop();
    
    virtual int release();
    
    /**
     *  外部传送帧进来
     */
    virtual int sendFrame(AVFrame **OutFrame);
    
    /**
     *  同步等待解码器退出
     */
    virtual int syncWait();
    
protected:
    
    static void* ThreadFunc_Video_Encoder(void *arg);

    /**
     *  编码器初始化、启动、关闭、释放
     */
    int  _EncoderInitial();
    int  _InputInitial();
    int  _SwscaleInitial();
    int  _DoSwscale(AVFrame *pInFrame, AVFrame **pOutFrame);

private:
    
} CSVideoEncoder;

}

#endif /* CSVideoEncoder_h */
