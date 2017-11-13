//
//  ThreadIPCContext.cpp
//  MediaRecordDemo
//
//  Created by meitu on 2017/8/18.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSThreadIPCContext.h"

namespace HBMedia {

ThreadIPCContext::ThreadIPCContext(int initNUm) {
    condCnt = initNUm;
    pthread_cond_init(&cond, NULL);
    pthread_mutex_init(&condMux, NULL);
}

ThreadIPCContext::~ThreadIPCContext() {
}

int ThreadIPCContext::create() {
    return HB_OK;
}

int ThreadIPCContext::condP() {
    pthread_mutex_lock(&condMux);
    condCnt++;
    pthread_mutex_unlock(&condMux);
    pthread_cond_signal(&cond);
    
    return HB_OK;
}

int ThreadIPCContext::condV() {
    pthread_mutex_lock(&condMux);
    if (condCnt <= 0) {
        pthread_cond_wait(&cond, &condMux);
    }
    condCnt--;
    pthread_mutex_unlock(&condMux);
    
    return HB_OK;
}

int ThreadIPCContext::release() {
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&condMux);
    
    condCnt = -1;
    
    return HB_OK;
}

}
