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
    ThreadIPCContext* pEncodeIpcCtx = pStreamThreadParam->mEncodeIPC;
    ThreadIPCContext *pQueueIpcCtx = pStreamThreadParam->mQueueIPC;
    ThreadIPCContext* pWriteIpcCtx = pStreamThreadParam->mWriteIPC;
    
    AVCodecContext* pCodecCtx = pStreamThreadParam->mCodecCtx;
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
            pFrameQueue->setQueueStat(QUEUE_INVAILD);
            if (clearFrameQueue(pFrameQueue) != HB_OK)
                LOGE("Work task clear frame queue error !");
            LOGE("Work task (encode) force quit !");
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
            
            if ((pFrameQueue->getQueueStat() == QUEUE_OVERFLOW) || (queueleft == 0)) {
                if (bPushRawDataWithSyncMode) {
                    pQueueIpcCtx->condP();
                    LOGI("Work task (encode) [%d]queueStat : %d queueleft %d\n", \
                         iStreamIndex, pFrameQueue->getQueueStat(), pFrameQueue->queueLeft());
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

        HBErr = avcodec_send_frame(pCodecCtx, pTargetFrame);
        if (HBErr < 0) {
            LOGE("Work task send data co codec context failed, %s !", av_err2str(HBErr));
            HBErr = pPacketRecycleQueue->push(pPacket);
            if (HBErr <= 0)
                av_packet_free(&pPacket);
            break;
        }
        
        if (!(pPacketRecycleQueue && (pPacket = pPacketRecycleQueue->get()))) {
            if ((pPacket = av_packet_alloc()) == nullptr) {
                LOGE("Work task alloc packet failed !");
                break;
            }
        }
        
        HBErr = avcodec_receive_packet(pCodecCtx, pPacket);
        if (HBErr == AVERROR(EAGAIN)) {
            LOGE("Work task buffer not enougt, again, %s", av_err2str(HBErr));
            if (pPacketRecycleQueue->push(pPacket) <= 0)
                av_packet_free(&pPacket);
            continue;
        }
        else if (HBErr < 0) {
            LOGE("Work task Enode data error ! <%s>", makeErrorStr(HBErr));
            break;
        }
        
        /** 得到编码好的数据包，计算当前得到的数据包的: PTS 时间 */
        pStreamThreadParam->mMinPacketPTS = av_rescale_q(pTargetFrame->pts, pStreamThreadParam->mTimeBase, AV_TIME_BASE_Q);
        LOGI("Work task stream-%d, PTS:%lld frame queue length:%d, left:%d", iStreamIndex, \
             pStreamThreadParam->mMinPacketPTS, pFrameQueue->queueLength(),pFrameQueue->queueLeft());
        
        HBErr = pFrameRecycleQueue->push(pTargetFrame);
        if (HBErr <= 0) {
            LOGW("Work task push frame to frame queue failed !");
            av_freep(&pTargetFrame->opaque);
            av_frame_free(&pTargetFrame);
        }
        
        pPacket->stream_index = iStreamIndex;
        gettimeofday(&end, NULL);
        
        HBErr = pPacketQueue->push(pPacket);
        if (HBErr <= 0) {
            av_packet_free(&pPacket);
            LOGE("Work task push a packet to queue error, left:%d !", pPacketQueue->queueLeft());
        }
        
        pWriteIpcCtx->condP();
    }
    
    /** 进入刷帧模式 */
    HBErr = avcodec_send_frame(pCodecCtx, NULL);
    if (HBErr < 0)
        LOGE("Work task send data to codec failed !");
    
    /** TODO: 此处任务待定: huangcl */
    while (HBErr >= 0) {
        if (!(pPacketRecycleQueue && (pPacket = pPacketRecycleQueue->get()))) {
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
            LOGE("Work task encode error, %s !", av_err2str(HBErr));
            if (pPacketRecycleQueue->push(pPacket) <= 0)
                av_packet_free(&pPacket);
            break;
        }
        
        pPacket->stream_index = iStreamIndex;
        LOGE("Work task push a packet to write queue !");
        if (pPacketQueue->push(pPacket) <= 0)
            av_packet_free(&pPacket);
        pWriteIpcCtx->condP();
    }
    
    pThreadCtx->markOver();
    pWriteIpcCtx->condP();
    
    LOGW("[%d]Work task Encode thread exit, drop frame:%d !\n", iStreamIndex, iDropFrameCnt);
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
    ThreadIPCContext *pCurWorkThreadIpcCtx = pWorkContextParam->mExportThreadIPCCtx;
    ThreadContext *pCurWorkThreadCtx = pWorkContextParam->mExportThread;
    if (!pWorkContextParam || !pOutFmt || !pCurWorkThreadIpcCtx || !pCurWorkThreadCtx) {
        LOGE("Work task write params failed !");
        return nullptr;
    }
    
    int iStreamThreadParamsSize = (int)pWorkContextParam->mStreamPthreadParamList.size();
    if (iStreamThreadParamsSize <= 0) {
        LOGE("Work task stream params size invalid !");
        return nullptr;
    }
    
    StreamThreadParam **pStreamThreadParamArrys = new StreamThreadParam*[iStreamThreadParamsSize];
    FiFoQueue<AVPacket *> **pPacketQueueArray = new FiFoQueue<AVPacket *> *[iStreamThreadParamsSize];
    FiFoQueue<AVPacket *> **pPacketRecyleQueueArray = new FiFoQueue<AVPacket *> *[iStreamThreadParamsSize];

    if (!pStreamThreadParamArrys || !pPacketQueueArray || !pPacketRecyleQueueArray) {
        LOGE("Work task allock run context failed ! <StreamThreadParam:%p> <Packet:%p> <Packet:%p>", \
                        pStreamThreadParamArrys, pPacketQueueArray, pPacketRecyleQueueArray);
        return nullptr;
    }

    for (int i=0; i<iStreamThreadParamsSize; i++) {
        pStreamThreadParamArrys[i] = pWorkContextParam->mStreamPthreadParamList[i];
        pPacketQueueArray[i] = pStreamThreadParamArrys[i]->mPacketQueue;
        pPacketRecyleQueueArray[i] = pStreamThreadParamArrys[i]->mPacketRecycleQueue;
    }
    
    bool bIsWrite = false;
    int HBErr = HB_OK, iBestPacketIndex = 0;
    int64_t iMinPTS = INT64_MAX, iQueueLastPTS=0;
    ThreadStat eThreadState = THREAD_IDLE;
    AVPacket *pPacket = nullptr;
    long iStreamsPacketCnt[8] = {0};
    
    while (true) {
        eThreadState = pCurWorkThreadCtx->getThreadState();
        if (eThreadState == THREAD_FORCEQUIT) {
            for (int i=0; i < iStreamThreadParamsSize; i++) {
                HBErr = clearPacketQueue(pPacketQueueArray[i]);
                if (HBErr < 0)
                    LOGE("Work stask clear packet queue failed !");
            }
            break;
        }
        
        bIsWrite = false;
        iMinPTS = INT64_MAX;
        for (int i=0; i<iStreamThreadParamsSize; i++) {
            if (pPacketQueueArray[i]->queueLength() > 0) {
                bIsWrite = true;
                /** 根据当前较小的packet pts 决定下一个应该写入的媒体流数据 */
                iQueueLastPTS = pStreamThreadParamArrys[i]->mMinPacketPTS;
                if (iMinPTS > iQueueLastPTS) {
                    iBestPacketIndex = i;
                    iMinPTS = iQueueLastPTS;
                }
            }
        }
        
        if (_CheckIsExitThread(pStreamThreadParamArrys, iStreamThreadParamsSize) \
            && eThreadState == THREAD_STOP && !bIsWrite) {
            LOGW("Work task write thread exit !");
            break;
        }
        else if (bIsWrite == false) {
            if (_CheckIsExitThread(pStreamThreadParamArrys, iStreamThreadParamsSize)) {
                LOGW("Work task encode thread all exit");
                /** huangcl TODO: 此处是否已经退出，还要继续等？ */
            }
            
            pCurWorkThreadIpcCtx->condV();
            continue;
        }
        
        HBErr = updateQueue(pStreamThreadParamArrys[iBestPacketIndex]);
        if (HBErr != HB_OK) {
            LOGE("Work task update queue failed !");
            continue;
        }
        
        pPacket = pStreamThreadParamArrys[iBestPacketIndex]->mBufferPacket;
        if (!pPacket) {
            LOGE("Work task buffer packet is null !");
            continue;
        }
        iStreamsPacketCnt[pPacket->stream_index]++;
        HBErr = av_interleaved_write_frame(pOutFmt, pPacket);
        if (HBErr < 0) {
            LOGE("Work task write data to file error ! %s", makeErrorStr(HBErr));
            if (pPacketRecyleQueueArray[iBestPacketIndex]->push(pPacket) <= 0)
                av_packet_free(&pPacket);
            break;
        }
        av_packet_unref(pPacket);
        if (pPacketRecyleQueueArray[iBestPacketIndex]->push(pPacket) <= 0)
            av_packet_free(&pPacket);
    }
    
    if (pPacketQueueArray)
        delete []pPacketQueueArray;
    if (pPacketRecyleQueueArray)
        delete []pPacketRecyleQueueArray;
    if (pStreamThreadParamArrys)
        delete []pStreamThreadParamArrys;
    
    LOGE("Work task write thread exit !");
    return nullptr;
}

/**
 *  检查线程状态，只要有一个线程没有退出，就返回 false, 表示还有工作线程尚未退出
 */
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
