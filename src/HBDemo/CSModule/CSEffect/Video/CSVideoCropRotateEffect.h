//
//  CSVideoCropRotateEffect.h
//  Sample
//
//  Created by zj-db0519 on 2017/11/14.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSVideoCropRotateEffect_h
#define CSVideoCropRotateEffect_h

#include <stdio.h>
#include "CSVideoBaseEffect.h"

namespace HBMedia {
    
class CSVideoCropRotateEffect : public CSVideoBaseEffect {
public:
    CSVideoCropRotateEffect();
    ~CSVideoCropRotateEffect();
    /*
     * @func 获取效果处理介绍.
     * @arg void
     * @return 效果介绍字符串
     */
    const char *getDescripe();
    
    /*
     * @func 设置输入视频参数.
     * @arg param 输入视频参数
     * @return 0正常，其他异常
     */
    int setInParam(ImageParams *param);
    
    /*
     * @func 设置输出视频参数.
     * @arg param 输出视频参数
     * @return 0正常，其他异常
     */
    int setOutParam(ImageParams *param);
    
    /*
     * @func 设置视频效果参数.
     * @arg param 视频效果参数
     * @return 0正常，其他异常
     */
    int setEffectParam(VideoEffectParam *param);
    
    /*
     * @func 初始化处理参数.
     * @arg void
     * @return 0正常，其他异常
     */
    int init();
    
    /*
     * @func 获取输出缓存的数据.
     * @arg void
     * @return >0正常，其他异常
     */
    size_t getBufSize();
    
    /*
     * @func 数据转换.
     * @arg inData 输入数据 inSize 输出的视频数据大小 outData 输出数据 outSample 输出视频outSize最大数
     * @return 返回处理数据量
     */
    size_t transfer(uint8_t *inData, size_t inSize, uint8_t *outData, size_t outSize);
    
    /*
     * @func 数据刷新,当最后不需要再写入数据的时候调用
     * @arg outData 输出数据 outSample 输出最大数据量
     * @return 返回处理数据量（单位sample）
     */
    //    int flush(uint8_t *outData, int outSize);
    
    /*
     * @func 释放效果处理类
     * @arg void
     * @return 0正常，其他异常
     */
    int release();
    
private:
    ImageParams inVideoParam;
    ImageParams outVideoParam;
    CropParam cropParam;
    int videoFmt;
    int cropYSize;
};
    
}

#endif /* CSVideoCropRotateEffect_h */
