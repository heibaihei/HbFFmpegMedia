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

#include "CSDefine.h"
#include "CSLog.h"
#include "CSDefine.h"
#include "CSCommon.h"
#include "CSUtil.h"

namespace HBMedia {

class ThreadIPCContext {
public:
    /** 初始化临界区资源数 */
    ThreadIPCContext(int initNUm);
    
    ~ThreadIPCContext();
    
    /** 暂时无用 */
    int create();
    
    /** 生产资源 */
    int condP();
    
    /** 消费资源 */
    int condV();
    
    /** 释放信号量等资源 */
    int release();
    
private:
    /** 互斥信号量 */
    pthread_mutex_t condMux;
    pthread_cond_t cond;
    
    /** 临界区可用资源数 */
    int condCnt;
};

}
#endif /* ThreadIPCContext_hpp */
