#ifndef __HBLOG_H__
#define __HBLOG_H__

#include "HBSampleDefine.h"

enum{
    LOG_LEVEL_ALL     = 0,
    LOG_LEVEL_VERBOSE = 1,
    LOG_LEVEL_DEBUG   = 2,
    LOG_LEVEL_INFO    = 3,
    LOG_LEVEL_WARN    = 4,
    LOG_LEVEL_ERROR   = 5,
    LOG_LEVEL_FATAL   = 6,
    LOG_LEVEL_OFF     = 7,
};

extern int gMtmvLogLevel;

#define  MT_TARGET_LOG_LEVEL           (gMtmvLogLevel)

#define DEBUG_NATVIE

#ifdef DEBUG_NATVIE

#if MT_TARGET_PLATFORM == MT_PLATFORM_ANDROID

#include <android/log.h>
#define  LOG_TAG    "HB_FFMPEG"
#define  LOGV(...)  do { if (MT_TARGET_LOG_LEVEL <= LOG_LEVEL_VERBOSE) __android_log_print(ANDROID_LOG_VERBOSE,LOG_TAG,__VA_ARGS__); } while(0)
#define  LOGD(...)  do { if (MT_TARGET_LOG_LEVEL <= LOG_LEVEL_DEBUG) __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__); } while(0)
#define  LOGI(...)  do { if (MT_TARGET_LOG_LEVEL <= LOG_LEVEL_INFO) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__); } while(0)
#define  LOGW(...)  do { if (MT_TARGET_LOG_LEVEL <= LOG_LEVEL_WARN) __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__); } while(0)
#define  LOGE(...)  do { if (MT_TARGET_LOG_LEVEL <= LOG_LEVEL_ERROR) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__); } while(0)
#define  LOGF(...)  do { if (MT_TARGET_LOG_LEVEL <= LOG_LEVEL_FATAL) __android_log_print(ANDROID_LOG_FATAL,LOG_TAG,__VA_ARGS__); } while(0)

#else // !MT_PLATFORM_ANDROID

#if (MT_TARGET_PLATFORM == MT_PLATFORM_WIN32) || (MT_TARGET_PLATFORM == MT_PLATFORM_MAC) || (MT_TARGET_PLATFORM == MT_PLATFORM_LINUX) \
|| (MT_TARGET_PLATFORM == MT_PLATFORM_IOS)

#include <stdio.h>

#define  LOGV(...)  do { if(MT_TARGET_LOG_LEVEL <= LOG_LEVEL_VERBOSE) {printf("[Herb] VERBOSE:> ");printf(__VA_ARGS__);printf("\n");} } while(0)
#define  LOGD(...)  do { if(MT_TARGET_LOG_LEVEL <= LOG_LEVEL_DEBUG) {printf("<[Herb] DEBUG:> ");printf(__VA_ARGS__);printf("\n");} } while(0)
#define  LOGI(...)  do { if(MT_TARGET_LOG_LEVEL <= LOG_LEVEL_INFO) {printf("<[Herb] INFO:> ");printf(__VA_ARGS__);printf("\n");} } while(0)
#define  LOGW(...)  do { if(MT_TARGET_LOG_LEVEL <= LOG_LEVEL_WARN) {printf("<[Herb] WARN:> ");printf(__VA_ARGS__);printf("\n");} } while(0)
#define  LOGE(...)  do { if(MT_TARGET_LOG_LEVEL <= LOG_LEVEL_ERROR) {printf("<[Herb] ERROR:> ");printf(__VA_ARGS__);printf("\n");} } while(0)
#define  LOGF(...)  do { if(MT_TARGET_LOG_LEVEL <= LOG_LEVEL_FATAL) {printf("<[Herb] FATAL:> ");printf(__VA_ARGS__);printf("\n");} } while(0)
#define  MTTRACE(...) LOGI(__VA_ARGS__)

#endif // ! MT_PLATFORM_WIN32 MT_PLATFORM_MAC MT_PLATFORM_LINUX MT_PLATFORM_IOS

#endif // !MT_PLATFORM_ANDROID

#else // !DEBUG_NATVIE

#define  LOGV(...)
#define  LOGD(...)
#define  LOGI(...)
#define  LOGW(...)
#define  LOGE(...)
#define  LOGF(...)

#endif // !DEBUG_NATVIE


// log for MT_PLATFORM_ANDROID only
#if MT_TARGET_PLATFORM == MT_PLATFORM_ANDROID
#define  ANDROID_LOGV(...)  LOGV(__VA_ARGS__)
#define  ANDROID_LOGD(...)  LOGD(__VA_ARGS__)
#define  ANDROID_LOGI(...)  LOGI(__VA_ARGS__)
#define  ANDROID_LOGW(...)  LOGW(__VA_ARGS__)
#define  ANDROID_LOGE(...)  LOGE(__VA_ARGS__)
#define  ANDROID_LOGF(...)  LOGF(__VA_ARGS__)
#else // !MT_PLATFORM_ANDROID
#define  ANDROID_LOGV(...)
#define  ANDROID_LOGD(...)
#define  ANDROID_LOGI(...)
#define  ANDROID_LOGW(...)
#define  ANDROID_LOGE(...)
#define  ANDROID_LOGF(...)
#endif // !MT_PLATFORM_ANDROID


#endif
