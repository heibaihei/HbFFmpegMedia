//
//  CSPlayer.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/10.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSPlayer.h"

namespace HBMedia {

CSPlayer::CSPlayer(){
    mbSaveMode = false;
}

CSPlayer::~CSPlayer(){
    
}

int CSPlayer::prepare() {
    if (initial() != HB_OK) {
        LOGE("Player prepare call initial failed !");
        return HB_ERROR;
    }
    
    return HB_OK;
}

int CSPlayer::initial() {
    av_register_all();
    av_log_set_flags(AV_LOG_DEBUG);
    
    return HB_OK;
}

}

