//
//  CSVideoUtil.h
//  Sample
//
//  Created by zj-db0519 on 2017/11/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSVideoUtil_h
#define CSVideoUtil_h

#include <stdio.h>
#include <vector>
#include "CSVideoBaseEffect.h"

namespace HBMedia {
    
class CSVideoUtil {
public:
    CSVideoUtil();
    ~CSVideoUtil();
    
    int init();
    
    /*
     * @func 添加效果处理.
     * @arg videoeffect 效果处理基类
     * @return 返回处理数据量（单位sample）
     */
    int addEffect(CSVideoBaseEffect *videoeffect);
    
    /*
     * @func 数据转换.
     * @arg inData 输入数据 inSamples 输出的音频sample数 outData 输出数据 outSample 输出音频sample最大数，outSample 需要比输出的数据大
     * @return 返回处理数据量 > 0 为正常其他异常,其他则异常
     */
    size_t transfer(uint8_t *inData, size_t inSize, uint8_t *outData, size_t outSize);
    
    /*
     * @func 数据刷新,当最后不需要再写入数据的时候调用
     * @arg outData 输出数据 outSize 输出最大数据量
     * @return 返回处理数据量
     */
    int flush(uint8_t *outData, size_t outSize);
    
    /*
     * @func 释放效果处理类
     * @arg void
     * @return 0正常，其他异常
     */
    int release();
    
private:
    
    std::vector<CSVideoBaseEffect *>videoEffectList;
    uint8_t *videoEffectBuf;
    size_t maxBufSize;
    
};
    
}

#endif /* CSVideoUtil_h */
