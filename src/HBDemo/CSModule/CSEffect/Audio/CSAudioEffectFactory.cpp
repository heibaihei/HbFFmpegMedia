//
//  CSAudioEffectFactory.c
//  Sample
//
//  Created by zj-db0519 on 2017/11/14.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSAudioEffectFactory.h"

namespace HBMedia {
    
    CSAudioEffectFactory::CSAudioEffectFactory()
    {
    }
    
    CSAudioEffectFactory::~CSAudioEffectFactory()
    {
        
    }
    
    CSAudioBaseEffect *CSAudioEffectFactory::getAudioEffect(int type)
    {
        CSAudioBaseEffect *audioEffect = NULL;
        
//        switch (type) {
//            case MT_AUDIO_TEMPO_PITCH:
//                audioEffect = new AudioTempoPitch();
//                break;
//                //#if CONFIG_RESAMPLING_AUDIO_EXAMPLE
//            case MT_AUDIO_RESAMPLER:
//                audioEffect = new AudioResamplerEffect();
//                break;
//                //#endif
//            default:
//
//                break;
//        }
        
        return audioEffect;
    }
    
}
