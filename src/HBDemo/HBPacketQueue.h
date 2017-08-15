//
//  HBPacketQueue.h
//  FFmpeg
//
//  Created by zj-db0519 on 2017/8/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef HBPacketQueue_h
#define HBPacketQueue_h

#include <condition_variable>
#include <mutex>
#include "HBAudio.h"

#define QUEUE_BLOCK_WAIT   1   /** 队列中无数据，阻塞等待直到得到数据 */
#define QUEUE_NOT_BLOCK    0   /** 队列中无数据，则马上返回 */

/** 定义最大队列大小 */
#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_FRAMES 5

typedef struct HBAVPacketList {
    AVPacket pkt;                        /* 表示当前要解码的原始媒体数据包 */
    struct HBAVPacketList *next;
    int serial;                          /* 当前要解码数据包的序列号 */
} HBAVPacketList;

typedef struct PacketQueue {
    HBAVPacketList *first_pkt, *last_pkt;/* first_pkt表示当前队列的第一个包，last_pkt表示当前队列的最后一个包 */
    int nb_packets;                      /* 当前包队列包的个数 */
    int size;
    int abort_request;
    int serial;
    
    std::shared_ptr<std::mutex> mutex;
    std::shared_ptr<std::condition_variable> cond;
} PacketQueue;

bool is_flush_pkt(AVPacket *pkt);
void packet_queue_start(PacketQueue *q);
void packet_queue_abort(PacketQueue *q);
void packet_queue_destroy(PacketQueue *q);
int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial);
void packet_queue_flush(PacketQueue *q);
int packet_queue_put_nullpacket(PacketQueue *q, int stream_index);
int packet_queue_put(PacketQueue *q, AVPacket *pkt);
int packet_queue_put_private(PacketQueue *q, AVPacket *pkt);
void packet_queue_init(PacketQueue *q);
int packet_queue_put_flush_pkt(PacketQueue *q);

#endif /* HBPacketQueue_h */
