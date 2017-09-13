//
//  CSIStream.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/11.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSIStream.h"

namespace HBMedia {

CSIStream::CSIStream(){
    mThreadParam = nullptr;
    mStreamType = CS_STREAM_TYPE_NONE;
    mStreamIndex = -1;
}

CSIStream::~CSIStream(){
    
}

}
