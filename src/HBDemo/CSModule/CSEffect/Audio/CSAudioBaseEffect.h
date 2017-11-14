//
//  CSIAudioEffect.h
//  Sample
//
//  Created by zj-db0519 on 2017/11/14.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSIAudioEffect_h
#define CSIAudioEffect_h

#include <stdio.h>
#include "CSDefine.h"

namespace HBMedia {
    /*
     * 音频效果处理基类
     */
    class CSAudioBaseEffect {
        
    public:
        CSAudioBaseEffect(){};
        virtual ~CSAudioBaseEffect(){};
        
        /*
         * @func 获取效果处理介绍.
         * @arg void
         * @return 效果介绍字符串
         */
        virtual const char *getDescripe(){return NULL;};
        
        /*
         * @func 设置输入音频参数.
         * @arg param 输入音频参数
         * @return 0正常，其他异常
         */
        virtual int setInParam(AudioParams *param){return -1;};
        
        /*
         * @func 设置输出音频参数.
         * @arg param 输出音频参数
         * @return 0正常，其他异常
         */
        virtual int setOutParam(AudioParams *param){return -1;};
        
        /*
         * @func 设置音频效果参数.
         * @arg param 音频效果参数
         * @return 0正常，其他异常
         */
        virtual int setEffectParam(AudioEffectParam *param){return -1;};
        
        /*
         * @func 设置音频效果参数.
         * @arg param 音频效果参数
         * @return 0正常，其他异常
         */
        virtual int init()=0;
        
        /*
         * @func 数据转换.
         * @arg inData 输入数据 inSamples 输出的音频sample数 outData 输出数据 outSample 输出音频sample最大数，outSample 需要比输出的数据大
         * @return 返回处理数据量（单位sample）
         */
        virtual int transfer(uint8_t *inData, int inSamples, uint8_t *outData, int outSample)=0;
        
        /*
         * @func 数据刷新,当最后不需要再写入数据的时候调用
         * @arg outData 输出数据 outSample 输出最大数据量
         * @return 返回处理数据量（单位sample）
         */
        virtual int flush(uint8_t *outData, int outSample){ return -1; };
        
        /*
         * @func 释放效果处理类
         * @arg void
         * @return 0正常，其他异常
         */
        virtual int release()=0;
        
    };
}

#endif /* CSIAudioEffect_h */
