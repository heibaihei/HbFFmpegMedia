//
//  CSAudioUtil.c
//  Sample
//
//  Created by zj-db0519 on 2017/11/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSAudioUtil.h"

#ifndef AUDIO_EFFECT_BUFFER_MAX_SIZE
#define AUDIO_EFFECT_BUFFER_MAX_SIZE 81920  // 最大缓存数
#endif

#ifndef AUDIO_EFFECT_BUFFER_SIZE
#define AUDIO_EFFECT_BUFFER_SIZE 10240 // 默认是用s16 该值用于判断传入音频是否超过最大缓存数
#endif

#ifndef AUDIO_EFFECT_DEBUG
#define AUDIO_EFFECT_DEBUG 0
#endif

namespace HBMedia {
    
CSAudioUtil::CSAudioUtil()
{
    audioEffectBuf = NULL;
}

CSAudioUtil::~CSAudioUtil()
{
}

int CSAudioUtil::init()
{
    if (audioEffectBuf) {
        free(audioEffectBuf);
    }
    
    audioEffectBuf = (uint8_t *)malloc(AUDIO_EFFECT_BUFFER_MAX_SIZE);
    if (audioEffectBuf == NULL) {
        return AV_MALLOC_ERR;
    }
    
    return 0;
}

int CSAudioUtil::addEffect(CSAudioBaseEffect *audioEffect)
{
    if (audioEffect == NULL) {
        return AV_PARM_ERR;
    }
    
    audioEffectList.push_back(audioEffect);
    
    return 0;
}

int CSAudioUtil::transfer(uint8_t *inData, int inSamples, uint8_t *outData, int outMaxSamples)
{
    size_t effectCnt;
    int ret = 0;
    uint8_t *inAudioDataSamples;
    int inAudioSamples;
    uint8_t *outAudioDataSamples;
    int outAudioSamples;
    CSAudioBaseEffect *audioEffect;
    bool isOutSwitch = false;
    
    effectCnt = audioEffectList.size();
    if (effectCnt <= 0) {
        return AV_NOT_FOUND;
    }
    
    if (inSamples > AUDIO_EFFECT_BUFFER_SIZE) {
        return AV_PARM_ERR;
    }
    
    inAudioDataSamples = inData;
    inAudioSamples = inSamples;
    
    if ((effectCnt & 0x01) == 0) {
        outAudioDataSamples = audioEffectBuf;
        outAudioSamples = AUDIO_EFFECT_BUFFER_MAX_SIZE;
    } else {
        outAudioDataSamples = outData;
        outAudioSamples = outMaxSamples;
    }
    
    for (std::vector <CSAudioBaseEffect *>::iterator it = audioEffectList.begin(); it != audioEffectList.end(); it++) {
        
        audioEffect = *it;
#if AUDIO_EFFECT_DEBUG
        printf("Effect: %s\n",audioEffect->getDescripe());
#endif
        ret = audioEffect->transfer(inAudioDataSamples, inAudioSamples, outAudioDataSamples, outAudioSamples);
        if (ret <= 0) {
            goto TAR_OUT;
        }
        
        inAudioDataSamples = outAudioDataSamples;
        inAudioSamples = ret;
        
        if (isOutSwitch) {
            outAudioDataSamples = audioEffectBuf;
            outAudioSamples = AUDIO_EFFECT_BUFFER_MAX_SIZE;
            isOutSwitch = false;
        } else {
            outAudioDataSamples = outData;
            outAudioSamples = outMaxSamples;
            isOutSwitch = true;
        }
    }
    
TAR_OUT:
    
    return ret;
}

int CSAudioUtil::flush(uint8_t *outData, int outMaxSamples)
{
    size_t effectCnt;
    int ret = 0;
    uint8_t *inAudioDataSamples = NULL;
    int inAudioSamples = 0;
    uint8_t *outAudioDataSamples = NULL;
    int outAudioSamples = 0;
    CSAudioBaseEffect *audioEffect;
    bool isOutSwitch = false;
    int i;
    
    effectCnt = audioEffectList.size();
    if (effectCnt <= 0) {
        return AV_NOT_INIT;
    }
    
    audioEffect = audioEffectList[0];
    if (effectCnt == 1) {
        return audioEffect->flush(outData, outMaxSamples);
    }
    
    if ((effectCnt & 0x01) == 0) {
        inAudioDataSamples = audioEffectBuf;
        inAudioSamples = AUDIO_EFFECT_BUFFER_MAX_SIZE;
        outAudioDataSamples = outData;
        outAudioSamples = outMaxSamples;
    } else {
        inAudioDataSamples = outData;
        inAudioSamples = outMaxSamples;
        outAudioDataSamples = audioEffectBuf;
        outAudioSamples = AUDIO_EFFECT_BUFFER_MAX_SIZE;
    }
    ret = audioEffect->flush(inAudioDataSamples, inAudioSamples);
    if (ret <= 0) {
        goto TAR_OUT;
    }
    inAudioSamples = ret;
    
    for (i=1; i < effectCnt; i++) {
        audioEffect = audioEffectList[i];
#if AUDIO_EFFECT_DEBUG
        printf("Effect: %s\n",audioEffect->getDescripe());
#endif
        ret = audioEffect->transfer(inAudioDataSamples, inAudioSamples, NULL, 0);
        
        ret = audioEffect->flush(outAudioDataSamples, outAudioSamples);
        if (ret <= 0) {
            goto TAR_OUT;
        }
        
        inAudioDataSamples = outAudioDataSamples;
        inAudioSamples = ret;
        
        if ((i & 0x01) == 0) {
            outAudioDataSamples = outData;
            outAudioSamples = outMaxSamples;
            isOutSwitch = false;
        } else {
            outAudioDataSamples = audioEffectBuf;
            outAudioSamples = AUDIO_EFFECT_BUFFER_MAX_SIZE;
            isOutSwitch = true;
        }
    }
    
TAR_OUT:
    
    
    return ret;
}

int CSAudioUtil::release()
{
    CSAudioBaseEffect *audioEffect;
    
    for (std::vector <CSAudioBaseEffect *>::iterator it = audioEffectList.begin(); it != audioEffectList.end(); it++) {
        audioEffect = *it;
        if (audioEffect) {
            audioEffect->release();
            delete audioEffect;
        }
    }
    
    std::vector<CSAudioBaseEffect *>().swap(audioEffectList);
    
    if (audioEffectBuf) {
        free(audioEffectBuf);
        audioEffectBuf = NULL;
    }
    
    return 0;
}
    
}
