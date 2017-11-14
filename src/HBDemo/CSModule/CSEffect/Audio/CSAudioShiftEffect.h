//
//  CSAudioShiftEffect.h
//  Sample
//
//  Created by zj-db0519 on 2017/11/14.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSAudioShiftEffect_h
#define CSAudioShiftEffect_h

#include <stdio.h>
#include "CSAudioBaseEffect.h"
#include "SoundTouch.h"

namespace HBMedia {
    
class CSAudioShiftEffect : public CSAudioBaseEffect {
public:
    CSAudioShiftEffect();
    ~CSAudioShiftEffect();
    const char *getDescripe();
    int setInParam(AudioParams *param);
    int setEffectParam(AudioEffectParam *param);
    int init();
    int transfer(uint8_t *inData, int inSamples, uint8_t *outData, int outSamples);
    
    int pushSamples(uint8_t *inData, int inSamples);
    
    int popSamples(uint8_t *outData, int outSample);
    
    int flush(uint8_t *outData, int outSample);
    
    int release();
    
private:
    AudioParams audioParam;
    AudioEffectParam effectParam;
    soundtouch::SoundTouch s_touch;
};

}

#endif /* CSAudioShiftEffect_h */
