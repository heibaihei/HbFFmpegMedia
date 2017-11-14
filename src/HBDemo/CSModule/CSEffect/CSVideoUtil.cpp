//
//  CSVideoUtil.c
//  Sample
//
//  Created by zj-db0519 on 2017/11/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSVideoUtil.h"

namespace HBMedia {
    
CSVideoUtil::CSVideoUtil()
{
    videoEffectBuf = NULL;
    maxBufSize = 0;
}

CSVideoUtil::~CSVideoUtil()
{
    
}


/*
 * @func 添加效果处理.
 * @arg videoeffect 效果处理基类
 * @return 返回处理数据量（单位sample）
 */
int CSVideoUtil::addEffect(CSVideoBaseEffect *videoeffect)
{
    
    if (videoeffect == NULL) {
        return AV_PARM_ERR;
    }
    
    videoEffectList.push_back(videoeffect);
    
    return 0;
}

int CSVideoUtil::init()
{
    CSVideoBaseEffect *videoEffect;
    
    for (std::vector <CSVideoBaseEffect *>::iterator it = videoEffectList.begin(); it != videoEffectList.end(); it++) {
        videoEffect = *it;
        videoEffect->init();
        if (maxBufSize < videoEffect->getBufSize()) {
            maxBufSize = videoEffect->getBufSize();
        }
    }
    
    if (maxBufSize <= 0) {
        return AV_SET_ERR;
    }
    
    if (videoEffectBuf) {
        free(videoEffectBuf);
    }
    
    videoEffectBuf = (uint8_t *)malloc(maxBufSize);
    if (videoEffectBuf == NULL) {
        return AV_MALLOC_ERR;
    }
    
    return 0;
}

/*
 * @func 数据转换.
 * @arg inData 输入数据 inSamples 输出的音频sample数 outData 输出数据 outSample 输出音频sample最大数，outSample 需要比输出的数据大
 * @return 返回处理数据量 > 0 为正常其他异常,其他则异常
 */
size_t CSVideoUtil::transfer(uint8_t *inData, size_t inSize, uint8_t *outData, size_t outSize)
{
    size_t effectCnt;
    size_t ret = 0;
    uint8_t *inDataVideoBuf;
    size_t inVideoBufSize;
    uint8_t *outDataVideoBuf;
    size_t outVideoBufSize;
    CSVideoBaseEffect *videoEffect;
    bool isOutSwitch = false;
    
    effectCnt = videoEffectList.size();
    if (effectCnt <= 0) {
        return AV_NOT_FOUND;
    }
    
    inDataVideoBuf = inData;
    inVideoBufSize = inSize;
    
    if ((effectCnt & 0x01) == 0) {
        outDataVideoBuf = videoEffectBuf;
        outVideoBufSize = maxBufSize;
    } else {
        outDataVideoBuf = outData;
        outVideoBufSize = outSize;
    }
    
    for (std::vector <CSVideoBaseEffect *>::iterator it = videoEffectList.begin(); it != videoEffectList.end(); it++) {
        videoEffect = *it;
#if AUDIO_EFFECT_DEBUG
        printf("Effect: %s\n",audioEffect->getDescripe());
#endif
        ret = videoEffect->transfer(inDataVideoBuf, inVideoBufSize, outDataVideoBuf, outVideoBufSize);
        if (ret <= 0) {
            goto TAR_OUT;
        }
        
        inDataVideoBuf = outDataVideoBuf;
        outVideoBufSize = ret;
        
        if (isOutSwitch) {
            outDataVideoBuf = videoEffectBuf;
            outVideoBufSize = maxBufSize;
            isOutSwitch = false;
        } else {
            outDataVideoBuf = outData;
            outVideoBufSize = outSize;
            isOutSwitch = true;
        }
    }
    
TAR_OUT:
    
    return ret;
}

/*
 * @func 数据刷新,当最后不需要再写入数据的时候调用
 * @arg outData 输出数据 outSize 输出最大数据量
 * @return 返回处理数据量
 */
int CSVideoUtil::flush(uint8_t *outData, size_t outSize)
{
    return 0;
}

/*
 * @func 释放效果处理类
 * @arg void
 * @return 0正常，其他异常
 */
int CSVideoUtil::release()
{
    CSVideoBaseEffect *videoEffect;
    
    for (std::vector <CSVideoBaseEffect *>::iterator it = videoEffectList.begin(); it != videoEffectList.end(); it++) {
        videoEffect = *it;
        if (videoEffect) {
            videoEffect->release();
            delete videoEffect;
        }
    }
    
    std::vector<CSVideoBaseEffect *>().swap(videoEffectList);
    
    if (videoEffectBuf) {
        free(videoEffectBuf);
        videoEffectBuf = NULL;
    }
    
    return 0;
}
    
}

