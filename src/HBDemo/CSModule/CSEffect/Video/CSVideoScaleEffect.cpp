//
//  CSVideoScaleEffect.c
//  Sample
//
//  Created by zj-db0519 on 2017/11/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSVideoScaleEffect.h"

#include "libyuv.h"
#include "libyuv/scale.h"


static const char *scaleDesp = "Scale video";

namespace HBMedia {
    
CSVideoScaleEffect::CSVideoScaleEffect()
{
    memset(&scaleParam, 0, sizeof(ScaleParam_t));
}

CSVideoScaleEffect::~CSVideoScaleEffect()
{
    
}

/*
 * @func 获取效果处理介绍.
 * @arg void
 * @return 效果介绍字符串
 */
const char *CSVideoScaleEffect::getDescripe()
{
    return scaleDesp;
}

/*
 * @func 设置输入视频参数.
 * @arg param 输入视频参数
 * @return 0正常，其他异常
 */
int CSVideoScaleEffect::setInParam(ImageParams *param)
{
    if (param == NULL) {
        return AV_PARM_ERR;
    }
    //    int i;
    
    scaleParam.inWidth = param->mWidth;
    scaleParam.inHeight = param->mWidth;
    
    //    for (i=0; i<MAX_VIDEO_PLANE; i++) {
    //        invideoLineSize[i] = param->lineSize[i];
    //    }
    
    return 0;
}

/*
 * @func 设置输出视频参数.
 * @arg param 输出视频参数
 * @return 0正常，其他异常
 */
int CSVideoScaleEffect::setOutParam(ImageParams *param)
{
    if (param == NULL) {
        return AV_PARM_ERR;
    }
    
    scaleParam.outWidth = param->mWidth;
    scaleParam.outHeight = param->mHeight;
    videoOutSize = param->mPreImagePixBufferSize;
    
    
    return 0;
}

/*
 * @func 设置音频效果参数.
 * @arg param 音频效果参数
 * @return 0正常，其他异常
 */
int CSVideoScaleEffect::init()
{
    int lineSize;
    
    lineSize = scaleParam.outWidth;
    scaleYsize = abs(scaleParam.outHeight * scaleParam.outWidth);
    
    scaleParam.lineSize[0] = lineSize;
    scaleParam.lineSize[1] = lineSize >> 1;
    scaleParam.lineSize[2] = lineSize >> 1;
    
    lineSize = scaleParam.inWidth;
    
    invideoLineSize[0] = lineSize;
    invideoLineSize[1] = lineSize >> 1;
    invideoLineSize[2] = lineSize >> 1;
    
    inVideoYsize = abs(scaleParam.inHeight * scaleParam.inWidth);
    
    return 0;
}

size_t CSVideoScaleEffect::getBufSize()
{
    return videoOutSize;
}

/*
 * @func 数据转换.
 * @arg inData 输入数据 inSamples 输出的音频sample数 outData 输出数据 outSample 输出音频sample最大数，outSample 需要比输出的数据大
 * @return 返回处理数据量（单位sample）
 */
size_t CSVideoScaleEffect::transfer(uint8_t *inData, size_t inSize, uint8_t *outData, size_t outSize)
{
    uint8_t* pDstY = NULL;
    uint8_t* pDstU = NULL;
    uint8_t* pDstV = NULL;
    uint8_t* pOutDstY = NULL;
    uint8_t* pOutDstU = NULL;
    uint8_t* pOutDstV = NULL;
    int *outLineSize = NULL;
    int ret;
    
    outLineSize = scaleParam.lineSize;
    
    
    pDstY = inData;
    pDstU = inData + inVideoYsize;
    pDstV = pDstU + (inVideoYsize >> 2);
    
    pOutDstY = outData;
    pOutDstU = outData + scaleYsize;
    pOutDstV = pOutDstU + (scaleYsize >> 2);
    
    ret = libyuv::I420Scale(pDstY, invideoLineSize[0], pDstU, invideoLineSize[1], pDstV, invideoLineSize[2], scaleParam.inWidth, scaleParam.inHeight, pOutDstY, outLineSize[0], pOutDstU, outLineSize[1], pOutDstV, outLineSize[2], scaleParam.outWidth, scaleParam.outHeight, libyuv::kFilterNone);
    if (ret < 0) {
        return AV_TRANSFER_ERR;
    }
    
    return ret;
}

/*
 * @func 数据刷新,当最后不需要再写入数据的时候调用
 * @arg outData 输出数据 outSample 输出最大数据量
 * @return 返回处理数据量（单位sample）
 */
int CSVideoScaleEffect::flush(uint8_t *outData, int outSize)
{
    return 0;
}

/*
 * @func 释放效果处理类
 * @arg void
 * @return 0正常，其他异常
 */
int CSVideoScaleEffect::release()
{
    return 0;
}
    
}

