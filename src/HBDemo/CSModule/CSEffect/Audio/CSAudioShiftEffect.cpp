//
//  CSAudioShiftEffect.c
//  Sample
//
//  Created by zj-db0519 on 2017/11/14.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSAudioShiftEffect.h"

#ifndef MT_RECORD_AUDIO_BUFFER
#define MT_RECORD_AUDIO_BUFFER 81920
#endif

static const char *desp = "Audio tempo and pitch effct";

namespace HBMedia {
    
CSAudioShiftEffect::CSAudioShiftEffect()
{
    effectParam.atempo = 1.0;
    effectParam.pitch = 1.0;
    memset(&audioParam, 0, sizeof(AudioParams));
}

CSAudioShiftEffect::~CSAudioShiftEffect()
{
    
}

const char *CSAudioShiftEffect::getDescripe()
{
    return desp;
}

int CSAudioShiftEffect::setInParam(AudioParams *param)
{
    memcpy(&audioParam, param, sizeof(AudioParams));
    
    return 0;
}

int CSAudioShiftEffect::setEffectParam(AudioEffectParam *param)
{
    memcpy(&effectParam, param, sizeof(AudioEffectParam));
    
    return 0;
}

int CSAudioShiftEffect::init()
{
    s_touch.setChannels(audioParam.channels);
    s_touch.setSampleRate(audioParam.sample_rate);
    s_touch.setSetting(SETTING_USE_QUICKSEEK, 1);
    s_touch.setSetting(SETTING_USE_AA_FILTER, 1);
    s_touch.setTempo(effectParam.atempo);
    
    return 0;
}

int CSAudioShiftEffect::transfer(uint8_t *inData, int inSamples, uint8_t *outData, int outSamples)
{
    int ret;
#ifndef  SOUND_TOUCH_MODULE_EXCLUDE
    s_touch.putSamples((short *)inData, inSamples);
    
    if (outData != NULL && outSamples > 0) {
        ret = s_touch.receiveSamples((short *)outData, MT_RECORD_AUDIO_BUFFER);
        if (ret <= 0) {
            goto TAR_OUT;
        }
    }
#endif
TAR_OUT:
    
    return ret;
}

int CSAudioShiftEffect::pushSamples(uint8_t *inData, int inSamples)
{
    if (inData == NULL || inSamples <= 0) {
        return AV_PARM_ERR;
    }
#ifndef  SOUND_TOUCH_MODULE_EXCLUDE
    s_touch.putSamples((short *)inData, inSamples);
#endif
    return 0;
}

int CSAudioShiftEffect::popSamples(uint8_t *outData, int outSamples)
{
    int ret = 0;
#ifndef  SOUND_TOUCH_MODULE_EXCLUDE
    ret = s_touch.receiveSamples((short *)outData, outSamples);
#endif
    return ret;
}

int CSAudioShiftEffect::flush(uint8_t *outData, int outSamples)
{
    s_touch.flush();
    int ret = 0;
#ifndef  SOUND_TOUCH_MODULE_EXCLUDE
    ret = s_touch.receiveSamples((short *)outData, outSamples);
#endif
    return ret;
}

int CSAudioShiftEffect::release()
{
    s_touch.clear();
    
    return 0;
}
    
}
