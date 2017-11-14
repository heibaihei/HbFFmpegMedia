//
//  CSAudioUtil.h
//  Sample
//
//  Created by zj-db0519 on 2017/11/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSAudioUtil_h
#define CSAudioUtil_h

#include <stdio.h>
#include <vector>
#include "CSAudioBaseEffect.h"

namespace HBMedia {
    
/*
 * 音频解码数据处理工具类，用于处理音频加减速、重采样等工作
 */

class CSAudioUtil {
public:
    CSAudioUtil();
    ~CSAudioUtil();
    
    /*
     * @func 初始化工具类
     * @arg void
     * @return 0 为正常, 其他为异常
     */
    int init();
    
    /*
     * @func 添加需要处理效果
     * @arg audioEffect 处理效果类，通过AudioEffectFactory::getAudioEffect(int type)来获取
     * @return 0 为正常, 其他为异常
     */
    int addEffect(CSAudioBaseEffect *audioEffect);
    
    /*
     * @func 数据转换
     * @arg inData 输入音频数据；inSamples 输入音频采样数；
     * outData 输出音频采样数； outMaxSamples 输出音频最大数
     * @return > 0 转换的数据量（单位samples） <=0 未转换出数据
     */
    int transfer(uint8_t *inData, int inSamples, uint8_t *outData, int outMaxSamples);
    
    /*
     * @func 刷新数据，在最后不再转换数据的时候使用
     * @arg outData 输出音频采样数； outMaxSamples 输出音频最大数
     * @return > 0 转换的数据量（单位samples） <=0 未转换出数据
     */
    int flush(uint8_t *outData, int outMaxSamples);
    
    /*
     * @func 对音频工具类空间的释放
     * @arg void
     * @return 0为正确，其他为异常。
     */
    int release();
    
private:
    
    std::vector<CSAudioBaseEffect *>audioEffectList;
    uint8_t *audioEffectBuf;
};
    
}

#endif /* CSAudioUtil_h */
