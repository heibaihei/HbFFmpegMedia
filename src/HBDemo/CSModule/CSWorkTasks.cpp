//
//  CSWorkTasks.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/12.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSWorkTasks.h"
#include "CSThreadContext.h"
#include "CSWorkContext.h"

namespace HBMedia {
    
void *CSWorkTasks::WorkTask_EncodeFrameRawData(void *arg) {
    ThreadParam_t* pThreadArgs = (ThreadParam_t *)arg;
    if (!pThreadArgs) {
        LOGE("Work task args is nullptr!");
        return nullptr;
    }
    
    StreamThreadParam* pStreamThreadParam = (StreamThreadParam*)(pThreadArgs->mThreadArgs);
    if (!pStreamThreadParam) {
        LOGE("Work task streams args is nullptr !");
        return nullptr;
    }
    
    int iStreamIndex = pStreamThreadParam->mStreamIndex;
    ThreadContext *pThreadCtx = pStreamThreadParam->mThreadCtx;
    
    
    
    
    return nullptr;
}

void *CSWorkTasks::WorkTask_WritePacketData(void *arg) {
    return nullptr;
}

}
