//
//  MTVideoChangeToGif.cpp
//  Sample
//
//  Created by zj-db0519 on 2017/11/2.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "MTVideoTransfer.h"
#include <pthread.h>
#include <unistd.h>

namespace FormatConvert {

#define STATE_UNKNOWN            0X0001
#define STATE_PREPARED           0X0002
#define STATE_ABORT              0X0004
#define STATE_FINISHED           0X0008
#define STATE_READ_END           0X0010
    
#define STATE_DECODE_END         0X0020
#define STATE_DECODE_FLUSHING    0X0040
#define STATE_DECODE_ABORT       0X0080
    
#define STATE_ENCODE_END         0X0100
#define STATE_ENCODE_FLUSHING    0X0200
#define STATE_ENCODE_ABORT       0X0400

#define MAX_FRAME_BUFFER  (5)
typedef void *(*ThreadFunc)(void *arg);

MediaCoder* AllocMediaCoder() {
    MediaCoder *coder = (MediaCoder *)malloc(sizeof(MediaCoder));
    if (coder) {
        coder->mPVideoFormatCtx = nullptr;
        coder->mPVideoStream = nullptr;
        coder->mPVideoCodecCtx = nullptr;
        coder->mPVideoCodec = nullptr;
        coder->mVideoStreamIndex = INVALID_STREAM_INDEX;
    }
    return coder;
}

void ImageParamsInitial(ImageParams *pParams) {
    if (pParams) {
        pParams->mPixFmt = CS_PIX_FMT_YUV420P;
        pParams->mWidth = 0;
        pParams->mHeight = 0;
        pParams->mFormatType = nullptr;
        pParams->mAlign = 1;
        pParams->mPreImagePixBufferSize = 0;
        pParams->mBitRate = 0;
        pParams->mRotate = 0;
        pParams->mFrameRate = 0.0;
    }
}

void ThreadIpcCtxInitial(ThreadIpcCtx *pIpcCtx) {
    if (pIpcCtx) {
        pIpcCtx->mIsThreadPending = false;
        pthread_cond_init(&(pIpcCtx->mThreadCond), NULL);
        pthread_mutex_init(&(pIpcCtx->mThreadMux), NULL);
    }
}

void *VideoFormatTranser::DecodeThreadFunc(void *arg) {
    if (!arg)
        return NULL;
    
    int HBError = -1;
    AVPacket* pNewPacket = nullptr;
    VideoFormatTranser *pVideoTranser = (VideoFormatTranser *)arg;
    MediaCoder * pMediaDecoder = pVideoTranser->mPMediaDecoder;
    
    while (!(pVideoTranser->mState & STATE_DECODE_END)) {
        
        if ((pVideoTranser->mEncodeFrameQueue->queueLeft() <= 0) \
            && (pVideoTranser->mDecodePacketQueue->queueLength() <= 0)) {
            
            pthread_mutex_lock(&(pVideoTranser->mDecodeThreadIpcCtx.mThreadMux));
            if (pVideoTranser->mEncodeFrameQueue->queueLeft() == 0 \
                && pVideoTranser->mDecodePacketQueue->queueLength() == 0)
            {
                if (!(pVideoTranser->mState & STATE_READ_END) \
                    && !(pVideoTranser->mState & STATE_DECODE_ABORT))
                {/** 判断是否读包结束 或者本身解包异常 */
                    pVideoTranser->mDecodeThreadIpcCtx.mIsThreadPending = true;
                    pthread_cond_wait(&(pVideoTranser->mDecodeThreadIpcCtx.mThreadCond), \
                                      &(pVideoTranser->mDecodeThreadIpcCtx.mThreadMux));
                    pVideoTranser->mDecodeThreadIpcCtx.mIsThreadPending = false;
                }
            }
            pthread_mutex_unlock(&(pVideoTranser->mDecodeThreadIpcCtx.mThreadMux));
        }
        
        pNewPacket = nullptr;
        if (pVideoTranser->mState & STATE_DECODE_FLUSHING) {
            /**  说明进入刷帧模式， 无需再从外部取帧 */
        }
        else if ((pVideoTranser->mDecodePacketQueue->queueLength() > 0) \
            && (pVideoTranser->mEncodeFrameQueue->queueLeft() > 0) \
            && (nullptr != (pNewPacket = pVideoTranser->mDecodePacketQueue->get())))
        {   /** 得到帧 */
        }
        else if (!(pVideoTranser->mState & STATE_READ_END) \
                 && !(pVideoTranser->mState & STATE_DECODE_ABORT)) {
            LOGE("Decode thread get valid packet failed or not free queue node !");
            continue;
        }

        if (pNewPacket) {
            HBError = avcodec_send_packet(pMediaDecoder->mPVideoCodecCtx, pNewPacket);
            if (HBError != 0) {
                if (HBError != AVERROR(EAGAIN))
                    pVideoTranser->mState |= STATE_DECODE_ABORT;
                if (pNewPacket) {
                    av_packet_free(&pNewPacket);
                }
                continue;
            }
        }
        else if (!(pVideoTranser->mState & STATE_DECODE_FLUSHING) \
                 && ((pVideoTranser->mState & STATE_READ_END) || (pVideoTranser->mState & STATE_DECODE_ABORT))) {
            HBError = avcodec_send_packet(pMediaDecoder->mPVideoCodecCtx, NULL);
            pVideoTranser->mState |= STATE_DECODE_FLUSHING;
            LOGW("Decode process into flush buffer !");
        }

        while (true)
        {
            AVFrame* pNewFrame = av_frame_alloc();
            if (!pNewFrame) {
                LOGE("Trans media malloc new frame failed !");
                pVideoTranser->mState |= (STATE_DECODE_ABORT | STATE_DECODE_END);
                return NULL;
            }
            
            HBError = avcodec_receive_frame(pMediaDecoder->mPVideoCodecCtx, pNewFrame);
            if (HBError != 0) {
                if ((HBError != AVERROR(EAGAIN)) || (pVideoTranser->mState & STATE_DECODE_FLUSHING)) {
                    pVideoTranser->mState |= STATE_DECODE_END;
                    LOGW("Decode process end!");
                }
                av_frame_free(&pNewFrame);
                break;
            }
            else {
                if (pVideoTranser->_ImageConvert(pNewFrame) != HB_OK) {
                    if (pNewFrame)
                        av_frame_free(&pNewFrame);
                    LOGE("image convert failed !");
                    break;
                }
                else {
                    pVideoTranser->mEncodeFrameQueue->push(pNewFrame);
                    if (!(pVideoTranser->mState & STATE_ENCODE_END)) {
                        pthread_mutex_lock(&(pVideoTranser->mEncodeThreadIpcCtx.mThreadMux));
                        if (pVideoTranser->mEncodeThreadIpcCtx.mIsThreadPending \
                            && (pVideoTranser->mEncodeFrameQueue->queueLength() > 0)) {
                            pthread_cond_signal(&(pVideoTranser->mEncodeThreadIpcCtx.mThreadCond));
                        }
                        pthread_mutex_unlock(&(pVideoTranser->mEncodeThreadIpcCtx.mThreadMux));
                    }
                    else
                        pVideoTranser->mState |= STATE_DECODE_END;
                }
                break;
            }
        }
    }
    
    return NULL;
}

void *VideoFormatTranser::EncodeThreadFunc(void *arg) {
    if (!arg)
        return NULL;
    
    int HBError = -1;
    AVFrame *pNewFrame = nullptr;
    VideoFormatTranser *pVideoTranser = (VideoFormatTranser *)arg;
    MediaCoder * pMediaEncoder = pVideoTranser->mPMediaEncoder;
    
    while (!(pVideoTranser->mState & STATE_ENCODE_END)) {
        
        /** TODO: 是否这里只有 output 为 0 的情况需要陷入等待 */
        if (pVideoTranser->mOutputPacketQueue->queueLeft() <= 0 \
            && pVideoTranser->mEncodeFrameQueue->queueLength() <= 0) {
            
            pthread_mutex_lock(&(pVideoTranser->mEncodeThreadIpcCtx.mThreadMux));
            if (pVideoTranser->mOutputPacketQueue->queueLeft() <= 0 \
                && pVideoTranser->mEncodeFrameQueue->queueLength() <= 0)
            {
                if (!(pVideoTranser->mState & STATE_DECODE_END) \
                    && !(pVideoTranser->mState & STATE_ENCODE_ABORT))
                {
                    pVideoTranser->mEncodeThreadIpcCtx.mIsThreadPending = true;
                    pthread_cond_wait(&(pVideoTranser->mEncodeThreadIpcCtx.mThreadCond), \
                                      &(pVideoTranser->mEncodeThreadIpcCtx.mThreadMux));
                    pVideoTranser->mEncodeThreadIpcCtx.mIsThreadPending = false;
                }
            }
            pthread_mutex_unlock(&(pVideoTranser->mEncodeThreadIpcCtx.mThreadMux));
        }
        
        pNewFrame = nullptr;
        if (pVideoTranser->mState & STATE_ENCODE_FLUSHING) {
            /**  说明进入刷帧模式， 无需再从外部取帧 */
        }
        else if ((pVideoTranser->mEncodeFrameQueue->queueLength() > 0) \
                 && (pVideoTranser->mOutputPacketQueue->queueLeft() > 0) \
                 && (nullptr != (pNewFrame = pVideoTranser->mEncodeFrameQueue->get())))
        {   /** 得到帧 */
        }
        else if (!(pVideoTranser->mState & STATE_DECODE_END) \
                 && !(pVideoTranser->mState & STATE_ENCODE_ABORT)) {
            LOGE("encode thread get valid frame failed or not free queue node !");
            continue;
        }

        if (pNewFrame) {
            HBError = avcodec_send_frame(pMediaEncoder->mPVideoCodecCtx, pNewFrame);
            if (HBError != 0) {
                LOGE("image convert failed, %s", makeErrorStr(HBError));
                if (HBError != AVERROR(EAGAIN))
                    pVideoTranser->mState |= STATE_ENCODE_ABORT;
                if (pNewFrame) {
                    av_frame_free(&pNewFrame);
                }
                continue;
            }
        }
        else if (!(pVideoTranser->mState & STATE_ENCODE_FLUSHING) \
                 && ((pVideoTranser->mState & STATE_DECODE_END) || (pVideoTranser->mState & STATE_ENCODE_ABORT))) {
            HBError = avcodec_send_frame(pMediaEncoder->mPVideoCodecCtx, NULL);
            pVideoTranser->mState |= STATE_ENCODE_FLUSHING;
            LOGW("Encode process into flush buffer !");
        }

        while (true) {
            AVPacket *pNewPacket = av_packet_alloc();
            if (!pNewPacket) {
                LOGE("Video format transer alloc new packet room failed !");
                return NULL;
            }
            
            HBError = avcodec_receive_packet(pMediaEncoder->mPVideoCodecCtx, pNewPacket);
            if (HBError != 0) {
                if ((HBError != AVERROR(EAGAIN)) || (pVideoTranser->mState & STATE_ENCODE_FLUSHING)) {
                    pVideoTranser->mState |= STATE_ENCODE_END;
                    LOGW("tran codec finished !%s", av_err2str(HBError));
                }
                av_packet_free(&pNewPacket);
                break;
            }
            else {
                pVideoTranser->mOutputPacketQueue->push(pNewPacket);
                if (!((pVideoTranser->mState & STATE_ABORT) \
                    || (pVideoTranser->mState & STATE_FINISHED)))
                {
                    pthread_mutex_lock(&(pVideoTranser->mReadThreadIpcCtx.mThreadMux));
                    if (pVideoTranser->mReadThreadIpcCtx.mIsThreadPending \
                        && (pVideoTranser->mOutputPacketQueue->queueLength() > 0)) {
                        pthread_cond_signal(&(pVideoTranser->mReadThreadIpcCtx.mThreadCond));
                    }
                    pthread_mutex_unlock(&(pVideoTranser->mReadThreadIpcCtx.mThreadMux));
                }
                else
                    pVideoTranser->mState |= STATE_ENCODE_END;
                break;
            }
        }
    }
    
    return NULL;
}

VideoFormatTranser::VideoFormatTranser() {
    mPMediaDecoder = AllocMediaCoder();
    mPMediaEncoder = AllocMediaCoder();
    mPVideoConvertCtx = nullptr;
    mInputMediaFile = nullptr;
    mOutputMediaFile = nullptr;
    mDecodeThreadId = nullptr;
    mEncodeThreadId = nullptr;
    mIsSyncMode = false;
    ImageParamsInitial(&mInputImageParams);
    ImageParamsInitial(&mOutputImageParams);
    
    mDecodePacketQueue = nullptr;
    mEncodeFrameQueue = nullptr;
    mOutputPacketQueue = nullptr;
    
    memset(&mState, 0x00, sizeof(mState));
    mState |= STATE_UNKNOWN;
}

int VideoFormatTranser::prepare() {
    
    if (S_NOT_EQ(mState,STATE_UNKNOWN)) {
        LOGE("Video transer state error, no initial state, can't do prepare !");
        return HB_ERROR;
    }
    
    av_register_all();
    avformat_network_init();
    
    if (_InputMediaInitial() != HB_OK) {
        LOGE("Input media initial failed !");
        goto PREPARE_END_LABEL;
    }
    
    if (_OutputMediaInitial() != HB_OK) {
        LOGE("Output media initial failed !");
        goto PREPARE_END_LABEL;
    }
    
    if (_SwsMediaInitial() != HB_OK) {
        LOGE("Media sws initial failed !");
        goto PREPARE_END_LABEL;
    }
    
    mOriginalFrame = av_frame_alloc();
    mState |= STATE_PREPARED;
    return HB_OK;
    
PREPARE_END_LABEL:
    _release();
    return HB_ERROR;
}

int VideoFormatTranser::doConvert() {
    if (S_NOT_EQ(mState,STATE_PREPARED)) {
        LOGE("Video format transer has't initial, can't do format convert !");
        return HB_ERROR;
    }
    
    int HBError = HB_ERROR;
    AVPacket *pNewPacket = nullptr;
    bool bNeedTranscode = ((mPVideoConvertCtx || (mPMediaDecoder->mPVideoCodec->id != mPMediaEncoder->mPVideoCodec->id)) ? true : false);
    if ((HBError = avformat_write_header(mPMediaEncoder->mPVideoFormatCtx, NULL)) < 0) {
        LOGE("Avformat write header failed, %s!", makeErrorStr(HBError));
        goto CONVERT_END_LABEL;
    }
    
    if (bNeedTranscode) {
        if (_WorkPthreadPrepare() != HB_OK) {
            LOGE("work phtread prepare failed !");
            goto CONVERT_END_LABEL;
        }
    }

    while (S_NOT_EQ(mState,STATE_ABORT) && S_NOT_EQ(mState,STATE_FINISHED)) {
        
        if (S_EQ(mState, STATE_DECODE_END)) {
            mState |= STATE_FINISHED;
            LOGW("Video format transer finished !");
            continue;
        }
            
        if (!mIsSyncMode \
            && mDecodePacketQueue->queueLeft() <= 0 \
            && mOutputPacketQueue->queueLength() <= 0)
        {   /** 如果编码以及解码线上两个地方都没有数据可用，则可以让当前线程陷入睡眠 */
            pthread_mutex_lock(&(mReadThreadIpcCtx.mThreadMux));
            if (mDecodePacketQueue->queueLeft() == 0 \
                && mOutputPacketQueue->queueLength() == 0)
            {
                mReadThreadIpcCtx.mIsThreadPending = true;
                pthread_cond_wait(&(mReadThreadIpcCtx.mThreadCond), &(mReadThreadIpcCtx.mThreadMux));
                mReadThreadIpcCtx.mIsThreadPending = false;
            }
            pthread_mutex_unlock(&(mReadThreadIpcCtx.mThreadMux));
        }

        /** 读取原始数据 */
        pNewPacket = nullptr;
        if (S_NOT_EQ(mState, STATE_READ_END) \
            && S_NOT_EQ(mState, STATE_DECODE_END) \
            && S_NOT_EQ(mState, STATE_DECODE_ABORT))
        {
            if (mIsSyncMode || (mDecodePacketQueue->queueLeft() > 0)) {
                if (!(pNewPacket = av_packet_alloc())) {
                    LOGE("%d do convert read frame failed !", __LINE__);
                    mState |= STATE_ABORT;
                    continue;
                }
                HBError = av_read_frame(mPMediaDecoder->mPVideoFormatCtx, pNewPacket);
                if (HBError != 0) {
                    if (HBError != AVERROR_EOF)
                        LOGE("Read input video data end, Err:%s", av_err2str(HBError));
                    else
                        LOGW("Read input video data end, reach file eof !");
                    
                    mState |= STATE_READ_END;
                    if (!bNeedTranscode && HBError == AVERROR_EOF)
                        mState |= STATE_FINISHED;
                    continue;
                }
                /** 得到数据包 */
                if (pNewPacket->stream_index != mPMediaDecoder->mVideoStreamIndex) {
                    av_packet_free(&pNewPacket);
                    continue;
                }
            }
        }
        
        if (bNeedTranscode) {
            if (mIsSyncMode) {
                if (!pNewPacket && !(pNewPacket = av_packet_alloc())) {
                    LOGE("%d do convert read frame failed !", __LINE__);
                    mState |= STATE_ABORT;
                    continue;
                }
                if ((HBError = _TransMedia(&pNewPacket)) != 0) {
                    if (pNewPacket)
                        av_packet_free(&pNewPacket);
                    if (HBError == -2) {/** 转码发生异常，则直接退出 */
                        mState |= STATE_ABORT;
                        LOGE("Trans video media packet occur exception !");
                    }
                    continue;
                }
            }
            else
            {   /** 异步模式 */
                if (pNewPacket) { /** 将得到的原始包放入解码包队列 */
                    if (mDecodePacketQueue->queueLeft() > 0) {
                        if (mDecodePacketQueue->push(pNewPacket) <= 0) {
                            LOGE("Push new raw packet to decode queue failed !");
                            av_packet_free(&pNewPacket);
                        }
                        else if (S_NOT_EQ(mState,STATE_DECODE_END)) {
                            pthread_mutex_lock(&(mDecodeThreadIpcCtx.mThreadMux));
                            if (mDecodeThreadIpcCtx.mIsThreadPending && (mDecodePacketQueue->queueLength() > 0))
                                pthread_cond_signal(&mDecodeThreadIpcCtx.mThreadCond);
                            pthread_mutex_unlock(&(mDecodeThreadIpcCtx.mThreadMux));
                        }
                    }
                    else {
                        av_packet_free(&pNewPacket);
                        LOGE("Read thread push packet into queue , but queue without free node !");
                    }
                }
                
                /** 从已经编码好的队列中读取输出 packet 数据包 */
                pNewPacket = nullptr;
                if (mOutputPacketQueue->queueLength() > 0) {
                    pNewPacket = mOutputPacketQueue->get();
                    if (!pNewPacket) {
                        LOGE("Get target output avpacket from queue failed !");
                    }
                    if ((mOutputPacketQueue->queueLeft() > 0) && S_NOT_EQ(mState,STATE_ENCODE_END)) {
                        pthread_mutex_lock(&(mEncodeThreadIpcCtx.mThreadMux));
                        if (mEncodeThreadIpcCtx.mIsThreadPending && (mOutputPacketQueue->queueLeft() > 0))
                            pthread_cond_signal(&mEncodeThreadIpcCtx.mThreadCond);/** 通知解码线程开始工作 */
                        pthread_mutex_unlock(&(mEncodeThreadIpcCtx.mThreadMux));
                    }
                }
                else if (S_EQ(mState,STATE_ENCODE_END)) {
                    mState |= STATE_FINISHED;
                }
            }
        }
        
        if (pNewPacket) {
            HBError = av_interleaved_write_frame(mPMediaEncoder->mPVideoFormatCtx, pNewPacket);
            if (HBError < 0) {
                LOGE("Write frame failed !");
                mState |= STATE_ABORT;/** 如果写入失败，则直接退出 */
            }
            av_packet_free(&pNewPacket);
        }
    }
    
    if ((HBError = av_write_trailer(mPMediaEncoder->mPVideoFormatCtx)) != 0)
        LOGE("AVformat wirte tailer failed, %s ", makeErrorStr(HBError));
    
    av_packet_free(&pNewPacket);
    _release();
    return HB_OK;

CONVERT_END_LABEL:
    av_packet_free(&pNewPacket);
    _release();
    return HB_ERROR;
}

int VideoFormatTranser::_release() {
    
    _WorkPthreadDispose();
    
    if (mPMediaDecoder->mPVideoCodecCtx) {
        if (avcodec_is_open(mPMediaDecoder->mPVideoCodecCtx))
            avcodec_close(mPMediaDecoder->mPVideoCodecCtx);
        avcodec_free_context(&(mPMediaDecoder->mPVideoCodecCtx));
        mPMediaDecoder->mPVideoCodecCtx = nullptr;
        mPMediaDecoder->mPVideoCodec = nullptr;
    }
    if (mPMediaEncoder->mPVideoCodecCtx) {
        if (avcodec_is_open(mPMediaEncoder->mPVideoCodecCtx))
            avcodec_close(mPMediaEncoder->mPVideoCodecCtx);
        avcodec_free_context(&(mPMediaEncoder->mPVideoCodecCtx));
        mPMediaEncoder->mPVideoCodecCtx = nullptr;
        mPMediaEncoder->mPVideoCodec = nullptr;
    }
    
    if (mPMediaEncoder->mPVideoFormatCtx)
        avformat_close_input(&(mPMediaEncoder->mPVideoFormatCtx));
    if (mPMediaDecoder->mPVideoFormatCtx)
        avformat_close_input(&(mPMediaDecoder->mPVideoFormatCtx));
    
    mPMediaDecoder->mPVideoStream = nullptr;
    mPMediaEncoder->mPVideoStream = nullptr;
    
    if (mOriginalFrame) {
        if (mOriginalFrame->opaque)
            av_freep(mOriginalFrame->opaque);
        av_frame_free(&mOriginalFrame);
    }
    memset(&mState, 0x00, sizeof(mState));
    mState |= STATE_UNKNOWN;
    return HB_OK;
}

int VideoFormatTranser::_TransMedia(AVPacket** pInPacket) {
    int HBError = -1;
    if (!pInPacket || !(*pInPacket)) {
        LOGE("Video trans failed, input a invalid packet !");
        return HBError;
    }
    
    AVFrame *pConvertFrame = nullptr, *pTargetFrame = nullptr;
    AVFrame *pNewFrame = mOriginalFrame;

    if (S_NOT_EQ(mState,STATE_DECODE_END))
    { /** 进入解码模块流程 */
        if (S_NOT_EQ(mState,STATE_READ_END) && S_NOT_EQ(mState,STATE_DECODE_ABORT))
        {
            /** 只有读数据包未结束以及本身解码器没有发生异常，说明都要往里面丢数据 */
            HBError = avcodec_send_packet(mPMediaDecoder->mPVideoCodecCtx, *pInPacket);
            if (HBError != 0) {
                if (HBError != AVERROR(EAGAIN)) {
                    mState |= STATE_DECODE_ABORT;
                    if (S_NOT_EQ(mState, STATE_READ_END))
                        mState |= STATE_ABORT;
                }
                return -1;
            }
        }
        else if (S_NOT_EQ(mState,STATE_DECODE_FLUSHING) \
                 && (S_EQ(mState,STATE_READ_END) || S_EQ(mState,STATE_DECODE_ABORT)))
        {
            HBError = avcodec_send_packet(mPMediaDecoder->mPVideoCodecCtx, NULL);
            mState |= STATE_DECODE_FLUSHING;
            LOGW("Decode process into flush buffer !");
        }
        
        while (true) {
            HBError = avcodec_receive_frame(mPMediaDecoder->mPVideoCodecCtx, pNewFrame);
            if (HBError != 0) {
                if (S_EQ(mState,STATE_DECODE_FLUSHING))
                    mState |= STATE_DECODE_END;
                else {
                    if (HBError != AVERROR(EAGAIN))
                        mState |= STATE_DECODE_ABORT;
                }
                
                if (S_EQ(mState,STATE_DECODE_END))
                    LOGW("Decode process end!");
                
                av_frame_unref(pNewFrame);
                return -1;
            }
            break;
        }
    }
    
    /** 将解码数据进行图像转码 */
    pTargetFrame = nullptr;
    if (pNewFrame) {
        pTargetFrame = pNewFrame;
        if (mPVideoConvertCtx) {
            pConvertFrame = nullptr;
            if (_ImageConvert(pNewFrame, &pConvertFrame) != HB_OK) {
                av_frame_unref(pNewFrame);
                LOGE("image convert failed !");
                return -1;
            }
            pTargetFrame = pConvertFrame;
        }
    }
    
    if (S_NOT_EQ(mState,STATE_DECODE_END) && S_NOT_EQ(mState,STATE_ENCODE_ABORT)) {
        HBError = avcodec_send_frame(mPMediaEncoder->mPVideoCodecCtx, pTargetFrame);
        if (HBError != 0) {
            LOGE("image convert failed, %s", makeErrorStr(HBError));
            
            /** 情况帧空间 */
            if (pConvertFrame) {
                if (pConvertFrame->opaque)
                    SAFE_FREE(pConvertFrame->opaque);
                av_frame_free(&pConvertFrame);
            }
            pTargetFrame = nullptr;
            av_frame_unref(pNewFrame);
            
            if (HBError != AVERROR(EAGAIN)) {
                mState |= STATE_ENCODE_ABORT;
                if (S_NOT_EQ(mState,STATE_DECODE_END)) {
                    /** 如果帧原始数据未解码结束，编码器发生异常，则直接当作异常退出 */
                    mState |= STATE_ABORT;
                }
            }
            return -1;
        }
        
        /** 情况帧空间 */
        if (pConvertFrame) {
            if (pConvertFrame->opaque)
                SAFE_FREE(pConvertFrame->opaque);
            av_frame_free(&pConvertFrame);
        }
        pTargetFrame = nullptr;
        av_frame_unref(pNewFrame);
    }
    else if (S_NOT_EQ(mState,STATE_ENCODE_FLUSHING) \
             && (S_EQ(mState,STATE_DECODE_END) || S_EQ(mState,STATE_ENCODE_ABORT))) {
        HBError = avcodec_send_frame(mPMediaEncoder->mPVideoCodecCtx, NULL);
        mState |= STATE_ENCODE_FLUSHING;
        LOGW("Encode process into flush buffer !");
    }
    
    AVPacket *pNewPacket = av_packet_alloc();
    if (!pNewPacket) {
        LOGE("Video format transer alloc new packet room failed !");
        return -2;
    }
    
    while (true) {
        HBError = avcodec_receive_packet(mPMediaEncoder->mPVideoCodecCtx, pNewPacket);
        if (HBError != 0) {
            if ((HBError != AVERROR(EAGAIN))) {
                if (S_EQ(mState,STATE_ENCODE_FLUSHING)) {
                    LOGW("1 tran codec finished !%s", av_err2str(HBError));
                    mState |= STATE_ENCODE_END;
                }
                else {
                    LOGE("trans decode video frame failed !%s", av_err2str(HBError));
                    mState |= STATE_ENCODE_ABORT;
                }
                mState |= STATE_ENCODE_END;
            }
            av_packet_free(&pNewPacket);
            return -1;
        }
        break;
    }
    
    /** 将外部传入的 pInPacket 内存清空，并将该指针指向新的 packet */
    av_packet_free(pInPacket);
    *pInPacket = pNewPacket;
    return 0;
    
TRANS_MEDIA_END_LABEL:
    return HBError;
}
    
int VideoFormatTranser::_ImageConvert(AVFrame* pInFrame, AVFrame** pOutFrame) {
    int HBError = -1;
    if (!pInFrame || !pOutFrame) {
        LOGE("Video trans frame failed, input a invalid frame !");
        return HB_ERROR;
    }
    
    *pOutFrame = nullptr;
    AVFrame *pNewFrame = av_frame_alloc();
    if (!pNewFrame) {
        LOGE("Image conver, alloc new frame room failed !");
        goto IMAGE_CONVERT_END_LABEL;
    }
    
    HBError = av_image_alloc(pNewFrame->data, pNewFrame->linesize, mOutputImageParams.mWidth, mOutputImageParams.mHeight, \
                  getImageInnerFormat(mOutputImageParams.mPixFmt), mOutputImageParams.mAlign);
    if (HBError < 0) {
        LOGE("Video format alloc new image room failed !");
        goto IMAGE_CONVERT_END_LABEL;
    }
    
    pNewFrame->opaque = pNewFrame->data[0];
    if (sws_scale(mPVideoConvertCtx, pInFrame->data, pInFrame->linesize, 0, pInFrame->height, \
                  pNewFrame->data, pNewFrame->linesize) <= 0) {
        LOGE("Image convert sws failed !");
        goto IMAGE_CONVERT_END_LABEL;
    }
    pNewFrame->width = mOutputImageParams.mWidth;
    pNewFrame->height = mOutputImageParams.mHeight;
    pNewFrame->pts =  av_rescale_q(pInFrame->pts, \
                                   mPMediaDecoder->mPVideoStream->time_base, \
                                   mPMediaEncoder->mPVideoStream->time_base);
    pNewFrame->format = getImageInnerFormat(mOutputImageParams.mPixFmt);
    *pOutFrame = pNewFrame;
    return HB_OK;
    
IMAGE_CONVERT_END_LABEL:
    if (pNewFrame) {
        if (pNewFrame->opaque)
            SAFE_FREE(pNewFrame->opaque);
        av_frame_free(&pNewFrame);
    }
    return HB_ERROR;
}

int VideoFormatTranser::_InputMediaInitial() {
    AVDictionaryEntry *tag = NULL;
    AVFormatContext* pFormatCtx = nullptr;
    AVStream* pVideoStream = nullptr;
    
    int HBError = avformat_open_input(&(mPMediaDecoder->mPVideoFormatCtx), mInputMediaFile, NULL, NULL);
    if (HBError != 0) {
        LOGE("Video decoder couldn't open input file <%s>", av_err2str(HBError));
        goto INPUT_INITIAL_END_LABEL;
    }
    
    HBError = avformat_find_stream_info(mPMediaDecoder->mPVideoFormatCtx, NULL);
    if (HBError < 0) {
        LOGE("Video decoder couldn't find stream information. <%s>", av_err2str(HBError));
        goto INPUT_INITIAL_END_LABEL;
    }
    
    pFormatCtx = mPMediaDecoder->mPVideoFormatCtx;
    mPMediaDecoder->mPVideoStream = nullptr;
    mPMediaDecoder->mVideoStreamIndex = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (mPMediaDecoder->mVideoStreamIndex < 0) {
        LOGW("Video decoder counldn't find valid audio stream, %s", \
             av_err2str(mPMediaDecoder->mVideoStreamIndex));
        goto INPUT_INITIAL_END_LABEL;
    }
    
    mPMediaDecoder->mPVideoStream = pFormatCtx->streams[mPMediaDecoder->mVideoStreamIndex];
    pVideoStream = mPMediaDecoder->mPVideoStream;
    mPMediaDecoder->mPVideoCodec = avcodec_find_decoder(pVideoStream->codecpar->codec_id);
    if (!mPMediaDecoder->mPVideoCodec) {
        LOGE("Codec <%d> not found !", pVideoStream->codecpar->codec_id);
        goto INPUT_INITIAL_END_LABEL;
    }
    
    mPMediaDecoder->mPVideoCodecCtx = avcodec_alloc_context3(mPMediaDecoder->mPVideoCodec);
    if (!mPMediaDecoder->mPVideoCodecCtx) {
        LOGE("Codec ctx <%d> not found !", pVideoStream->codecpar->codec_id);
        goto INPUT_INITIAL_END_LABEL;
    }
    avcodec_parameters_to_context(mPMediaDecoder->mPVideoCodecCtx, pVideoStream->codecpar);
    
    tag = av_dict_get(pVideoStream->metadata, "rotate", tag, 0);
    if (tag != NULL) {
        mInputImageParams.mRotate = atoi(tag->value);
    } else {
        mInputImageParams.mRotate = 0;
    }
    LOGD("input video rotate:%d", mInputImageParams.mRotate);
    mInputImageParams.mBitRate = pVideoStream->codecpar->bit_rate;
    mInputImageParams.mWidth = pVideoStream->codecpar->width;
    mInputImageParams.mHeight = pVideoStream->codecpar->height;
    mInputImageParams.mPixFmt = getImageExternFormat((AVPixelFormat)(pVideoStream->codecpar->format));
    mInputImageParams.mFrameRate = pVideoStream->avg_frame_rate.num * 1.0 / pVideoStream->avg_frame_rate.den;
    mInputImageParams.mPreImagePixBufferSize = av_image_get_buffer_size((AVPixelFormat)(pVideoStream->codecpar->format), \
                                       mInputImageParams.mWidth, mInputImageParams.mHeight, mInputImageParams.mAlign);
    
    av_dump_format(pFormatCtx, mPMediaDecoder->mVideoStreamIndex, mInputMediaFile, 0);

    HBError = avcodec_open2(mPMediaDecoder->mPVideoCodecCtx, mPMediaEncoder->mPVideoCodec, NULL);
    if (HBError < 0) {
        LOGE("Could not open codec. <%s>", av_err2str(HBError));
        goto INPUT_INITIAL_END_LABEL;
    }
    
    return HB_OK;
INPUT_INITIAL_END_LABEL:
    if (mPMediaDecoder->mPVideoCodecCtx) {
        avcodec_close(mPMediaDecoder->mPVideoCodecCtx);
        avcodec_free_context(&(mPMediaDecoder->mPVideoCodecCtx));
    }
    if (mPMediaDecoder->mPVideoFormatCtx) {
        avformat_close_input(&(mPMediaDecoder->mPVideoFormatCtx));
        mPMediaDecoder->mPVideoFormatCtx = nullptr;
    }
    mPMediaDecoder->mVideoStreamIndex = INVALID_STREAM_INDEX;
    mPMediaDecoder->mPVideoStream = nullptr;
    return HB_ERROR;
}

int VideoFormatTranser::_OutputMediaInitial() {
    
    {/** 参数初始化 */
        if (mOutputImageParams.mWidth <= 0 || mOutputImageParams.mHeight <= 0) {
            mOutputImageParams.mWidth = mInputImageParams.mWidth;
            mOutputImageParams.mHeight = mInputImageParams.mHeight;
        }
        if (mOutputImageParams.mBitRate <= 1000)
            mOutputImageParams.mBitRate = mInputImageParams.mBitRate;
        if (mOutputImageParams.mFrameRate <= 0)
            mOutputImageParams.mFrameRate = mInputImageParams.mFrameRate;
        mOutputImageParams.mPixFmt = CS_PIX_FMT_RGB8;
        mOutputImageParams.mAlign = 4;
    }
    
    AVFormatContext* pFormatCtx = nullptr;
    AVStream *pVideoStream = nullptr;
    AVCodecContext *pCodecCtx = nullptr;
    AVDictionary *opts = NULL;
    
    /** 是否要指定输出格式为 gif */
    int HBError = avformat_alloc_output_context2(&(mPMediaEncoder->mPVideoFormatCtx), NULL, NULL, mOutputMediaFile);
    if (HBError < 0) {
        LOGE("Create output media avforamt failed, %s !", av_err2str(HBError));
        goto OUTPUT_INITIAL_END_LABEL;
    }
    
    pFormatCtx = mPMediaEncoder->mPVideoFormatCtx;
    HBError = avio_open(&(pFormatCtx->pb), mOutputMediaFile, AVIO_FLAG_WRITE);
    if (HBError < 0) {
        LOGE("Avio open output file failed, %s !", av_err2str(HBError));
        goto OUTPUT_INITIAL_END_LABEL;
    }
    
    strncpy(pFormatCtx->filename, mOutputMediaFile, strlen(mOutputMediaFile));
    mPMediaEncoder->mPVideoCodec = avcodec_find_encoder_by_name("gif");
    if (mPMediaEncoder->mPVideoCodec == NULL) {
        LOGE("Avcodec find output encode codec failed !");
        goto OUTPUT_INITIAL_END_LABEL;
    }
    
    pVideoStream = avformat_new_stream(mPMediaEncoder->mPVideoFormatCtx, mPMediaEncoder->mPVideoCodec);
    if (pVideoStream == NULL) {
        LOGE("Avformat new output video stream failed !");
        goto OUTPUT_INITIAL_END_LABEL;
    }
    pVideoStream->time_base.num = 1;
    pVideoStream->time_base.den = 90000;
    mPMediaEncoder->mVideoStreamIndex = pVideoStream->index;
    mPMediaEncoder->mPVideoStream = pVideoStream;
    
    mPMediaEncoder->mPVideoCodecCtx = avcodec_alloc_context3(mPMediaEncoder->mPVideoCodec);
    if (mPMediaEncoder->mPVideoCodecCtx == NULL) {
        LOGE("avcodec context alloc output video context failed !");
        goto OUTPUT_INITIAL_END_LABEL;
    }
    
    pCodecCtx = mPMediaEncoder->mPVideoCodecCtx;
    if (mOutputImageParams.mWidth < 0) {
        pCodecCtx->width = -mOutputImageParams.mWidth;
    } else {
        pCodecCtx->width = mOutputImageParams.mWidth;
    }
    
    if (mOutputImageParams.mHeight < 0) {
        pCodecCtx->height = -mOutputImageParams.mHeight;
    } else {
        pCodecCtx->height = mOutputImageParams.mHeight;
    }

    pCodecCtx->pix_fmt = getImageInnerFormat(mOutputImageParams.mPixFmt);
    pCodecCtx->codec_id = mPMediaEncoder->mPVideoCodec->id;
    pCodecCtx->codec_type = mPMediaEncoder->mPVideoCodec->type;
    pCodecCtx->gop_size = 250;
    pCodecCtx->framerate.den = 30;
    pCodecCtx->framerate.num = 1;// {30, 1};
    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 30;
    pCodecCtx->keyint_min = 60;
    pCodecCtx->bit_rate = mOutputImageParams.mBitRate;

    if (pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
        pCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }

//    av_dict_set(&opts, "profile", "baseline", 0);
    if (pCodecCtx->codec_id == AV_CODEC_ID_H264) {
        av_opt_set(pCodecCtx->priv_data, "level", "4.1", 0);
        av_opt_set(pCodecCtx->priv_data, "preset", "superfast", 0);
        av_opt_set(pCodecCtx->priv_data, "tune", "zerolatency", 0);
    }
    
    av_dict_set(&opts, "threads", "auto", 0);
    HBError = avcodec_open2(pCodecCtx, mPMediaEncoder->mPVideoCodec, &opts);
    if (HBError < 0) {
        av_dict_free(&opts);
        LOGE("Avcodec open output failed !");
        goto OUTPUT_INITIAL_END_LABEL;
    }
    av_dict_free(&opts);
    
    HBError = avcodec_parameters_from_context(pVideoStream->codecpar, pCodecCtx);
    if (HBError < 0) {
        LOGE("AVcodec Copy context paramter error!\n");
        goto OUTPUT_INITIAL_END_LABEL;
    }
    
    mOutputImageParams.mBitRate = pVideoStream->codecpar->bit_rate;
    mOutputImageParams.mWidth = pVideoStream->codecpar->width;
    mOutputImageParams.mHeight = pVideoStream->codecpar->height;
    mOutputImageParams.mPixFmt = getImageExternFormat((AVPixelFormat)(pVideoStream->codecpar->format));
    mOutputImageParams.mFrameRate = mInputImageParams.mFrameRate;
    mOutputImageParams.mPreImagePixBufferSize = av_image_get_buffer_size((AVPixelFormat)(pVideoStream->codecpar->format), \
                                      mOutputImageParams.mWidth, mOutputImageParams.mHeight, mOutputImageParams.mAlign);
    av_dump_format(pFormatCtx, mPMediaEncoder->mVideoStreamIndex, mOutputMediaFile, 1);
    return HB_OK;
    
OUTPUT_INITIAL_END_LABEL:
    if (mPMediaEncoder->mPVideoCodecCtx) {
        avcodec_close(mPMediaEncoder->mPVideoCodecCtx);
        avcodec_free_context(&(mPMediaEncoder->mPVideoCodecCtx));
    }
    mPMediaEncoder->mPVideoCodec = nullptr;
    if (mPMediaEncoder->mPVideoFormatCtx) {
        avformat_close_input(&(mPMediaEncoder->mPVideoFormatCtx));
        mPMediaEncoder->mPVideoFormatCtx = nullptr;
    }
    return HB_ERROR;
}

int VideoFormatTranser::_SwsMediaInitial() {
    if (mInputImageParams.mWidth != mOutputImageParams.mWidth \
        || mInputImageParams.mHeight != mOutputImageParams.mHeight \
        || mInputImageParams.mPixFmt != mOutputImageParams.mPixFmt)
    {
        mPVideoConvertCtx = sws_getContext(mInputImageParams.mWidth, mInputImageParams.mHeight,               getImageInnerFormat(mInputImageParams.mPixFmt), \
                                mOutputImageParams.mWidth, mOutputImageParams.mHeight, getImageInnerFormat(mOutputImageParams.mPixFmt), \
                                SWS_BICUBIC, NULL, NULL, NULL);
        if (!mPVideoConvertCtx) {
            LOGE("Create video format sws context failed !");
            return HB_ERROR;
        }
    }
    return HB_OK;
}
    
VideoFormatTranser::~VideoFormatTranser() {
    if (mInputMediaFile)
        av_freep(&mInputMediaFile);
    if (mOutputMediaFile)
        av_freep(&mOutputMediaFile);
}

void VideoFormatTranser::setInputVideoMediaFile(char *pFilePath) {
    if (mInputMediaFile)
        av_freep(&mInputMediaFile);
    mInputMediaFile = av_strdup(pFilePath);
}

void VideoFormatTranser::setVideoOutputFrameRate(float frameRate) {
    mOutputImageParams.mFrameRate = frameRate;
}

void VideoFormatTranser::setVideoOutputBitrate(int64_t bitrate) {
    mOutputImageParams.mBitRate = bitrate;
}

void VideoFormatTranser::setVideoOutputSize(int width, int height) {
    mOutputImageParams.mWidth = width;
    mOutputImageParams.mHeight = height;
}

void VideoFormatTranser::setOutputVideoMediaFile(char *pFilePath) {
    if (mOutputMediaFile)
        av_freep(&mOutputMediaFile);
    mOutputMediaFile = av_strdup(pFilePath);
}

int VideoFormatTranser::_WorkPthreadPrepare() {
    if (!mIsSyncMode) {
        int HBError = 0;
        mEncodeFrameQueue = new FiFoQueue<AVFrame *>(MAX_FRAME_BUFFER);
        mDecodePacketQueue = new FiFoQueue<AVPacket *>(MAX_FRAME_BUFFER);
        mOutputPacketQueue = new FiFoQueue<AVPacket *>(MAX_FRAME_BUFFER);
        
        if (!mEncodeFrameQueue || !mDecodePacketQueue || !mOutputPacketQueue) {
            LOGE("malloc convert queue failed !");
            return HB_ERROR;
        }
        
        ThreadIpcCtxInitial(&mReadThreadIpcCtx);
        ThreadIpcCtxInitial(&mDecodeThreadIpcCtx);
        ThreadIpcCtxInitial(&mEncodeThreadIpcCtx);
        
        HBError = pthread_create(&mDecodeThreadId, NULL, DecodeThreadFunc, this);
        if (HBError < 0) {
            LOGE("Create decode thread failed !");
            return HB_ERROR;
        }
        HBError = pthread_create(&mEncodeThreadId, NULL, EncodeThreadFunc, this);
        if (HBError < 0) {
            LOGE("Create decode thread failed !");
            return HB_ERROR;
        }
    }
    return HB_OK;
}

int VideoFormatTranser::_WorkPthreadDispose() {
    if (mIsSyncMode)
        return HB_OK;
    
    if (mDecodeThreadId) {
        pthread_join(mDecodeThreadId, NULL);
        mDecodeThreadId = nullptr;
    }
    
    if (mEncodeThreadId) {
        pthread_join(mEncodeThreadId, NULL);
        mEncodeThreadId = nullptr;
    }
    
    if (mEncodeFrameQueue) {
        AVFrame *pNewFrame = nullptr;
        while (mEncodeFrameQueue->queueLength() > 0) {
            pNewFrame = mEncodeFrameQueue->get();
            av_frame_free(&pNewFrame);
        }
        mEncodeFrameQueue = nullptr;
    }
    
    if (mDecodePacketQueue) {
        AVPacket *pNewPacket = nullptr;
        while (mDecodePacketQueue->queueLength() > 0) {
            pNewPacket = mDecodePacketQueue->get();
            av_packet_free(&pNewPacket);
        }
        mDecodePacketQueue = nullptr;
    }
    
    if (mOutputPacketQueue) {
        AVPacket *pNewPacket = nullptr;
        while (mOutputPacketQueue->queueLength() > 0) {
            pNewPacket = mOutputPacketQueue->get();
            av_packet_free(&pNewPacket);
        }
        mOutputPacketQueue = nullptr;
    }

    return HB_OK;
}
    
int VideoFormatTranser::setSyncMode(bool mode) {
    mIsSyncMode = mode;
    return HB_OK;
}

}
