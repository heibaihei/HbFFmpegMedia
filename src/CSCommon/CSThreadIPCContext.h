//
//  ThreadIPCContext.hpp
//  MediaRecordDemo
//
//  Created by meitu on 2017/8/18.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef ThreadIPCContext_hpp
#define ThreadIPCContext_hpp

#include <pthread.h>
#include <unistd.h>

#include <stdio.h>

class ThreadIPCContext {
public:
    ThreadIPCContext(int initNUm);
    ~ThreadIPCContext();
    int create();
    int condP();
    int condV();
    int release();
    
private:
    pthread_mutex_t condMux;
    pthread_cond_t cond;
    int condCnt;
};

#endif /* ThreadIPCContext_hpp */
