//
//  HBPacketQueue.c
//  FFmpeg
//
//  Created by zj-db0519 on 2017/8/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSPacketQueue.h"

// Make below global variable private.
static AVPacket flush_pkt;
static bool flush_pkt_init = false;

bool is_flush_pkt(AVPacket *pkt) {
    if (pkt == NULL) {
        return false;
    }
    
    return (pkt->data == flush_pkt.data);
}

/* packet queue handling */
void packet_queue_init(PacketQueue *q) {
    
    q->first_pkt = q->last_pkt = nullptr;
    q->nb_packets = 0;
    q->size = 0;
    q->abort_request = 0;
    q->serial = 0;
    
    q->mutex = std::make_shared<std::mutex>();
    q->cond = std::make_shared<std::condition_variable>();
    q->abort_request = 1;
    
    if (!flush_pkt_init) {
        av_init_packet(&flush_pkt);
        flush_pkt.data = (uint8_t *) &flush_pkt;
        flush_pkt_init = true;
    }
}

int packet_queue_put_private(PacketQueue *q, AVPacket *pkt) {
    HBAVPacketList *pkt1;
    
    if (q->abort_request)
        return -1;
    
    pkt1 = (HBAVPacketList *) av_malloc(sizeof(HBAVPacketList));
    if (!pkt1)
        return -1;
    pkt1->pkt = *pkt;
    pkt1->next = NULL;
    if (pkt == &flush_pkt)
        q->serial++;
    pkt1->serial = q->serial;
    
    if (!q->last_pkt)
        q->first_pkt = pkt1;
    else
        q->last_pkt->next = pkt1;
    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size + sizeof(*pkt1);
    /* XXX: should duplicate packet data in DV case */
    q->cond->notify_one();
    return 0;
}

int packet_queue_put_flush_pkt(PacketQueue *q) {
    return packet_queue_put(q, &flush_pkt);
}

int packet_queue_put(PacketQueue *q, AVPacket *pkt) {
    int ret;
    
    /* 将底层ffmpeg创建的pkt转换成外部独立缓冲区，以便外部开发人员可自行调用释放接口进行空间释放 */
    if (pkt != &flush_pkt && av_dup_packet(pkt) < 0)
        return -1;
    
    // 将包插入包队列
    {
        std::unique_lock<std::mutex> lck(*q->mutex);
        ret = packet_queue_put_private(q, pkt);
    }
    
    if (pkt != &flush_pkt && ret < 0)
        av_packet_unref(pkt);
    
    return ret;
}

int packet_queue_put_nullpacket(PacketQueue *q, int stream_index) {
    AVPacket pkt1, *pkt = &pkt1;
    av_init_packet(pkt);
    pkt->data = NULL;
    pkt->size = 0;
    pkt->stream_index = stream_index;
    return packet_queue_put(q, pkt);
}

/*
 * 清空包队列
 **/
void packet_queue_flush(PacketQueue *q) {
    HBAVPacketList *pkt, *pkt1;
    
    std::unique_lock<std::mutex> lck(*q->mutex);
    
    for (pkt = q->first_pkt; pkt != NULL; pkt = pkt1) {
        pkt1 = pkt->next;
        av_packet_unref(&pkt->pkt);
        av_freep(&pkt);
    }
    q->last_pkt = NULL;
    q->first_pkt = NULL;
    q->nb_packets = 0;
    q->size = 0;
    
}

void packet_queue_destroy(PacketQueue *q) {
    
    packet_queue_flush(q);
    
    q->mutex.reset();
    q->cond.reset();
}

void packet_queue_abort(PacketQueue *q) {
    
    std::unique_lock<std::mutex> lck(*q->mutex);
    
    q->abort_request = 1;
    
    q->cond->notify_one();
}

void packet_queue_start(PacketQueue *q) {
    std::unique_lock<std::mutex> lck(*q->mutex);
    
    q->abort_request = 0;
    packet_queue_put_private(q, &flush_pkt);
    
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.
 * 参数：block表示，当队列中没有数据包时，是否要阻塞等待包队列数据， 0表示不阻塞，1表示阻塞
 **/
int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial) {
    HBAVPacketList *pkt1 = NULL;
    int ret = 0;
    
    std::unique_lock<std::mutex> lck(*q->mutex);
    
    for (;;) {
        if (q->abort_request) {
            ret = -1;
            break;
        }
        
        pkt1 = q->first_pkt;
        if (pkt1) {
            // 队列头指针下移，检测尾指针是否还有数据，如果没有，则尾指针置空
            q->first_pkt = pkt1->next;
            if (!q->first_pkt)
                q->last_pkt = NULL;
            
            // 包个数减1，同时重新计算当前队列的大小
            q->nb_packets--;
            q->size -= pkt1->pkt.size + sizeof(*pkt1);
            *pkt = pkt1->pkt;
            if (serial)
                *serial = pkt1->serial;
            
            // 返回底层媒体包后，即可释放链表节点资源
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            // 不阻塞等待
            ret = 0;
            break;
        } else {
            // 阻塞等待，直到队列中有数据
            //			ALOGE("packet_queue_get block: no pkt");
            q->cond->wait(lck);
            //			ALOGE("packet_queue_get unblock");
        }
    }
    
    return ret;
}
