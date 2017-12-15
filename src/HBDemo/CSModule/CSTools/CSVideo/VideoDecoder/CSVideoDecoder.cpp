//
//  CSVideoDecoder.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/1.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSVideoDecoder.h"

namespace HBMedia {
    
static void EchoStatus(uint64_t status) {

    bool bReadEnd = ((status & DECODE_STATE_READPKT_END) != 0 ? true : false);
    bool bDecodeEnd = ((status & DECODE_STATE_DECODE_END) != 0 ? true : false);
    bool bReadAbort = ((status & DECODE_STATE_READPKT_ABORT) != 0 ? true : false);
    bool bDecodeAbort = ((status & DECODE_STATE_DECODE_ABORT) != 0 ? true : false);
    bool bFlushMode = ((status & DECODE_STATE_FLUSH_MODE) != 0 ? true : false);
    
    LOGI("[Work task: <Decoder>] Status: Read<End:%d, Abort:%d> | <Flush:%d> | Decode<End:%d, Abort:%d>", bReadEnd, bReadAbort, bFlushMode, bDecodeEnd, bDecodeAbort);
}

int CSVideoDecoder::S_MAX_BUFFER_CACHE = 8;
void* CSVideoDecoder::ThreadFunc_Video_Decoder(void *arg) {
    if (!arg) {
        LOGE("[Work task: <Decoder>] Thread param args is invalid !");
        return nullptr;
    }
    
    int HBError = HB_OK;
    
    ThreadParam_t *pThreadParams = (ThreadParam_t *)arg;
    CSVideoDecoder* pDecoder = (CSVideoDecoder *)(pThreadParams->mThreadArgs);
    AVPacket *pNewPacket = av_packet_alloc();
    AVFrame  *pNewFrame = av_frame_alloc();
    AVFrame *pTargetFrame = nullptr;
    while (!(pDecoder->mState & DECODE_STATE_DECODE_END)) {
        /** 初始化区域 */
        pTargetFrame = nullptr;
        
        if (pDecoder->mAbort) {
            pDecoder->mState |= (DECODE_STATE_DECODE_ABORT | DECODE_STATE_DECODE_END);
            LOGE("[Work task: <Decoder>] Abort decoder process !");
            break;
        }
        
        if (!(pDecoder->mState & DECODE_STATE_DECODE_ABORT) \
            && !(pDecoder->mState & DECODE_STATE_READPKT_END))
        {
            av_init_packet(pNewPacket);
            HBError = av_read_frame(pDecoder->mPInVideoFormatCtx, pNewPacket);
            if (HBError != 0) {
                if (HBError != AVERROR_EOF) {
                    LOGE("[Work task: <Decoder>] Read frame failed, Err:%s", av_err2str(HBError));
                    pDecoder->mState |= DECODE_STATE_READPKT_ABORT;
                }
                pDecoder->mState |= DECODE_STATE_READPKT_END;
                av_packet_free(&pNewPacket);
                pNewPacket = nullptr;
                continue;
            }
            /** 拿到媒体数据包 */
            if (pNewPacket->stream_index != pDecoder->mVideoStreamIndex) {
                av_packet_unref(pNewPacket);
                continue;
            }
            /** 得到想要的指定数据类型的数据包 */
            if (pNewPacket) {
                HBError = avcodec_send_packet(pDecoder->mPInputVideoCodecCtx, pNewPacket);
                av_packet_unref(pNewPacket);
                if (HBError != 0) {
                    if (HBError != AVERROR(EAGAIN)) {
                        LOGE("[Work task: <Decoder>] Send packet failed, Err:%s", av_err2str(HBError));
                        pDecoder->mState |= DECODE_STATE_DECODE_ABORT;
                        pDecoder->mState |= DECODE_STATE_DECODE_END;
                    }
                    else
                        LOGE("[Work task: <Decoder>] Send packet require eagain, desc:%s", av_err2str(HBError));
                    continue;
                }
            }
        }
        else {
            if (pNewPacket)
                av_packet_free(&pNewPacket);
            
            if (!(pDecoder->mState & DECODE_STATE_FLUSH_MODE)) {
                if (!(pDecoder->mState & DECODE_STATE_READPKT_END)) {
                    LOGE("[Work task: <Decoder>] Flush decoder buffer, but the not reach the end of file !");
                }
                pDecoder->mState |= DECODE_STATE_FLUSH_MODE;
                avcodec_send_packet(pDecoder->mPInputVideoCodecCtx, nullptr);
                continue;
            }
        }
        
        while (true) {
            HBError = avcodec_receive_frame(pDecoder->mPInputVideoCodecCtx, pNewFrame);
            if (HBError != 0) {
                if (HBError != AVERROR(EAGAIN)) {
                    if (HBError != AVERROR_EOF) {
                        LOGE("[Work task: <Decoder>] Receive frame, desc:%s", av_err2str(HBError));
                        pDecoder->mState |= DECODE_STATE_DECODE_ABORT;
                    }
                    av_frame_unref(pNewFrame);
                }
                if (pDecoder->mState & DECODE_STATE_FLUSH_MODE)
                    pDecoder->mState |= DECODE_STATE_DECODE_END;
                break;
            }
            
            if (pDecoder->mIsNeedTransfer) {
                /** 执行格式转换 */
                if (pDecoder->_DoSwscale(pNewFrame, &pTargetFrame) != HB_OK) {
                    av_frame_unref(pNewFrame);
                    continue;
                }
            }
            else {
                pTargetFrame = av_frame_clone(pNewFrame);
                if (!pTargetFrame) {
                    av_frame_unref(pNewFrame);
                    LOGE("Clone target frame failed !");
                    continue;
                }
                pTargetFrame->format = pNewFrame->format;
                pTargetFrame->width = pNewFrame->width;
                pTargetFrame->height = pNewFrame->height;
            }
            av_frame_unref(pNewFrame);
            
            if (pDecoder->_DoExport(&pTargetFrame) != HB_OK) {
                if (pTargetFrame) {
                    if (pTargetFrame->opaque)
                        av_freep(pTargetFrame->opaque);
                    av_frame_free(&pTargetFrame);
                }
            }
        }
    }

VIDEO_DECODER_THREAD_END_LABEL:
    EchoStatus(pDecoder->mState);
    if (pNewPacket)
        av_packet_free(&pNewPacket);
    if (pNewFrame) {
        if (pNewFrame->opaque)
            av_freep(pNewFrame->opaque);
        av_frame_free(&pNewFrame);
    }
    if (pTargetFrame) {
        if (pTargetFrame->opaque)
            av_freep(pTargetFrame->opaque);
        av_frame_free(&pTargetFrame);
    }
    
    return arg;
}

int  CSVideoDecoder::_DoExport(AVFrame **pOutFrame) {
    AVFrame *pFrame = *pOutFrame;
    if (!pFrame) {
        LOGE("[Work task: <Decoder>] Do export frame failed, invalid params !");
        return HB_ERROR;
    }
    
    switch (mOutMediaType) {
        case MD_TYPE_RAW_BY_MEMORY:
        {
            if (mTargetFrameQueue) {
                /** 阻塞等待空间帧缓冲区存在可用资源 */
                mEmptyFrameQueueIPC->condV();
                {
                    /** 重新计算时间 */
                    (*pOutFrame)->pts = (int64_t)av_rescale_q((*pOutFrame)->pts, \
                                                     mPInVideoFormatCtx->streams[mVideoStreamIndex]->time_base, AV_TIME_BASE_Q);
                }
                if (mTargetFrameQueue->push(*pOutFrame) > 0) {
//                    LOGD("[Work task: <Decoder>] Push frame:%lld, %lf !", (*pOutFrame)->pts, ((*pOutFrame)->pts * av_q2d(AV_TIME_BASE_Q)));
                    mTargetFrameQueueIPC->condP();
                }
                else {/** push 帧失败 */
                    LOGE("[Work task: <Decoder>] Push valid frame data to queue failed !");
                    mEmptyFrameQueueIPC->condP();
                    if (pFrame->opaque)
                        av_freep(pFrame->opaque);
                    av_frame_free(pOutFrame);
                }
            }
            else {
                LOGE("[Work task: <Decoder>] Frame buffer queue is invalid !");
                if (pFrame->opaque)
                    av_freep(pFrame->opaque);
                av_frame_free(pOutFrame);
                break;
            }
            
            return HB_OK;
        }
            break;
        case MD_TYPE_RAW_BY_FILE:
        {
            if (fwrite(pFrame->data[0], 1, mTargetVideoParams.mPreImagePixBufferSize, mTrgMediaFileHandle) <= 0) {
                LOGE("[Work task: <Decoder>] Write output frame failed !");
                return HB_ERROR;
            }
            if (pFrame->opaque)
                av_freep(pFrame->opaque);
            av_frame_free(pOutFrame);
            return HB_OK;
        }
            break;
        default:
            break;
    }
    
    return HB_ERROR;
}

CSVideoDecoder::CSVideoDecoder() {
    memset(&mState, 0x00, sizeof(mState));
    imageParamInit(&mSrcVideoParams);
    imageParamInit(&mTargetVideoParams);

    mVideoStreamIndex = INVALID_STREAM_INDEX;
    mPInVideoFormatCtx = nullptr;

    mPInputVideoCodecCtx = nullptr;
    mPInputVideoCodec = nullptr;
    mPVideoConvertCtx = nullptr;
    mTargetFrameQueue = new FiFoQueue<AVFrame *>(S_MAX_BUFFER_CACHE);
    mTargetFrameQueueIPC = new ThreadIPCContext(0);
    mEmptyFrameQueueIPC = new ThreadIPCContext(S_MAX_BUFFER_CACHE);
    mDecodeThreadCtx.setFunction(nullptr, nullptr);
}

CSVideoDecoder::~CSVideoDecoder() {

}

int CSVideoDecoder::prepare() {
    
    if (baseInitial() != HB_OK) {
        LOGE("Video base initial failed !");
        goto VIDEO_DECODER_PREPARE_END_LABEL;
    }
    
    if (_mediaParamInitial() != HB_OK) {
        LOGE("Check Video decoder param failed !");
        goto VIDEO_DECODER_PREPARE_END_LABEL;
    }

    if (_DecoderInitial() != HB_OK) {
        LOGE("Video decoder initial failed !");
        goto VIDEO_DECODER_PREPARE_END_LABEL;
    }
    
    if (_ExportInitial() != HB_OK) {
        LOGE("Video decoder open failed !");
        goto VIDEO_DECODER_PREPARE_END_LABEL;
    }
    
    if (_SwscaleInitial() != HB_OK) {
        LOGE("Video swscale initail failed !");
        goto VIDEO_DECODER_PREPARE_END_LABEL;
    }
    
    mState |= DECODE_STATE_PREPARED;
    return HB_OK;
    
VIDEO_DECODER_PREPARE_END_LABEL:
    release();
    return HB_ERROR;
}
    
int CSVideoDecoder::start() {
    if (!(mState & DECODE_STATE_PREPARED)) {
        LOGE("Media decoder is not prepared !");
        return HB_ERROR;
    }
    if (HB_OK != mDecodeThreadCtx.setFunction(ThreadFunc_Video_Decoder, this)) {
        LOGE("Initial decode thread context failed !");
        return HB_ERROR;
    }
    
    if (mDecodeThreadCtx.start() != HB_OK) {
        LOGE("Start decoder thread context failed !");
        return HB_ERROR;
    }
    
    return HB_OK;
}
    
int CSVideoDecoder::stop() {
    if (!(mState & DECODE_STATE_DECODE_END)) {
        mAbort = true;
    }
    mDecodeThreadCtx.join();
    
    if (CSMediaBase::stop() != HB_OK) {
        LOGE("Media base stop failed !");
        return HB_ERROR;
    }
    
    AVFrame *pFrame = nullptr;
    while (mTargetFrameQueue->queueLength()) {
        pFrame = mTargetFrameQueue->get();
        if (pFrame) {
            if (pFrame->opaque)
                av_freep(pFrame);
            av_frame_free(&pFrame);
        }
    }
    
    if (release() != HB_OK) {
        LOGE("decoder release failed !");
        return HB_ERROR;
    }
    return HB_OK;
}

int CSVideoDecoder::syncWait() {
    while (!(mState & DECODE_STATE_DECODE_END)) {
        usleep(100);
    }
    stop();
    return HB_OK;
}

int  CSVideoDecoder::_DoSwscale(AVFrame *pInFrame, AVFrame **pOutFrame) {
    if (!pOutFrame) {
        LOGE("Video decoder do swscale invalid args !");
        return HB_ERROR;
    }
    
    uint8_t *pTargetImageBuffer = nullptr;
    *pOutFrame = av_frame_alloc();
    if (!(*pOutFrame)) {
        LOGE("Alloc new output frame failed !");
        goto DO_SWSCALE_END_LABEL;
    }
    
    pTargetImageBuffer = (uint8_t *)av_mallocz(mTargetVideoParams.mPreImagePixBufferSize);
    if (!pTargetImageBuffer) {
        LOGE("malloc target image buffer failed !");
        goto DO_SWSCALE_END_LABEL;
    }
    
    av_image_fill_arrays((*pOutFrame)->data, (*pOutFrame)->linesize,\
                         pTargetImageBuffer, getImageInnerFormat(mTargetVideoParams.mPixFmt),\
                         mTargetVideoParams.mWidth, mTargetVideoParams.mHeight, mTargetVideoParams.mAlign);
    
    pTargetImageBuffer = nullptr;
    if (sws_scale(mPVideoConvertCtx, pInFrame->data, pInFrame->linesize,\
                  0, mSrcVideoParams.mHeight, (*pOutFrame)->data, (*pOutFrame)->linesize) <= 0) {
        LOGE("swscale to target frame format failed !");
        goto DO_SWSCALE_END_LABEL;
    }
    (*pOutFrame)->pts = pInFrame->pts;
    (*pOutFrame)->format = getImageInnerFormat(mTargetVideoParams.mPixFmt);
    (*pOutFrame)->width = mTargetVideoParams.mWidth;
    (*pOutFrame)->height = mTargetVideoParams.mHeight;
    return HB_OK;
    
DO_SWSCALE_END_LABEL:
    if (*pOutFrame) {
        av_frame_free(pOutFrame);
        *pOutFrame = nullptr;
    }
    if (pTargetImageBuffer)
        av_free(pTargetImageBuffer);
    
    return HB_ERROR;
}

int CSVideoDecoder::receiveFrame(AVFrame **OutFrame) {
    int HBError = HB_ERROR;
    if (!OutFrame) {
        LOGE("Video Decoder >>> invalid params !");
        return HBError;
    }
    *OutFrame = nullptr;
    if (!(mState & DECODE_STATE_PREPARED) || mOutMediaType != MD_TYPE_RAW_BY_MEMORY) {
        LOGE("Video Decoder >>> receive raw frame failed, invalid output media type !");
        return HBError;
    }

RETRY_RECEIVE_FRAME:
    AVFrame *pNewFrame = nullptr;
    HBError = HB_ERROR;
    if (mTargetFrameQueue && mTargetFrameQueue->queueLength() > 0) {
        if (!(pNewFrame = mTargetFrameQueue->get())) {
            LOGE("Video Decoder [%d] >>> receive failed failed, <frame:%d>!", __LINE__, mTargetFrameQueue->queueLength());
            goto RETRY_RECEIVE_FRAME;
        }
        else {
            HBError = HB_OK;
            mTargetFrameQueueIPC->condV();
            mEmptyFrameQueueIPC->condP();
        }
    }
//    else
//        LOGE("Video decoder >>> cur not valid decoder data: %d!", mTargetFrameQueue->queueLength());
    
    *OutFrame = pNewFrame;
    return HBError;
}

int  CSVideoDecoder::_DecoderInitial() {
    
    if (mInMediaType == MD_TYPE_COMPRESS) {
        int HBError = HB_OK;
        AVStream* pVideoStream = nullptr;
        mPInputVideoCodecCtx = nullptr;
        memset(&mState, 0x00, sizeof(mState));
        
        if (CSMediaBase::_InMediaInitial() != HB_OK) {
            LOGE("Media base in media initial failed !");
            goto VIDEO_DECODER_INITIAL_END_LABEL;
        }
        
        mVideoStreamIndex = av_find_best_stream(mPInVideoFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
        if (mVideoStreamIndex < 0) {
            LOGW("Video decoder counldn't find valid audio stream, %s", av_err2str(mVideoStreamIndex));
            goto VIDEO_DECODER_INITIAL_END_LABEL;
        }
        
        pVideoStream = mPInVideoFormatCtx->streams[mVideoStreamIndex];
        mPInputVideoCodec = avcodec_find_decoder(pVideoStream->codecpar->codec_id);
        if (!mPInputVideoCodec) {
            LOGE("Codec <%d> not found !", pVideoStream->codecpar->codec_id);
            goto VIDEO_DECODER_INITIAL_END_LABEL;
        }
        
        mPInputVideoCodecCtx = avcodec_alloc_context3(mPInputVideoCodec);
        if (!mPInputVideoCodecCtx) {
            LOGE("Codec ctx <%d> not found !", pVideoStream->codecpar->codec_id);
            goto VIDEO_DECODER_INITIAL_END_LABEL;
        }
        avcodec_parameters_to_context(mPInputVideoCodecCtx, pVideoStream->codecpar);
        
        HBError = avcodec_open2(mPInputVideoCodecCtx, mPInputVideoCodec, NULL);
        if (HBError != 0) {
            LOGE("Could not open input codec context. <%s>", av_err2str(HBError));
            goto VIDEO_DECODER_INITIAL_END_LABEL;
        }
        
        /** 初始化原参数信息 */
        mSrcVideoParams.mPreImagePixBufferSize = av_image_get_buffer_size(mPInputVideoCodecCtx->pix_fmt, \
                                          mPInputVideoCodecCtx->width, mPInputVideoCodecCtx->height, mSrcVideoParams.mAlign);
        mSrcVideoParams.mWidth = mPInputVideoCodecCtx->width;
        mSrcVideoParams.mHeight = mPInputVideoCodecCtx->height;
        mSrcVideoParams.mPixFmt = getImageExternFormat(mPInputVideoCodecCtx->pix_fmt);
        
        av_dump_format(mPInVideoFormatCtx, mVideoStreamIndex, mSrcMediaFile, false);
    }
    
    return HB_OK;
    
VIDEO_DECODER_INITIAL_END_LABEL:
    if (mPInputVideoCodec)
        avcodec_free_context(&mPInputVideoCodecCtx);
    mPInputVideoCodec = nullptr;
    CSMediaBase::release();
    return HB_ERROR;
}

int CSVideoDecoder::_SwscaleInitial() {
    if (mTargetVideoParams.mPixFmt == CS_PIX_FMT_NONE)
        mTargetVideoParams.mPixFmt = mSrcVideoParams.mPixFmt;
    if (mTargetVideoParams.mWidth == 0 || mTargetVideoParams.mHeight == 0) {
        mTargetVideoParams.mWidth = mSrcVideoParams.mWidth;
        mTargetVideoParams.mHeight = mSrcVideoParams.mHeight;
    }
    mTargetVideoParams.mPreImagePixBufferSize = av_image_get_buffer_size(getImageInnerFormat(mTargetVideoParams.mPixFmt),\
                                             mTargetVideoParams.mWidth, mTargetVideoParams.mHeight, mTargetVideoParams.mAlign);
    if (!mTargetVideoParams.mPreImagePixBufferSize) {
        LOGE("Get target image buffer size failed, size:%d !", mTargetVideoParams.mPreImagePixBufferSize);
        goto VIDEO_SWSCALE_INITIAL_END_LABEL;
    }

    mPVideoConvertCtx = nullptr;
    mIsNeedTransfer = false;
    if (mTargetVideoParams.mPixFmt != mSrcVideoParams.mPixFmt \
        || mTargetVideoParams.mWidth != mSrcVideoParams.mWidth \
        || mTargetVideoParams.mHeight != mSrcVideoParams.mHeight) {
        mIsNeedTransfer = true;
        
        mPVideoConvertCtx = sws_getContext(mSrcVideoParams.mWidth, mSrcVideoParams.mHeight, getImageInnerFormat(mSrcVideoParams.mPixFmt), \
                                           mTargetVideoParams.mWidth, mTargetVideoParams.mHeight, getImageInnerFormat(mTargetVideoParams.mPixFmt), \
                                           SWS_BICUBIC, NULL, NULL, NULL);
        if (!mPVideoConvertCtx) {
            LOGE("Create video sws context failed !");
            goto VIDEO_SWSCALE_INITIAL_END_LABEL;
        }
    }
    
    return HB_OK;
VIDEO_SWSCALE_INITIAL_END_LABEL:
    if (mPVideoConvertCtx) {
        sws_freeContext(mPVideoConvertCtx);
        mPVideoConvertCtx = nullptr;
    }
    
    return HB_ERROR;
}

int  CSVideoDecoder::_mediaParamInitial() {

    mIsNeedTransfer = false;

    switch (mInMediaType) {
        case MD_TYPE_UNKNOWN: {
            if (mSrcMediaFile)
                mInMediaType = MD_TYPE_COMPRESS;
            else {
                LOGE("[%s] >>> [Type:%s] Video decoder input file is invalid !", __func__, getMediaDataTypeDescript(mInMediaType));
                return HB_ERROR;
            }
        }
            break;
        case MD_TYPE_COMPRESS:
            if (!mSrcMediaFile) {
                LOGE("[%s] >>> [Type:%s]Video decoder input file is invalid !", __func__, getMediaDataTypeDescript(mInMediaType));
                return HB_ERROR;
            }
            break;
        default:
            LOGE("Unknown media type !");
            return HB_ERROR;
    }
    
    switch (mOutMediaType) {
        case MD_TYPE_UNKNOWN: {
            if (mTrgMediaFile)
                mOutMediaType = MD_TYPE_RAW_BY_FILE;
            else
                mOutMediaType = MD_TYPE_RAW_BY_MEMORY;
        }
            break;
        case MD_TYPE_RAW_BY_FILE:
            if (!mTrgMediaFile) {
                LOGE("[%s] >>> [Type:%s]Video decoder output file is invalid !", __func__, getMediaDataTypeDescript(mOutMediaType));
                return HB_ERROR;
            }
            break;
        case MD_TYPE_RAW_BY_MEMORY:
            if (!mTargetFrameQueue || !mEmptyFrameQueueIPC) {
                LOGE("[%s] >>> [Type:%s]Video decoder output prepare failed !", __func__, getMediaDataTypeDescript(mOutMediaType));
                return HB_ERROR;
            }
            break;
        case MD_TYPE_COMPRESS:
            LOGE("Cur work mode is not support this output type !");
            return HB_ERROR;
            
        default:
            LOGE("Unknown media type !");
            return HB_ERROR;
    }
    
    return HB_OK;
}

int CSVideoDecoder::_ExportInitial() {
    switch (mOutMediaType) {
        case MD_TYPE_RAW_BY_FILE:
            {
                if (CSMediaBase::_OutMediaInitial() != HB_OK) {
                    LOGE("Base media initial output media initial failed !");
                    return HB_ERROR;
                }
            }
            break;
        case MD_TYPE_RAW_BY_MEMORY:
        {
            if (!mTargetFrameQueueIPC || !mTargetFrameQueue) {
                LOGE("Video decoder output buffer module initial failed !");
                return HB_ERROR;
            }
        }
            break;
        default:
            LOGE("output media initial failed, cur media type:%s", getMediaDataTypeDescript(mOutMediaType));
            return HB_ERROR;
    }

    return HB_OK;
}

int CSVideoDecoder::release() {
    if (mPInputVideoCodec)
        avcodec_free_context(&mPInputVideoCodecCtx);
    mPInputVideoCodec = nullptr;
    if (mPVideoConvertCtx) {
        sws_freeContext(mPVideoConvertCtx);
        mPVideoConvertCtx = nullptr;
    }
    CSMediaBase::release();
    return HB_OK;
}

}
