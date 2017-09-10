//
//  ThreadContext.hpp
//  MediaRecordDemo
//
//  Created by meitu on 2017/8/18.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef ThreadContext_hpp
#define ThreadContext_hpp

#include <stdio.h>
#include "CSThreadIPCContext.h"

enum ThreadStat_t {
    THREAD_IDLE         = 0,
    THREAD_PREPARE      = 1,
    THREAD_RUNNING      = 2,
    THREAD_STOP         = 3,
    THREAD_DEAD         = 4,
    THREAD_FORCEQUIT    = 5,
};

typedef void *(callback)(void *handle, int stat);

struct ThreadParam_t {
    void *arg;
    enum ThreadStat_t stat;
    callback threadcb;
};

typedef void *(*func)(void *arg);

class ThreadContext {
public:
    ThreadContext();
    ~ThreadContext();
    int setFunction(func function, void *arg);
    int getThreadState();
    int bindIPC(ThreadIPCContext *pv);
    int start();
    int abort();
    int stop();
    int join();
    int markOver();
    
private:
    func threadFunc;
    ThreadParam_t *threadArg;
    pthread_t thread;
};

#endif /* ThreadContext_hpp */
