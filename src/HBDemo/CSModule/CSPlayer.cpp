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
}

CSPlayer::~CSPlayer(){
    
}

int CSPlayer::prepare() {
    if (mTimeline->prepare() != HB_OK) {
        LOGE("Player prepare timeline failed !");
        return HB_ERROR;
    }
    
    
    mTimeline->writeHeader();
    return HB_OK;
}

int CSPlayer::start() {
    return HB_OK;
}
    
    


}

