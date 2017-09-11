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

namespace HBMedia {
    
typedef class CSAStream : public CSIStream
{
public:
    CSAStream();
    ~CSAStream();
    
    virtual int sendRawData(uint8_t* pData, long DataSize, int64_t TimeStamp) override;
    
    virtual int bindOpaque(void *handle) override;
    
    virtual void EchoStreamInfo() override;
protected:
    
private:
    
} CSAStream;
    
}

#endif /* CSAStream_h */
