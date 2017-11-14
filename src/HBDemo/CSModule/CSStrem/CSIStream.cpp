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
    mStreamThreadParam = nullptr;
    mFmtCtx = nullptr;
    mStream = nullptr;
    mCodec = nullptr;
    mCodecCtx = nullptr;
    mSrcFrame = nullptr;
    mPushDataWithSyncMode = true;
    mStreamType = CS_STREAM_TYPE_NONE;
    mStreamIndex = -1;
    mSpeed = 0.1f;
}

CSIStream::~CSIStream(){
    
}

}
