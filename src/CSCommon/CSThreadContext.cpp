//
//  ThreadContext.cpp
//  MediaRecordDemo
//
//  Created by meitu on 2017/8/18.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSThreadContext.h"
#include "CSLog.h"

ThreadContext::ThreadContext()
{
    threadArg = NULL;
}

ThreadContext::~ThreadContext()
{
    if (threadArg) {
        free(threadArg);
        threadArg = NULL;
    }
}

int ThreadContext::setFunction(func function, void *arg)
{
    threadArg = (ThreadParam_t *)malloc(sizeof(*threadArg));
    if (threadArg == NULL) {
        return HB_ERROR;
    }
    
    threadFunc = function;
    threadArg->arg = arg;
    threadArg->stat = THREAD_IDLE;
    
    return 0;
}

int ThreadContext::start()
{
    int ret;
    
    ret = pthread_create(&thread, NULL, threadFunc, threadArg);
    if (ret < 0) {
        return ret;
    }

    threadArg->stat = THREAD_RUNNING;
    return 0;
}

int ThreadContext::abort()
{
    threadArg->stat = THREAD_FORCEQUIT;
    
    return 0;
}

int ThreadContext::getThreadState()
{
    if (threadArg == NULL) {
        return HB_ERROR;
    }
    
    return threadArg->stat;
}

int ThreadContext::stop()
{
//    if (threadArg) {
//        free(threadArg);
//        threadArg = NULL;
//    }
    threadArg->stat = THREAD_STOP;
    
    return 0;
}

int ThreadContext::markOver()
{
    threadArg->stat = THREAD_DEAD;
    
    return 0;
}

int ThreadContext::join()
{
//    if (thread <= 0) {
//        return AV_STAT_ERR;
//    }
    return pthread_join(thread, NULL);
}
