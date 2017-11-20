//
//  CSAudioEffectFactory.c
//  Sample
//
//  Created by zj-db0519 on 2017/11/14.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSAudioEffectFactory.h"
#include "CSAudioResamplerEffect.h"
#include "CSAudioShiftEffect.h"

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
        
        switch (type) {
            case CS_AUDIO_TEMPO_PITCH:
                audioEffect = new CSAudioShiftEffect();
                break;

            case CS_AUDIO_RESAMPLER:
                audioEffect = new CSAudioResamplerEffect();
                break;

            default:

                break;
        }
        
        return audioEffect;
    }
    
}
