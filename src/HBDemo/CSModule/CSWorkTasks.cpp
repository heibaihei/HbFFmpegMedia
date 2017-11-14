//
//  CSWorkTasks.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/12.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSWorkTasks.h"
#include "CSThreadContext.h"
#include "CSWorkContext.h"
#include <sys/time.h>

namespace HBMedia {

bool _CheckIsExitThread(StreamThreadParam **streamThreadParamsArry, int Size);

void *CSWorkTasks::WorkTask_EncodeFrameRawData(void *arg) {
    ThreadParam_t* pThreadArgs = (ThreadParam_t *)arg;
    if (!pThreadArgs) {
        LOGE("Work task args is nullptr!");
        return nullptr;
    }
    
    StreamThreadParam* pStreamThreadParam = (StreamThreadParam*)(pThreadArgs->mThreadArgs);
    if (!pStreamThreadParam) {
        LOGE("Work task streams args is nullptr !");
        return nullptr;
    }
    
    bool bPushRawDataWithSyncMode = true;
    int iStreamIndex = pStreamThreadParam->mStreamIndex;
    ThreadContext *pThreadCtx = pStreamThreadParam->mThreadCtx;
    AVCodecContext* pCodecCtx = pStreamThreadParam->mCodecCtx;
    ThreadIPCContext* pEncodeIpcCtx = pStreamThreadParam->mEncodeIPC;
    ThreadIPCContext *pQueueIpcCtx = pStreamThreadParam->mQueueIPC;
    ThreadIPCContext* pWriteIpcCtx = pStreamThreadParam->mWriteIPC;
    FiFoQueue<AVFrame *> *pFrameQueue = pStreamThreadParam->mFrameQueue;
    FiFoQueue<AVFrame *> *pFrameRecycleQueue = pStreamThreadParam->mFrameRecycleQueue;
    FiFoQueue<AVPacket *> *pPacketQueue = pStreamThreadParam->mPacketQueue;
    FiFoQueue<AVPacket *> *pPacketRecycleQueue = pStreamThreadParam->mPacketRecycleQueue;
    
    if (!pCodecCtx || !pEncodeIpcCtx || !pWriteIpcCtx \
        || !pFrameQueue || !pFrameRecycleQueue || !pPacketQueue || !pPacketRecycleQueue) {
        LOGE("Work task get thread param failed !");
        return NULL;
    }
    
    LOGI("Work task with stream<%d>", iStreamIndex);

    int HBErr = HB_OK;
    int iFrameQueueLen = 0;
    int iDropFrameCnt = 0;
    AVFrame* pTargetFrame = nullptr;
    AVPacket* pPacket = nullptr;
    ThreadStat eThreadState = THREAD_IDLE;
    struct timeval start, end;
    while (true) {
        eThreadState = pThreadCtx->getThreadState();
        if (eThreadState == THREAD_FORCEQUIT) {
            pFrameQueue ->setQueueStat(QUEUE_INVAILD);
            if (clearFrameQueue(pFrameQueue) != HB_OK)
                LOGE("Work task clear frame queue error !");
            break;
        }
        
        iFrameQueueLen = pFrameQueue->queueLength();
        if (iFrameQueueLen > 0) {
            int queueleft = pFrameQueue->queueLeft();
            pTargetFrame = pFrameQueue->get();
            if (pTargetFrame == NULL) {
                LOGV("Get frame error!\n");
                continue;
            }
            
            if ((pFrameQueue->getQueueStat() == QUEUE_OVERFLOW) \
                || (queueleft == 0)) {
                if (bPushRawDataWithSyncMode) {
                    pQueueIpcCtx->condP();
                }
                else {
                    iDropFrameCnt++;
                    HBErr = pFrameRecycleQueue->push(pTargetFrame);
                    if (HBErr <= 0) {
                        av_frame_free(&pTargetFrame);
                        LOGE("Work task frame recycle queue push frame failed !");
                    }
                    continue;
                }
            }
        }
        else {
            if (eThreadState >= THREAD_STOP) {
                LOGE("Work task thread quit request !");
                break;
            }
            pQueueIpcCtx->condP();
            pEncodeIpcCtx->condV();
            continue;
        }
        
        gettimeofday(&start, NULL);
//        if (NULL != (pTargetFrame = pFrameQueue->get())) {
//            LOGE("Frame queue get frame failed !");
//            continue;
//        }
        
        HBErr = avcodec_send_frame(pCodecCtx, pTargetFrame);
        if (HBErr < 0) {
            pPacketRecycleQueue->push(pPacket);
            LOGE("Work task send data co codec context failed !");
            break;
        }
        
        if ((pPacket = av_packet_alloc()) == nullptr) {
            LOGE("Work task alloc packet failed !");
            break;
        }
        
        HBErr = avcodec_receive_packet(pCodecCtx, pPacket);
        if (HBErr == AVERROR(EAGAIN)) {
            pPacketRecycleQueue->push(pPacket);
            LOGE("Work task buffer not enougt, again");
            continue;
        }
        else if (HBErr < 0) {
            LOGE("Work task Enode data error ! <%s>", makeErrorStr(HBErr));
            break;
        }
        
        pStreamThreadParam->mMinPacketPTS = av_rescale_q(pTargetFrame->pts, pStreamThreadParam->mTimeBase, AV_TIME_BASE_Q);
        LOGI("Work task stream-%d, frame queue length:%d, left:%d", iStreamIndex,  pFrameQueue->queueLength(),pFrameQueue->queueLeft());
        
        HBErr = pFrameQueue->push(pTargetFrame);
        if (HBErr <= 0)
            LOGW("Work task push frame to frame queue failed !");
        
        pPacket->stream_index = iStreamIndex;
        gettimeofday(&end, NULL);
        
        HBErr = pPacketQueue->push(pPacket);
        if (HBErr < 0)
            LOGE("Work tas");
        
        pWriteIpcCtx->condP();
    }
    
    HBErr = avcodec_send_frame(pCodecCtx, NULL);
    if (HBErr < 0)
        LOGE("Work task send data to codec failed !");
    
    while (HBErr >= 0) {
        if (!pPacketRecycleQueue || (pPacket = pPacketRecycleQueue->get()) == nullptr) {
            if ((pPacket = av_packet_alloc()) == nullptr) {
                LOGE("Work task allock packet error !");
                break;
            }
        }
        
        av_init_packet(pPacket);
        pPacket->data = nullptr;
        pPacket->size = 0;
        HBErr = avcodec_receive_packet(pCodecCtx, pPacket);
        if (HBErr < 0) {
            pPacketRecycleQueue->push(pPacket);
            LOGE("Work task encode error !");
            break;
        }
        
        pPacket->stream_index = iStreamIndex;
        LOGE("Work task push a packet to write queue !");
        pPacketQueue->push(pPacket);
        pWriteIpcCtx->condP();
    }
    
    pThreadCtx->markOver();
    pWriteIpcCtx->condP();
    
    LOGW("[%d]Work task Encode thread exit !\n", iStreamIndex);
    return nullptr;
}

void *CSWorkTasks::WorkTask_WritePacketData(void *arg) {
    if (!arg) {
        LOGE("Work task write packet thread args invalid !");
        return NULL;
    }
    
    ThreadParam_t *pThreadParams = (ThreadParam_t *)arg;
    WorkContextParam *pWorkContextParam = (WorkContextParam *)(pThreadParams->mThreadArgs);
    AVFormatContext *pOutFmt = pWorkContextParam->mTargetFormatCtx;
    ThreadIPCContext *pWriteIpcCtx = pWorkContextParam->mWorkIPCCtx;
    ThreadContext *pThreadCtx = pWorkContextParam->mWorkThread;
    
    if (!pWorkContextParam || !pOutFmt || !pWriteIpcCtx || !pThreadCtx) {
        LOGE("Work task write params failed !");
        return nullptr;
    }
    
    int iStreamParamsQueueSize = (int)pWorkContextParam->mStreamPthreadParamList.size();
    if (iStreamParamsQueueSize <= 0) {
        LOGE("Work task stream params size invalid !");
        return nullptr;
    }
    
    StreamThreadParam **streamThreadArrys = new StreamThreadParam*[iStreamParamsQueueSize];
    FiFoQueue<AVPacket *> **packetQueueArray = new FiFoQueue<AVPacket *> *[iStreamParamsQueueSize];
    FiFoQueue<AVPacket *> **packetRecyleQueueArray = new FiFoQueue<AVPacket *> *[iStreamParamsQueueSize];
    if (!streamThreadArrys || !packetQueueArray || !packetRecyleQueueArray) {
        LOGE("Work task allock run context failed ! <StreamThreadParam:%p> <Packet:%p> <Packet:%p>", streamThreadArrys, packetQueueArray, packetRecyleQueueArray);
        return nullptr;
    }
    for (int i=0; i<iStreamParamsQueueSize; i++) {
        streamThreadArrys[i] = pWorkContextParam->mStreamPthreadParamList[i];
        packetQueueArray[i] = streamThreadArrys[i]->mPacketQueue;
        packetRecyleQueueArray[i] = streamThreadArrys[i]->mPacketRecycleQueue;
    }
    
    bool bIsWrite = false;
    int HBErr = HB_OK, iBestPacketIndex = 0;
    int64_t iMinPTS = INT64_MAX, iQueueLastPTS=0;
    ThreadStat eThreadState = THREAD_IDLE;
    AVPacket *pPacket = nullptr;
    long iStreamsPacketCnt[8] = {0};
    while (true) {
        eThreadState = pThreadCtx->getThreadState();
        if (eThreadState == THREAD_FORCEQUIT) {
            for (int i=0; i < iStreamParamsQueueSize; i++) {
                HBErr = clearPacketQueue(packetQueueArray[i]);
                if (HBErr < 0)
                    LOGE("Work stask clear packet queue failed !");
            }
            break;
        }
        
        bIsWrite = false;
        iMinPTS = INT64_MAX;
        for (int i=0; i<iStreamParamsQueueSize; i++) {
            if (packetQueueArray[i]->queueLength() > 0) {
                bIsWrite = true;
                iQueueLastPTS = streamThreadArrys[i]->mMinPacketPTS;
                if (iMinPTS > iQueueLastPTS) {
                    iBestPacketIndex = i;
                    iMinPTS = iQueueLastPTS;
                }
            }
        }
        
        if (_CheckIsExitThread(streamThreadArrys, iStreamParamsQueueSize) \
            && eThreadState == THREAD_STOP && !bIsWrite) {
            LOGW("Work task write thread exit !");
            break;
        }
        else if (bIsWrite == false) {
            if (_CheckIsExitThread(streamThreadArrys, iStreamParamsQueueSize))
                LOGW("Work task encode thread all exit");
            pWriteIpcCtx->condV();
            continue;
        }
        
        HBErr = updateQueue(streamThreadArrys[iBestPacketIndex]);
        if (HBErr != HB_OK) {
            LOGE("Work task update queue failed !");
            continue;
        }
        
        pPacket = streamThreadArrys[iBestPacketIndex]->mBufferPacket;
        if (!pPacket) {
            LOGE("Work task buffer packet is null !");
            continue;
        }
        iStreamsPacketCnt[pPacket->stream_index]++;
        HBErr = av_interleaved_write_frame(pOutFmt, pPacket);
        if (HBErr < 0) {
//            av_packet_unref(pPacket);
            packetRecyleQueueArray[iBestPacketIndex]->push(pPacket);
            LOGE("Work task write data to file error ! %s", makeErrorStr(HBErr));
            break;
        }
        av_packet_unref(pPacket);
        packetRecyleQueueArray[iBestPacketIndex]->push(pPacket);
    }
    
    if (packetQueueArray)
        delete []packetQueueArray;
    if (packetRecyleQueueArray)
        delete []packetRecyleQueueArray;
    if (streamThreadArrys)
        delete []streamThreadArrys;
    
    LOGE("Work task write thread exit !");
    return nullptr;
}

bool _CheckIsExitThread(StreamThreadParam **streamThreadParamsArry, int Size)
{
    ThreadContext *pThreadCtx = nullptr;
    for (int i=0; i<Size; i++) {
        pThreadCtx = streamThreadParamsArry[i]->mThreadCtx;
        if (pThreadCtx->getThreadState() != THREAD_DEAD) {
            return false;
        } else
            LOGI("Work stask Check status: %d", i);
    }
    
    return true;
}
}
