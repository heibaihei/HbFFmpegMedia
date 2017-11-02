#ifndef __HB_DEMO_COMMON_H__
#define __HB_DEMO_COMMON_H__

#define INVALID_STREAM_INDEX  (-1)

#define SAFE_DELETE(p)           do { if(p) { delete (p); (p) = nullptr;} } while(0)
#define SAFE_DELETE_ARRAY(p)     do { if(p) { delete[] (p); (p) = nullptr; } } while(0)
#define SAFE_FREE(p)             do { if(p) { free(p); (p) = nullptr; } } while(0)

#include "CSDefine.h"
#include "CSUtil.h"
#include "CSLog.h"
#include "HBPickPicture.h"
#include "CSPacketQueue.h"

/**
 *  获取当前时间
 */
double mt_gettime_monotonic();

/**
 *  全局性的注册，主要是ffmpeg 一些注册接口等全局变量的初始化
 *  @return HB_OK 正常初始化;  HB_ERROR 初始化异常
 */
int globalInitial();

/**
 *  图像参数初始化
 *  @param param 指向待初始化图像参数对象的指针
 *  @return HB_ERROR 初始化失败， HB_OK 初始化成功;
 */
int imageParamInit(ImageParams* param);

/**
 *  将 IMAGE_PIX_FORMAT 格式转换成 ffmpeg 内部的图像格式，实现对封装格式的抽象封装
 *
 */
enum AVPixelFormat getImageInnerFormat(IMAGE_PIX_FORMAT pixFormat);

enum IMAGE_PIX_FORMAT getImageExternFormat(AVPixelFormat pixFormat);
/**
 *  获取像素素材描述信息
 */
char* getImagePixFmtDescript(IMAGE_PIX_FORMAT dataType);

/**
 *  获取媒体类型的描述信息
 */
char* getMediaDataTypeDescript(MEDIA_DATA_TYPE dataType);
#endif
