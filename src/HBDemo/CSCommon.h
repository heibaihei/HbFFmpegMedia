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

#endif
