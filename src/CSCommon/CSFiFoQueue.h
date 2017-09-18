//
//  Queue.h
//  MediaRecordDemo
//
//  Created by meitu on 2017/8/19.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSQueue_h
#define CSQueue_h

#ifdef __cplusplus
extern "C" {
#endif

#include "Kfifo.h"

#ifdef __cplusplus
};
#endif

typedef enum QueueStat_t {
    QUEUE_EMPTY     = 0,
    QUEUE_FILLED    = 1,
    QUEUE_OVERFLOW  = 2,
    QUEUE_INVAILD  = 3,
} QueueStat_t;

template <typename T>
class FiFoQueue {
public:
    /**
     *  队列构造函数: 构造时，传递内部节点个数
     */
    FiFoQueue(uint32_t queueSize);
    ~FiFoQueue();
    
    /** 往队列中插入类型为 T 的数据节点 */
    int push(T data);
    
    /** 从队列中取出数据，先入先出 */
    T get();
    
    /** 获取当前队列中存储的节点个数 */
    uint32_t queueLength();
    
    /** 获取当前队列中空闲的节点个数 */
    int32_t queueLeft();
    
    /** 重置队列中数据  */
    void reset();
    
    /** 设置队列状态 */
    int setQueueStat(QueueStat_t stat);
    QueueStat_t getQueueStat();

protected:
    /** 创建 & 释放 queue 中内部使用的对象空间 */
    int init();
    void release();
    
private:
    /** 队列的状态 */
    QueueStat_t mStatus;
    
    /** 内部真实操作队列对象 */
    kfifo_t* mKfifo;
    
    /** 队列中节点的个数 */
    uint32_t mQueueSize;
};


template <typename T>
FiFoQueue<T>::FiFoQueue(uint32_t size) {
    mKfifo = NULL;
    mStatus = QUEUE_EMPTY;
    mQueueSize = size;
    
    init();
}

template <typename T>
FiFoQueue<T>::~FiFoQueue() {
    release();
}

template <typename T>
int FiFoQueue<T>::init() {
    mKfifo = kfifo_alloc(mQueueSize * sizeof(T));
    if (mKfifo == NULL) {
        return -1;
    }
    
    return 0;
}

template <typename T>
void FiFoQueue<T>::release() {
    if (mKfifo) {
        kfifo_free(mKfifo);
        mKfifo = NULL;
    }
}

template <typename T>
int FiFoQueue<T>::push(T data) {
    int len = 0;
    len = __kfifo_put(mKfifo, (const unsigned char *)&data, sizeof(T));
    if(len > 0) {
        mStatus = QUEUE_FILLED;
    } else {
        mStatus = QUEUE_OVERFLOW;
    }
    return len;
}

template <typename T>
T FiFoQueue<T>::get() {
    T data;
    int len = __kfifo_get(mKfifo, (unsigned char *)&data, sizeof(T));
    if(len > 0) {
        mStatus = QUEUE_FILLED;
        return data;
    } else {
        mStatus = QUEUE_OVERFLOW;
        return NULL;
    }
}

template <typename T>
int FiFoQueue<T>::setQueueStat(QueueStat_t queueStat) {
    mStatus = queueStat;
    return 0;
}

template <typename T>
int32_t FiFoQueue<T>::queueLeft() {
    return __kfifo_left(mKfifo) / sizeof(T);
}

template <typename T>
uint32_t FiFoQueue<T>::queueLength() {
    return __kfifo_len(mKfifo) / sizeof(T);
}

template <typename T>
void FiFoQueue<T>::reset() {
    __kfifo_reset(mKfifo);
    mStatus = QUEUE_EMPTY;
}

template <typename T>
QueueStat_t FiFoQueue<T>::getQueueStat() {
    return mStatus;
}

#endif /* Queue_hpp */
