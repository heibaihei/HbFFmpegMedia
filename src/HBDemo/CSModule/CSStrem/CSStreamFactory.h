//
//  CSStreamFactory.hpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/11.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSStreamFactory_h
#define CSStreamFactory_h

#include <stdio.h>
#include "CSIStream.h"

namespace HBMedia {
    
typedef enum STREAM_TYPE {
    CS_STREAM_TYPE_NONE      = 0,
    CS_STREAM_TYPE_VIDEO     = 0x01,
    CS_STREAM_TYPE_AUDIO     = 0x02,
    CS_STREAM_TYPE_DATA      = 0x04,
    CS_STREAM_TYPE_ALL       = 0x07,
} STREAM_TYPE;

typedef class CSStreamFactory {
public:
    static CSIStream* CreateMediaStream(STREAM_TYPE type);
} CSStreamFactory;

}

#endif /* CSStreamFactory_h */
