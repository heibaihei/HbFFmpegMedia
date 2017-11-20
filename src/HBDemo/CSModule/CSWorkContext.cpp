//
//  CSWorkContext.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/12.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSWorkContext.h"
#include "CSThreadContext.h"
#include "CSThreadIPCContext.h"
#include "CSIStream.h"
#include "CSWorkTasks.h"
#include "CSWorkContext.h"

namespace HBMedia {
    
CSWorkContext::CSWorkContext() {
}

CSWorkContext::~CSWorkContext() {
    
}

int CSWorkContext::prepare(void *args) {
    AVFormatContext *outputFormatCtx = (AVFormatContext *)args;
    if (!outputFormatCtx || outputFormatCtx->nb_streams <= 0) {
        LOGE("Work context prepare failed !");
        return HB_ERROR;
    }
    
    mWorkContextParam = (WorkContextParam *)av_mallocz(sizeof(WorkContextParam));
    if (!mWorkContextParam) {
        LOGE("Workcontext create work context param failed !");
        return HB_ERROR;
    }
    mWorkContextParam->mTargetFormatCtx = outputFormatCtx;
    mWorkContextParam->mWorkIPCCtx = new ThreadIPCContext(0);
    if (!mWorkContextParam->mWorkIPCCtx) {
        LOGE("Work context create IPC context failed !");
        return HB_ERROR;
    }
    
    _createOutputWorker();
    return HB_OK;
}

int CSWorkContext::_createOutputWorker() {
    ThreadContext *pThreadCtx = new ThreadContext();
    if (!pThreadCtx) {
        LOGE("Work context create thread context failed !");
        return HB_ERROR;
    }
    
    mWorkContextParam->mWorkThread = pThreadCtx;
    int HBErr = pThreadCtx->setFunction(CSWorkTasks::WorkTask_WritePacketData, mWorkContextParam);
    if (HBErr != HB_OK) {
        LOGE("Work context set work func failed !");
        return HB_ERROR;
    }
    
    mThreadContextList.push_back(pThreadCtx);
    return HB_OK;
}

int CSWorkContext::start() {
    ThreadContext *pThreadCtx = nullptr;
    for (std::vector<ThreadContext *>::iterator pNode = mThreadContextList.begin(); \
         pNode != mThreadContextList.end(); pNode++) {
        pThreadCtx = *pNode;
        pThreadCtx->start();
    }
    
    return HB_OK;
}

int CSWorkContext::stop() {
    ThreadContext *pThreadCtx = nullptr;
    StreamThreadParam *pStreamPthreadParam = nullptr;
    
    for (std::vector<ThreadContext *>::iterator pNode; pNode != mThreadContextList.end(); pNode++) {
        pThreadCtx = *pNode;
        pThreadCtx->stop();
    }
    
    if (mWorkContextParam) {
        for (std::vector<StreamThreadParam*>::iterator pNode; pNode != mWorkContextParam->mStreamPthreadParamList.end(); pNode++) {
            pStreamPthreadParam = *pNode;
            pStreamPthreadParam->mEncodeIPC->condP();
        }
    }
    
    for (std::vector<ThreadContext *>::iterator pNode; pNode != mThreadContextList.end(); pNode++) {
        pThreadCtx = *pNode;
        pThreadCtx->join();
    }
    
    for (std::vector<ThreadContext *>::iterator pNode; pNode != mThreadContextList.end(); pNode++) {
        pThreadCtx = *pNode;
        *pNode = NULL;
        delete pThreadCtx;
    }
    std::vector<ThreadContext *>().swap(mThreadContextList);
    
    if (mWorkContextParam) {
        mWorkContextParam->mStreamPthreadParamList.clear();
        std::vector<StreamThreadParam*>().swap(mWorkContextParam->mStreamPthreadParamList);
        if (mWorkContextParam->mWorkIPCCtx) {
            mWorkContextParam->mWorkIPCCtx->release();
            delete mWorkContextParam->mWorkIPCCtx;
        }
    }
    
    return HB_OK;
}

int CSWorkContext::release() {
    return HB_OK;
}

int CSWorkContext::flush() {
    return HB_OK;
}

int CSWorkContext::pushStream(CSIStream* pStream) {
    if (!pStream) {
        LOGE("Work context push stream args invalid !");
        return HB_ERROR;
    }
    
    StreamThreadParam* pStreamPthreadParam = pStream->getStreamThreadParam();
    if (!pStreamPthreadParam) {
        LOGE("Stream's thread param invalid !");
        return HB_ERROR;
    }

    ThreadContext* pThreadCtx = new ThreadContext();
    if (!pThreadCtx) {
        LOGE("Timeline work context create thread context failed !");
        return HB_ERROR;
    }
    
    pStreamPthreadParam->mThreadCtx = pThreadCtx;
    pStreamPthreadParam->mWriteIPC = mWorkContextParam->mWorkIPCCtx;
    pStreamPthreadParam->mThreadCtx->setFunction(CSWorkTasks::WorkTask_EncodeFrameRawData, pStreamPthreadParam);
    
    mWorkContextParam->mThreadNum++;
    mWorkContextParam->mStreamPthreadParamList.push_back(pStreamPthreadParam);
    mThreadContextList.push_back(pStreamPthreadParam->mThreadCtx);

    return HB_OK;
}

int updateQueue(StreamThreadParam *streamParam) {
    if (!streamParam) {
        LOGE("Updata stream packet queue failed !");
        return HB_ERROR;
    }
    
    AVPacket *pPacket = nullptr;
    FiFoQueue<AVPacket *> *pPacketQueue = streamParam->mPacketQueue;
    streamParam->mUpdateFlag = false;
    if (pPacketQueue->queueLength() == 0)
        return HB_ERROR;
    
    pPacket = pPacketQueue->get();
    if (!pPacket) {
        streamParam->mBufferPacket = nullptr;
        LOGE("Update queue buffer packet is null !");
        return HB_ERROR;
    }
    
    streamParam->mBufferPacket= pPacket;
    streamParam->mUpdateFlag = true;
    return HB_OK;
}

int initialStreamThreadParams(StreamThreadParam *pStreamThreadParam) {
    if (!pStreamThreadParam) {
        LOGE("Stream thread initial with invalid params !");
        return HB_ERROR;
    }
    
    FiFoQueue<AVFrame *> *frameQueue = new FiFoQueue<AVFrame *>(MAX_FRAME_BUFFER);
    FiFoQueue<AVFrame *> *frameRecycleQueue = new FiFoQueue<AVFrame *>(MAX_FRAME_BUFFER);
    FiFoQueue<AVPacket *>*packetQueue = new FiFoQueue<AVPacket *>(MAX_PACKET_BUFFER);
    FiFoQueue<AVPacket *>*packetRecycleQueue = new FiFoQueue<AVPacket *>(MAX_PACKET_BUFFER);
    ThreadIPCContext *pEncodeIpcCtx = new ThreadIPCContext(0);
    ThreadIPCContext *pQueueIpcCtx = new ThreadIPCContext(0);
    
    if (!frameQueue || !frameRecycleQueue\
        || !packetQueue || !packetRecycleQueue || !pQueueIpcCtx) {
        LOGE("Stream thread initial failed, buffer queue malloc error !");
        goto STREAM_THREAD_PARAM_END_LABEL;
    }
    
    pStreamThreadParam->mFrameQueue = frameQueue;
    pStreamThreadParam->mFrameRecycleQueue = frameRecycleQueue;
    pStreamThreadParam->mPacketQueue = packetQueue;
    pStreamThreadParam->mPacketRecycleQueue = packetRecycleQueue;
    pStreamThreadParam->mEncodeIPC = pEncodeIpcCtx;
    pStreamThreadParam->mQueueIPC = pQueueIpcCtx;
    
    return HB_OK;
    
STREAM_THREAD_PARAM_END_LABEL:
    releaseStreamThreadParams(pStreamThreadParam);
    return HB_ERROR;
}

int releaseStreamThreadParams(StreamThreadParam *pStreamThreadParam) {
    AVFrame* pFrame = nullptr;
    if (pStreamThreadParam->mFrameQueue) {
        while ((pFrame = pStreamThreadParam->mFrameQueue->get())) {
            av_freep(&pFrame->opaque);
            av_frame_free(&pFrame);
        }
    }
    
    if (pStreamThreadParam->mFrameRecycleQueue) {
        while ((pFrame = pStreamThreadParam->mFrameRecycleQueue->get())) {
            av_freep(&pFrame->opaque);
            av_frame_free(&pFrame);
        }
    }
    
    AVPacket*pPacket = nullptr;
    if (pStreamThreadParam->mPacketQueue) {
        while ((pPacket = pStreamThreadParam->mPacketQueue->get())) {
            av_packet_free(&pPacket);
        }
    }
    
    if (pStreamThreadParam->mPacketRecycleQueue) {
        while ((pPacket = pStreamThreadParam->mPacketRecycleQueue->get())) {
            av_packet_free(&pPacket);
        }
    }
    
    if (pStreamThreadParam->mEncodeIPC) {
        pStreamThreadParam->mEncodeIPC->release();
        delete pStreamThreadParam->mEncodeIPC;
    }
    
    if (pStreamThreadParam->mQueueIPC) {
        pStreamThreadParam->mQueueIPC->release();
        delete pStreamThreadParam->mQueueIPC;
    }
    
    return HB_OK;
}

int clearFrameQueue(FiFoQueue<AVFrame*> *queue) {
    if (!queue) {
        LOGE("Clear frame queue<%p> failed, args invalid !", queue);
        return HB_ERROR;
    }
    
    AVFrame *pFrame = nullptr;
    while ((pFrame = queue->get()) != NULL) {
        av_freep(pFrame->opaque);
        av_frame_free(&pFrame);
    }
    
    return HB_OK;
}

int clearPacketQueue(FiFoQueue<AVPacket*> *queue) {
    if (!queue) {
        LOGE("Clear packet queue<%p> failed, args invalid !", queue);
        return HB_ERROR;
    }
    
    AVPacket *pPacket = nullptr;
    while ((pPacket = queue->get()) != NULL) {
        av_packet_free(&pPacket);
    }
    return HB_OK;
}

}






