//
//  CSVideo.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/1.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSVideoDemo.h"
#include "CSModule.h"

int CSVideoDemo_VideoDecoder() {
    HBMedia::CSVideoDecoder* pVideoDecoder = new HBMedia::CSVideoDecoder();
    pVideoDecoder->prepare();
    pVideoDecoder->start();
    pVideoDecoder->stop();
    pVideoDecoder->release();
    
    return HB_OK;
}
