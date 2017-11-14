//
//  CSVideoCropRotateEffect.c
//  Sample
//
//  Created by zj-db0519 on 2017/11/14.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSVideoCropRotateEffect.h"
#include "libyuv.h"

static const char *cropRotateDesp = "crop and rotate video";

namespace HBMedia {
    
CSVideoCropRotateEffect::CSVideoCropRotateEffect()
{
    
    
}

CSVideoCropRotateEffect::~CSVideoCropRotateEffect()
{
    
}
/*
 * @func 获取效果处理介绍.
 * @arg void
 * @return 效果介绍字符串
 */
const char *CSVideoCropRotateEffect::getDescripe()
{
    return cropRotateDesp;
}

/*
 * @func 设置输入视频参数.
 * @arg param 输入视频参数
 * @return 0正常，其他异常
 */
int CSVideoCropRotateEffect::setInParam(ImageParams *param)
{
    int ret = 0;
    
    if (param == NULL) {
        return AV_PARM_ERR;
    }
    
    inVideoParam.mWidth = param->mWidth;
    inVideoParam.mHeight = param->mHeight;
    inVideoParam.mPixFmt = param->mPixFmt;
    inVideoParam.mDataSize = param->mDataSize;
    
    switch (param->mPixFmt) {
        case CS_PIX_FMT_NV21:
            videoFmt = libyuv::FOURCC_NV21;
            break;
        case CS_PIX_FMT_YUV420P:
            videoFmt = libyuv::FOURCC_I420;
            break;
        case CS_PIX_FMT_YUV422P:
            videoFmt = libyuv::FOURCC_I422;
            break;
        case CS_PIX_FMT_YUV444P:
            videoFmt = libyuv::FOURCC_I444;
            break;
            
        default:
            videoFmt = -1;
            ret = AV_NOT_FOUND;
            break;
    }
    
    return 0;
}

/*
 * @func 设置输出视频参数.
 * @arg param 输出视频参数
 * @return 0正常，其他异常
 */
int CSVideoCropRotateEffect::setOutParam(ImageParams *param)
{
    if (param == NULL) {
        return AV_PARM_ERR;
    }
    
    outVideoParam.mWidth = param->mWidth;
    outVideoParam.mHeight = param->mHeight;
    outVideoParam.mPixFmt = param->mPixFmt;
    outVideoParam.mDataSize = param->mDataSize;
    outVideoParam.mRotate = param->mRotate;
    
    return 0;
}

/*
 * @func 设置视频效果参数.
 * @arg param 视频效果参数
 * @return 0正常，其他异常
 */
int CSVideoCropRotateEffect::setEffectParam(VideoEffectParam *param)
{
    int i;
    CropParam *inCrop;
    if (param == NULL || param->cropParam == NULL) {
        return AV_PARM_ERR;
    }
    
    inCrop = param->cropParam;
    
    cropParam.posX = inCrop->posX;
    cropParam.posY = inCrop->posY;
    cropParam.cropWidth = inCrop->cropWidth;
    cropParam.cropHeight = inCrop->cropHeight;
    for (i=0; i<MAX_VIDEO_PLANE; i++) {
        if (inCrop->lineSize[i] > 0) {
            cropParam.lineSize[i] = inCrop->lineSize[i];
        } else {
            if (i == 0) {
                cropParam.lineSize[i] = inCrop->cropWidth;
            } else {
                cropParam.lineSize[i] = inCrop->cropWidth >> 1;
            }
        }
    }
    
    return 0;
}

/*
 * @func 初始化处理参数.
 * @arg void
 * @return 0正常，其他异常
 */
int CSVideoCropRotateEffect::init()
{
    cropYSize = cropParam.cropWidth * cropParam.cropHeight;
    
    return 0;
}


size_t CSVideoCropRotateEffect::getBufSize()
{
    return outVideoParam.mDataSize;
}

/*
 * @func 数据转换.
 * @arg inData 输入数据 inSize 输出的视频数据大小 outData 输出数据 outSample 输出视频outSize最大数
 * @return 返回处理数据量
 */
size_t CSVideoCropRotateEffect::transfer(uint8_t *inData, size_t inSize, uint8_t *outData, size_t outSize)
{
    int ret;
    
    uint8_t* pDstY = NULL;
    uint8_t* pDstU = NULL;
    uint8_t* pDstV = NULL;
    int *lineSize;
    
    pDstY = outData;
    pDstU = outData + cropYSize;
    pDstV = pDstU + (cropYSize>>2);
    
    lineSize = cropParam.lineSize;
#ifndef LIBYUV_MODULE_EXCLUDE
    ret = libyuv::ConvertToI420(inData, inSize, \
                                pDstY, lineSize[0], \
                                pDstU, lineSize[1], \
                                pDstV, lineSize[2], \
                                cropParam.posX, cropParam.posY, \
                                inVideoParam.mWidth, \
                                inVideoParam.mHeight, \
                                cropParam.cropWidth, cropParam.cropHeight, \
                                (libyuv::RotationModeEnum)outVideoParam.mRotate, \
                                videoFmt);
#endif
    if (ret < 0) {
        return AV_TRANSFER_ERR;
    }
    
    ret = outVideoParam.mDataSize;
    
    return ret;
}

/*
 * @func 释放效果处理类
 * @arg void
 * @return 0正常，其他异常
 */
int CSVideoCropRotateEffect::release()
{
    return 0;
}
    
}

