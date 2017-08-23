//
//  Queue.hpp
//  MediaRecordDemo
//
//  Created by meitu on 2017/8/19.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef Queue_hpp
#define Queue_hpp

#include <stdio.h>
extern "C"
{
#include "Kfifo.h"
}

typedef enum QueueStat_t {
    QUEUE_EMPTY     = 0,
    QUEUE_FILLED    = 1,
    QUEUE_OVERFLOW  = 2,
    QUEUE_INVAILD  = 3,
} QueueStat_t;

template <typename T>
class Queue {
public:
    Queue(uint32_t queueSize);
    ~Queue();

    int init();
    void release();
    bool push(T data);
    T get();
    uint32_t queueLen();
    int32_t left();
    void reset();
    int setQueueStat(QueueStat_t stat);
    QueueStat_t getQueueStat();
private:
    QueueStat_t stat;
    kfifo_t* _kfifo;
    uint32_t queueSize;
};


template <typename T>
Queue<T>::Queue(uint32_t size)
{
    _kfifo = NULL;
    stat = QUEUE_EMPTY;
    queueSize = size;
}

template <typename T>
Queue<T>::~Queue()
{
    
}

template <typename T>
int Queue<T>::init()
{
#if 1
    _kfifo = kfifo_alloc(queueSize * sizeof(T));
    if (_kfifo == NULL) {
        return -1;
    }
#endif
    
    return 0;
}

template <typename T>
void Queue<T>::release()
{
    if (_kfifo) {
        kfifo_free(_kfifo);
        _kfifo = NULL;
    }
}


template <typename T>
bool Queue<T>::push(T data)
{
    int len = 0;
    len = __kfifo_put(_kfifo, (const unsigned char *)&data, sizeof(T));
    if(len > 0) {
        stat = QUEUE_FILLED;
        return true;
    } else {
        stat = QUEUE_OVERFLOW;
        return false;
    }
}

template <typename T>
T Queue<T>::get()
{
    T data;
    int len = __kfifo_get(_kfifo, (unsigned char *)&data, sizeof(T));
    if(len > 0) {
        stat = QUEUE_FILLED;
        return data;
    } else {
        stat = QUEUE_OVERFLOW;
        return NULL;
    }
}

template <typename T>
int Queue<T>::setQueueStat(QueueStat_t queueStat)
{
    stat = queueStat;
    
    return 0;
}

template <typename T>
int32_t Queue<T>::left()
{
    return __kfifo_left(_kfifo) / sizeof(T);
}

template <typename T>
uint32_t Queue<T>::queueLen()
{
    return __kfifo_len(_kfifo) / sizeof(T);
}

template <typename T>
void Queue<T>::reset()
{
    __kfifo_reset(_kfifo);
    stat = QUEUE_EMPTY;
}

template <typename T>
QueueStat_t Queue<T>::getQueueStat()
{
    return stat;
}

#endif /* Queue_hpp */
