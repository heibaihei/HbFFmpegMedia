//
//  CSVStream.h
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/11.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSVStream_h
#define CSVStream_h

#include <stdio.h>
#include "CSIStream.h"

namespace HBMedia {
    
typedef class CSVStream : public CSIStream
{
public:
    CSVStream();
    virtual ~CSVStream();
    
    virtual int sendRawData(uint8_t* pData, long DataSize, int64_t TimeStamp) override;
    
    virtual int bindOpaque(void *handle) override;
    
    void setVideoParam(ImageParams* param);
    ImageParams* getVideoParam() { return mImageParam; };
    
    /*
    * @func 通过编码器的名称来配置编码器
    * @arg codecname 编码器
    * @return 0 为正常, 其他为异常
    */
    int setEncoder(const char *CodecName);
    
    virtual int stop() override;
    
    virtual int release() override;
    
    int setFrameBufferNum(int num);
    
    virtual void EchoStreamInfo() override;
protected:
    
private:
    int          mQueueFrameNums;
    ImageParams* mImageParam;
} CSVStream;
    
}
#endif /* CSVStream_h */
