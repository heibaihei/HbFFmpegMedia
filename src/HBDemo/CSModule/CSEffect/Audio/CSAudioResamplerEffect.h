//
//  CSResamplerEffect.h
//  Sample
//
//  Created by zj-db0519 on 2017/11/14.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSAudioResamplerEffect_h
#define CSAudioResamplerEffect_h

#include <stdio.h>

#include "CSAudioBaseEffect.h"

namespace HBMedia {
    
class CSAudioResamplerEffect : public CSAudioBaseEffect {
public:
    CSAudioResamplerEffect();
    ~CSAudioResamplerEffect();
    
    const char *getDescripe();
    
    int setInParam(AudioParams *param);
    
    int setOutParam(AudioParams *param);
    
    int init();
    
    int transfer(uint8_t *inData, int inSamples, uint8_t *outData, int outSamples);
    
    int flush(uint8_t *outData, int outSample);
    
    int release();
    
private:
    AudioParams inParam;
    AudioParams outParam;
    AudioParams audioParam;
    AudioEffectParam effectParam;
    struct SwrContext *swrCtx;
};

}

#endif /* CSAudioResamplerEffect_h */
