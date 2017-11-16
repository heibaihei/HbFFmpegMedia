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
#include "CSDefine.h"
#include "CSThreadIPCContext.h"

namespace HBMedia {

typedef enum ThreadStat_t {
    THREAD_IDLE         = 0,
    THREAD_PREPARE      = 1,
    THREAD_RUNNING      = 2,
    THREAD_STOP         = 3,
    THREAD_DEAD         = 4,
    THREAD_FORCEQUIT    = 5,
} ThreadStat;

typedef void *(CALLBACK)(void *handle, int stat);

typedef struct ThreadParam_t {
    void *mThreadArgs;        /** 当前线程对应的媒体流参数 */
                              /** 正常对应: StreamThreadParam */
    ThreadStat mStatus;       /** 线程运行状态 */
    CALLBACK mThreadCB;       /** 线程要执行的回调接口 */
} ThreadParam_t;

typedef void *(*ThreadFunc)(void *arg);

class ThreadContext {
public:
    ThreadContext();
    ~ThreadContext();
    /** 设置线程要执行的函数以及运行需要伴随的参数 */
    int setFunction(ThreadFunc function, void *arg);
    
    /** 获取线程状态 */
    ThreadStat getThreadState();
    
    /** 绑定线程间通信机制 */
    int bindIPC(ThreadIPCContext *pv);
    
    /** 启动线程 */
    int start();
    
    /** 通知线程退出 */
    int abort();
    
    /** 通知线程停止执行 */
    int stop();
    
    int join();
    int markOver();
    
private:
    /** 线程要执行的线程代码块 */
    ThreadFunc mThreadFunc;
    
    /** 线程执行需要的参数, 指向对应的线程参数 */
    ThreadParam_t *mThreadArg;
    
    /** 线程ID */
    pthread_t mThreadID;
    
    ThreadIPCContext* mThreadIPC;
};

}
#endif /* ThreadContext_hpp */
