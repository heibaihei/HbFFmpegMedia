//
//  CSAudioEncoder.c
//  FFmpeg
//
//  Created by zj-db0519 on 2017/8/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSAudioEncoder.h"

namespace HBMedia {
    
CSAudioEncoder::CSAudioEncoder(AudioParams* targetAudioParam)
{
    if (!targetAudioParam) {
        LOGE("Initial audio decoder param failed !");
        return;
    }
    mTargetAudioParams = *targetAudioParam;
    
}

CSAudioEncoder::~CSAudioEncoder()
{

}

}
