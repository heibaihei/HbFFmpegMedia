//
//  CSAStream.h
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/11.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSAStream_h
#define CSAStream_h

#include <stdio.h>
#include "CSIStream.h"
#include "CSAudioBaseEffect.h"
#include "CSAudioUtil.h"

namespace HBMedia {
    
typedef class CSAStream : public CSIStream
{
public:
    CSAStream();
    virtual ~CSAStream();
    
    virtual int sendRawData(uint8_t* pData, long DataSize, int64_t TimeStamp) override;
    
    virtual int bindOpaque(void *handle) override;
    
    void setAudioParam(AudioParams* param);
    
    virtual int stop() override;
    
    virtual int release() override;
    
    virtual void EchoStreamInfo() override;
protected:
    
private:
    /** 以下三个变量去留待定 */
    long mInTotalOfSamples;  /** 统计输入的音频采样数 */
    long mOutTotalOfSamples;
    long mOutTotalOfFrame;   /** 表示当前输出音频帧数 */
    
    AVFrame     *mSrcFrame;
    AVAudioFifo *mAudioFifo;
    /** 本身媒体流信息 */
    AudioParams *mAudioParam;
} CSAStream;
    
}

#endif /* CSAStream_h */
