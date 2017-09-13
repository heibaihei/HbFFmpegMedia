//
//  CSPlayer.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/10.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSPlayer.h"
#include "CSMediaTimeline.h"

namespace HBMedia {

CSPlayer::CSPlayer(){
    mbSaveMode = false;
    memset(&mStatues, 0x00, sizeof(unsigned long long));
}

CSPlayer::~CSPlayer(){
    
}

int CSPlayer::prepare() {
    if (mTimeline->prepare() != HB_OK) {
        LOGE("Player prepare timeline failed !");
        return HB_ERROR;
    }
    
    return HB_OK;
}

int CSPlayer::start() {
    if (mTimeline) {
        mTimeline->start();
    }
    
    return HB_OK;
}

int CSPlayer::stop() {
    if (mTimeline) {
        mTimeline->stop();
    }
    return HB_OK;
}

int CSPlayer::release(void) {
    if (mTimeline) {
        mTimeline->release();
        SAFE_DELETE(mTimeline);
    }
    return HB_OK;
}

int CSPlayer::writeExternData(uint8_t data[], size_t dataSize, int index, long timeStamp) {
    if (mTimeline) {
        mTimeline->sendRawData(data, dataSize, index, timeStamp);
    }
    return HB_OK;
}

}
