//
//  CSVideoEffectFactory.c
//  Sample
//
//  Created by zj-db0519 on 2017/11/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSVideoEffectFactory.h"
#include "CSVideoCropRotateEffect.h"
#include "CSVideoScaleEffect.h"

namespace HBMedia {
    
CSVideoBaseEffect *CSVideoEffectFactory::getVideoEffect(int type)
{
    CSVideoBaseEffect *videoEffect = nullptr;
    switch (type) {
        case CS_VIDEO_SCALE_EFFECT:
            videoEffect = new CSVideoScaleEffect();
            break;
        case CS_VIDEO_CROP_ROTATE:
            videoEffect = new CSVideoCropRotateEffect();
            break;
        default:
            videoEffect = NULL;
            break;
    }
    
    return videoEffect;
}
    
}

