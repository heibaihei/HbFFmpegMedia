//
//  ThreadContext.cpp
//  MediaRecordDemo
//
//  Created by meitu on 2017/8/18.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSThreadContext.h"
#include "CSLog.h"

namespace HBMedia {

ThreadContext::ThreadContext() {
    mThreadArg = nullptr;
    mThreadID = nullptr;
    mThreadIPC = nullptr;
}

ThreadContext::~ThreadContext() {
    if (mThreadArg) {
        free(mThreadArg);
        mThreadArg = NULL;
    }
}

int ThreadContext::setFunction(ThreadFunc function, void *arg) {
    if (!mThreadArg) {
        mThreadArg = (ThreadParam_t *)malloc(sizeof(*mThreadArg));
        if (mThreadArg == NULL) {
            LOGE("Malloc thread arg failed !");
            return HB_ERROR;
        }
    }
    
    mThreadFunc = function;
    mThreadArg->mThreadArgs = arg;
    mThreadArg->mStatus = THREAD_IDLE;
    return HB_OK;
}

int ThreadContext::bindIPC(ThreadIPCContext *pv) {
    if (!pv) {
        LOGE("Bind thread IPC context failed !");
        return HB_ERROR;
    }
    mThreadIPC = pv;
    return HB_OK;
}

int ThreadContext::start() {
    int ret = pthread_create(&mThreadID, NULL, mThreadFunc, mThreadArg);
    if (ret < 0) {
        return HB_ERROR;
    }

    mThreadArg->mStatus = THREAD_RUNNING;
    return HB_OK;
}

int ThreadContext::abort() {
    mThreadArg->mStatus = THREAD_FORCEQUIT;
    return HB_OK;
}

ThreadStat ThreadContext::getThreadState() {
    if (mThreadArg == NULL) {
        return THREAD_FORCEQUIT;
    }
    return mThreadArg->mStatus;
}

int ThreadContext::stop() {
    mThreadArg->mStatus = THREAD_STOP;
    return HB_OK;
}

int ThreadContext::markOver() {
    mThreadArg->mStatus = THREAD_DEAD;
    return HB_OK;
}

int ThreadContext::join() {
    return pthread_join(mThreadID, NULL);
}

}
