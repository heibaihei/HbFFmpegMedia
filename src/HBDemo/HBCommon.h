#ifndef __HB_DEMO_COMMON_H__
#define __HB_DEMO_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif
    
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libavutil/samplefmt.h"
#include "libavutil/error.h"
#include "libavutil/mem.h"
#include "libavutil/audio_fifo.h"
    
#ifdef __cplusplus
};
#endif

#define INVALID_STREAM_INDEX  (-1)

#define SAFE_DELETE(p)           do { if(p) { delete (p); (p) = nullptr;} } while(0)
#define SAFE_DELETE_ARRAY(p)     do { if(p) { delete[] (p); (p) = nullptr; } } while(0)
#define SAFE_FREE(p)             do { if(p) { free(p); (p) = nullptr; } } while(0)

#ifndef makeErrorStr
static char errorStr[AV_ERROR_MAX_STRING_SIZE];
#define makeErrorStr(errorCode) av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, errorCode)
#endif

#include "HBSampleDefine.h"
#include "HBLog.h"
#include "HBPickPicture.h"

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
