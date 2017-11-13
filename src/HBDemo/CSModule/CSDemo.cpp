//
//  CSDemo.cpp
//  Sample
//
//  Created by zj-db0519 on 2017/11/13.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSDemo.h"
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>

#include "CSExternAPI.h"

#define SINGLE_SAMPLES  (1024)

void *audioReadThread(void *arg)
{
    int ret;
    FILE *pcmfp;
    const char *pcmFile = CS_MODULE_RESOURCE_ROOT_PATH"audio.pcm";
    bool startFlag = false;
    struct timeval start, end;
    uint8_t *decodedBuffer;
    int64_t audioTimestamp = 0;
    HBMedia::CSPlayer::CSPlayer *recordtest;
    
    recordtest = (HBMedia::CSPlayer::CSPlayer *)arg;
    
    LOGI("Audio read thread @@@@\n");
    
    pcmfp = fopen(pcmFile, "r");
    if (pcmfp == NULL) {
        LOGE("Open file %s error!\n", pcmFile);
        startFlag = false;
    } else {
        startFlag = true;
    }
    decodedBuffer = (uint8_t *)malloc(SINGLE_SAMPLES);
    if (decodedBuffer == NULL) {
        LOGE("Malloc buffer error!\n");
        goto TAR_OUT;
    }
    
    while (startFlag) {
        gettimeofday(&start, NULL);
        ret = fread(decodedBuffer, 1, (size_t)SINGLE_SAMPLES, pcmfp);
        if (ret <= 0) {
            LOGE("Read pcm file error!\n");
            break;
        }
        
        ret = recordtest->writeExternData(decodedBuffer, SINGLE_SAMPLES, 1, audioTimestamp);
        if (ret < 0) {
            LOGE("Audio Write data error![%d]\n", ret);
        }
        usleep(1500);
        gettimeofday(&end, NULL);
        //        _DEBUG_PRINT("Write audio data use time %ld us\n", 1000000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec));
        audioTimestamp += 15;
    }
    
TAR_OUT:
    
    if (decodedBuffer) {
        free(decodedBuffer);
    }
    
    if (pcmfp) {
        fclose(pcmfp);
    }
    
    return NULL;
}

void *videoReadThread(void *arg)
{
    int ret;
    int ySize;
    int inWidth = 960, inHeight = 540;
    long bufSize;
    FILE *yuvfp;
    int64_t videoTimestamp = 0;
    uint8_t *decodedBuffer;
    struct timeval start, end;
    HBMedia::CSPlayer::CSPlayer *recordtest;
    const char *yuvfile = CS_MODULE_RESOURCE_ROOT_PATH"yuv420p.yuv";
    bool startFlag = false;
    
    recordtest = (HBMedia::CSPlayer::CSPlayer *)arg;
    
    LOGI("video read thread @@@@\n");
    
    yuvfp = fopen(yuvfile, "r");
    if (yuvfp == NULL) {
        LOGE("Open file %s error!\n", yuvfile);
        startFlag = false;
    } else {
        startFlag = true;
    }
    
    ySize = inWidth * inHeight;
    //  huangcl 待定
//    bufSize = param.readinVideoBufSize();
    decodedBuffer = (uint8_t *)malloc(bufSize);
    if (decodedBuffer == NULL) {
        LOGE("Malloc decoded data error!\n");
        goto TAR_OUT;
    }
    
    while (startFlag) {
        gettimeofday(&start, NULL);
        ret = fread(decodedBuffer, 1, (size_t)bufSize, yuvfp);
        if (ret <= 0) {
            LOGE("Read yuv file error!\n");
            break;
        }
        usleep(2000);
        ret = recordtest->writeExternData(decodedBuffer, bufSize, 0, videoTimestamp);
        if (ret < 0) {
            LOGE("Video Write data error![%d]\n", ret);
        }
        gettimeofday(&end, NULL);
        //        _DEBUG_PRINT("Write video data use time %ld us\n", 1000000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec));
        videoTimestamp += 33.3;
    }
    
    LOGW("Video thread exit!\n");
    
TAR_OUT:
    
    if (decodedBuffer) {
        free(decodedBuffer);
    }
    
    if (yuvfp) {
        fclose(yuvfp);
    }
    
    return NULL;
}

