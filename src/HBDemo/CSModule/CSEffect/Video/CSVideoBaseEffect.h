//
//  CSVideoBaseEffect.h
//  Sample
//
//  Created by zj-db0519 on 2017/11/14.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSVideoBaseEffect_h
#define CSVideoBaseEffect_h

#include "CSDefine.h"

#define MAX_VIDEO_PLANE 4

namespace HBMedia {
/*
 * 视频效果处理基类
 */
class CSVideoBaseEffect {
    
public:
    CSVideoBaseEffect(){};
    virtual ~CSVideoBaseEffect(){};
    
    /*
     * @func 获取效果处理介绍.
     * @arg void
     * @return 效果介绍字符串
     */
    virtual const char *getDescripe(){return NULL;};
    
    /*
     * @func 设置输入视频参数.
     * @arg param 输入视频参数
     * @return 0正常，其他异常
     */
    virtual int setInParam(ImageParams *param){return -1;};
    
    /*
     * @func 设置输出视频参数.
     * @arg param 输出视频参数
     * @return 0正常，其他异常
     */
    virtual int setOutParam(ImageParams *param){return -1;};
    
    /*
     * @func 设置视频效果参数.
     * @arg param 视频效果参数
     * @return 0正常，其他异常
     */
    virtual int setEffectParam(VideoEffectParam *param){return -1;};
    
    /*
     * @func 设置音频效果参数.
     * @arg void
     * @return 0正常，其他异常
     */
    virtual int init()=0;
    
    /*
     * @func 获取输出缓存的数据.
     * @arg void
     * @return >0正常，其他异常
     */
    virtual size_t getBufSize()=0;
    
    /*
     * @func 数据转换.
     * @arg inData 输入数据 inSize 输出的视频数据大小 outData 输出数据 outSample 输出视频outSize最大数
     * @return 返回处理数据量 > 0 则正常，其他异常
     */
    virtual size_t transfer(uint8_t *inData, size_t inSize, uint8_t *outData, size_t outSize)=0;
    
    /*
     * @func 数据刷新,当最后不需要再写入数据的时候调用
     * @arg outData 输出数据 outSample 输出最大数据量
     * @return 返回处理数据量
     */
    virtual int flush(uint8_t *outData, size_t outSize){ return -1; };
    
    /*
     * @func 释放效果处理类
     * @arg void
     * @return 0正常，其他异常
     */
    virtual int release()=0;
    
};

}

#endif /* CSVideoBaseEffect_h */
