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
    pVideoDecoder->setInMediaType(MD_TYPE_COMPRESS);
    pVideoDecoder->setOutMediaType(MD_TYPE_RAW_BY_FILE);
    pVideoDecoder->setInMediaFile((char *)CS_COMMON_RESOURCE_ROOT_PATH"/video/100.mp4");
    pVideoDecoder->setOutMediaFile((char *)VIDEO_RESOURCE_ROOT_PATH"/decode/100_raw.mp4");
    
    pVideoDecoder->prepare();
    pVideoDecoder->start();
    while (true) {
        sleep(3);
    }
    pVideoDecoder->stop();
    pVideoDecoder->release();
    
    return HB_OK;
}
