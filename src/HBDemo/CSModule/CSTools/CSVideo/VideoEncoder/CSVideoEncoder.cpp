//
//  CSVideoEncoder.cpp
//  Sample
//
//  Created by zj-db0519 on 2017/12/8.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSVideoEncoder.h"

namespace HBMedia {

int CSVideoEncoder::S_MAX_BUFFER_CACHE = 8;

static void EchoStatus(uint64_t status) {
    
    bool bReadEnd = ((status & ENCODE_STATE_READPKT_END) != 0 ? true : false);
    bool bEncodeEnd = ((status & ENCODE_STATE_ENCODE_END) != 0 ? true : false);
    bool bReadAbort = ((status & ENCODE_STATE_READPKT_ABORT) != 0 ? true : false);
    bool bEncodeAbort = ((status & ENCODE_STATE_ENCODE_ABORT) != 0 ? true : false);
    bool bFlushMode = ((status & ENCODE_STATE_FLUSH_MODE) != 0 ? true : false);
    
    LOGI("[Work task: <Encoder>] Status: Read<End:%d, Abort:%d> | <Flush:%d> | encode<End:%d, Abort:%d>", bReadEnd, bReadAbort, bFlushMode, bEncodeEnd, bEncodeAbort);
}

void* CSVideoEncoder::ThreadFunc_Video_Encoder(void *arg) {
    if (!arg) {
        LOGE("[Work task: <Decoder>] Thread param args is invalid !");
        return nullptr;
    }
    
    int HBError = HB_OK;
    ThreadParam_t *pThreadParams = (ThreadParam_t *)arg;
    CSVideoEncoder* pEncoder = (CSVideoEncoder *)(pThreadParams->mThreadArgs);
    AVPacket *pNewPacket = av_packet_alloc();
    AVFrame *pInFrame = nullptr;
    AVFrame *pOutFrame = nullptr;
    
    while (S_NOT_EQ(pEncoder->mState, ENCODE_STATE_READPKT_END) \
           || (pEncoder->mSrcFrameQueue->queueLength() > 0))
    {
        pInFrame = nullptr;
        pOutFrame = nullptr;
        
        if (S_EQ(pEncoder->mState, ENCODE_STATE_ENCODE_ABORT))
            break;
        
        pEncoder->mSrcFrameQueueIPC->condV();
        if (!(pInFrame = pEncoder->mSrcFrameQueue->get())) {
            LOGE("[Work task: <Encoder:%p>] get queue[%p] length:%d frame failed !", pEncoder, \
                 pEncoder->mSrcFrameQueue, pEncoder->mSrcFrameQueue->queueLength());
            continue;
        }
        pEncoder->mEmptyFrameQueueIPC->condP();
        
        if (pEncoder->mIsNeedTransfer) {
            if (pEncoder->_DoSwscale(pInFrame, &pOutFrame) != HB_OK) {
                
                if (pInFrame->opaque)
                    av_freep(pInFrame->opaque);
                av_frame_free(&pInFrame);
                continue;
            }
            
            if (pInFrame->opaque)
                av_freep(pInFrame->opaque);
            av_frame_free(&pInFrame);
        }
        else {
            pOutFrame = pInFrame;
            pInFrame = nullptr;
            
            pOutFrame->format = getImageInnerFormat(pEncoder->mTargetVideoParams.mPixFmt);
            pOutFrame->width = pEncoder->mTargetVideoParams.mWidth;
            pOutFrame->height = pEncoder->mTargetVideoParams.mHeight;
        }
        
        if (pOutFrame) {
            /** 传入的文件，默认以 AV_TIME_BASE_Q 为实践基传入, 在这里对 它的时间基进行本地转换 */
            pOutFrame->pts = av_rescale_q(pOutFrame->pts, AV_TIME_BASE_Q, pEncoder->mPOutMediaFormatCtx->streams[pEncoder->mVideoStreamIndex]->time_base);
        }
        
        HBError = avcodec_send_frame(pEncoder->mPOutVideoCodecCtx, pOutFrame);
        if (pOutFrame->opaque)
            av_freep(pOutFrame->opaque);
        av_frame_free(&pOutFrame);
        
        if (HBError != 0) {
            if (HBError != AVERROR(EAGAIN)) {
                LOGE("[Work task: <Encoder>] Send frame failed, Err:%s", av_err2str(HBError));
                pEncoder->mState |= ENCODE_STATE_ENCODE_ABORT;
            }
            continue;
        }
        
        while (true) {
            HBError = avcodec_receive_packet(pEncoder->mPOutVideoCodecCtx, pNewPacket);
            if (HBError == 0) {
                pNewPacket->stream_index = pEncoder->mVideoStreamIndex;
                HBError = pEncoder->_DoExport(pNewPacket);
                if (HBError != HB_OK) {
                    av_packet_unref(pNewPacket);
                }
            }
            else {
                if (HBError == AVERROR(EAGAIN))
                    break;
                else if (HBError != AVERROR_EOF) {
                    LOGE("[Work task: <Encoder>] Receive packet failed, Err:%s", av_err2str(HBError));
                    pEncoder->mState |= ENCODE_STATE_ENCODE_ABORT;
                }
            }
        }
    }
    
    pEncoder->_flush();
    
VIDEO_ENCODER_THREAD_END_LABEL:
    if ((HBError = av_write_trailer(pEncoder->mPOutMediaFormatCtx)) != 0)
        LOGE("[Work task: <Encoder>] write trail failed, %s !", av_err2str(HBError));

    avcodec_close(pEncoder->mPOutVideoCodecCtx);
    avio_close(pEncoder->mPOutMediaFormatCtx->pb);
    avformat_free_context(pEncoder->mPOutMediaFormatCtx);
    pEncoder->mPOutMediaFormatCtx = nullptr;
    
    pEncoder->mState |= ENCODE_STATE_ENCODE_END;
    av_packet_free(&pNewPacket);
    
    if (pInFrame) {
        if (pInFrame->opaque)
            av_freep(pInFrame->opaque);
        av_frame_free(&pInFrame);
    }
    if (pOutFrame) {
        if (pOutFrame->opaque)
            av_freep(pOutFrame->opaque);
        av_frame_free(&pOutFrame);
    }
    return nullptr;
}

CSVideoEncoder::CSVideoEncoder() {
    memset(&mState, 0x00, sizeof(mState));
    imageParamInit(&mSrcVideoParams);
    imageParamInit(&mTargetVideoParams);
    
    mVideoStreamIndex = INVALID_STREAM_INDEX;
    
    mSrcFrameQueue = new FiFoQueue<AVFrame *>(S_MAX_BUFFER_CACHE);
    mSrcFrameQueueIPC = new ThreadIPCContext(0);
    mEmptyFrameQueueIPC = new ThreadIPCContext(S_MAX_BUFFER_CACHE);
    mEncodeThreadCtx.setFunction(nullptr, nullptr);
}

CSVideoEncoder::~CSVideoEncoder() {
    
}

int CSVideoEncoder::prepare() {
    if (baseInitial() != HB_OK) {
        LOGE("Video base initial failed !");
        goto VIDEO_ENCODER_PREPARE_END_LABEL;
    }
    
    if (_mediaParamInitial() != HB_OK) {
        LOGE("Check Video encoder param failed !");
        goto VIDEO_ENCODER_PREPARE_END_LABEL;
    }
    
    if (_InputInitial() != HB_OK) {
        LOGE("Video decoder open failed !");
        goto VIDEO_ENCODER_PREPARE_END_LABEL;
    }
    
    if (_SwscaleInitial() != HB_OK) {
        LOGE("Video swscale initail failed !");
        goto VIDEO_ENCODER_PREPARE_END_LABEL;
    }
    
    if (_EncoderInitial() != HB_OK) {
        LOGE("Video encoder initial failed !");
        goto VIDEO_ENCODER_PREPARE_END_LABEL;
    }
    
    mState |= ENCODE_STATE_PREPARED;
    return HB_OK;
    
VIDEO_ENCODER_PREPARE_END_LABEL:
    release();
    return HB_ERROR;
}

int CSVideoEncoder::start() {
    if (!(mState & ENCODE_STATE_PREPARED)) {
        LOGE("Media video encoder is not prepared !");
        return HB_ERROR;
    }
    if (HB_OK != mEncodeThreadCtx.setFunction(ThreadFunc_Video_Encoder, this)) {
        LOGE("Initial encode thread context failed !");
        return HB_ERROR;
    }
    
    if (mEncodeThreadCtx.start() != HB_OK) {
        LOGE("Start encode thread context failed !");
        return HB_ERROR;
    }
    
    return HB_OK;
}

int CSVideoEncoder::stop() {
    if (!(mState & DECODE_STATE_DECODE_END)) {
        mAbort = true;
    }
    mEncodeThreadCtx.join();
    
    if (CSMediaBase::stop() != HB_OK) {
        LOGE("Media base stop failed !");
        return HB_ERROR;
    }
    
    AVFrame *pFrame = nullptr;
    while (mSrcFrameQueue->queueLength()) {
        pFrame = mSrcFrameQueue->get();
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

int CSVideoEncoder::release() {
    if (mPOutVideoCodecCtx)
        avcodec_free_context(&mPOutVideoCodecCtx);
    mPOutVideoCodecCtx = nullptr;
    if (mPVideoConvertCtx) {
        sws_freeContext(mPVideoConvertCtx);
        mPVideoConvertCtx = nullptr;
    }
    CSMediaBase::release();
    return HB_OK;
}

int CSVideoEncoder::sendFrame(AVFrame **pSrcFrame) {
    int HBError = HB_ERROR;
    if (!pSrcFrame) {
        mState |= ENCODE_STATE_READPKT_END;
        LOGI("Video Encoder >>> Send frame end !");
        return HB_OK;
    }

    if (!(mState & ENCODE_STATE_PREPARED) || mInMediaType != MD_TYPE_RAW_BY_MEMORY) {
        LOGE("Video Encoder >>> Send raw frame failed, invalid output media type !");
        return HBError;
    }
    
    if (!(*pSrcFrame)) {
        LOGE("Video Encoder >>> Invalid frame, send failed !");
        return HBError;
    }
    
RETRY_SEND_FRAME:
    HBError = HB_ERROR;
    if (mSrcFrameQueue \
        && S_NOT_EQ(mState, ENCODE_STATE_ENCODE_ABORT) \
        && S_NOT_EQ(mState, ENCODE_STATE_FLUSH_MODE))
    {
        mEmptyFrameQueueIPC->condV();
        if (mSrcFrameQueue->queueLeft() > 0) {
            
//            int64_t tSrcFramePts = av_rescale_q((*pSrcFrame)->pts, \
//                                                AV_TIME_BASE_Q, mPOutVideoFormatCtx->streams[mVideoStreamIndex]->time_base);
//
//            LOGD("Video Encoder >>> Send frame, OriginalPts<%lld, %lf> !", \
                 (*pSrcFrame)->pts, ((*pSrcFrame)->pts * av_q2d(AV_TIME_BASE_Q)));
            
            if (mSrcFrameQueue->push(*pSrcFrame) > 0) {
                HBError = HB_OK;
                mSrcFrameQueueIPC->condP();
            }
        }
        else {
            goto RETRY_SEND_FRAME;
        }
    }
    
    return HBError;
}

int CSVideoEncoder::syncWait() {
    while (S_NOT_EQ(mState, ENCODE_STATE_ENCODE_END)) {
        usleep(100);
    }
    return HB_OK;
}

int CSVideoEncoder::_InputInitial() {
    switch (mInMediaType) {
        case MD_TYPE_RAW_BY_MEMORY:
            if (!mSrcFrameQueue || !mSrcFrameQueueIPC || !mEmptyFrameQueueIPC) {
                LOGE("[%s] >>> [Type:%s]Video encoder input prepare failed !", __FUNCTION__, \
                     getMediaDataTypeDescript(mInMediaType));
                return HB_ERROR;
            }
            break;
            
        case MD_TYPE_COMPRESS:
            LOGE("[%s] >>> [Type:%s]Video encoder not susport this media data type !", __FUNCTION__, \
                 getMediaDataTypeDescript(mInMediaType));
            break;
            
        default:
            LOGE("input media initial failed, cur media type:%s", getMediaDataTypeDescript(mInMediaType));
            return HB_ERROR;
    }
    
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
        return HB_ERROR;
    }
    
    return HB_OK;
}

int CSVideoEncoder::_SwscaleInitial() {
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
            return HB_ERROR;
        }
    }
    
    return HB_OK;
}

void CSVideoEncoder::_flush() {
    int HbError = HB_OK;
    mState |= ENCODE_STATE_FLUSH_MODE;
    avcodec_send_frame(mPOutVideoCodecCtx, NULL);
    
    AVPacket *pNewPacket = av_packet_alloc();
    while (true) {
        HbError = avcodec_receive_packet(mPOutVideoCodecCtx, pNewPacket);
        if (HbError == 0) {
            pNewPacket->stream_index = mVideoStreamIndex;
            HbError = _DoExport(pNewPacket);
            if (HbError != HB_OK) {
                av_packet_unref(pNewPacket);
            }
        }
        else {
            if (HbError != AVERROR_EOF) {
                mState |= ENCODE_STATE_ENCODE_ABORT;
                LOGE("[Work task: <Encoder>] Encoder flush success, %s !", av_err2str(HbError));
            }
            break;
        }
    }

    av_packet_free(&pNewPacket);
}

int CSVideoEncoder::_DoExport(AVPacket *pPacket)
{
    int HbError = av_write_frame(mPOutMediaFormatCtx, pPacket);
    if (HbError != 0) {
        LOGE("[Work task: <Encoder>] Export new packet failed, %s !", av_err2str(HbError));
        return HB_ERROR;
    }
    av_packet_unref(pPacket);
    return HB_OK;
}

int CSVideoEncoder::_DoSwscale(AVFrame *pInFrame, AVFrame **pOutFrame) {
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

int CSVideoEncoder::_EncoderInitial() {
    if (mOutMediaType == MD_TYPE_COMPRESS) {
        int HBError = HB_OK;
        AVStream* pVideoStream = nullptr;
        mPOutVideoCodecCtx = nullptr;
        memset(&mState, 0x00, sizeof(mState));
        
        if (CSMediaBase::_OutMediaInitial() != HB_OK) {
            LOGE("Media base in media initial failed !");
            goto VIDEO_ENCODER_INITIAL_END_LABEL;
        }
        
        pVideoStream = avformat_new_stream(mPOutMediaFormatCtx, NULL);
        if (!pVideoStream) {
            LOGE("[%s] >>> Video encoder initial failed, new stream failed !", __func__);
            goto VIDEO_ENCODER_INITIAL_END_LABEL;
        }
        
        pVideoStream->time_base.num = 1;
        pVideoStream->time_base.den = 90000;
        
        mPOutVideoCodec = avcodec_find_encoder(mPOutMediaFormatCtx->oformat->video_codec);
        if (!mPOutVideoCodec) {
            LOGE("[%s] >>> Video encoder initial failed, find valid encoder failed !", __func__);
            goto VIDEO_ENCODER_INITIAL_END_LABEL;
        }
        
        mPOutVideoCodecCtx = avcodec_alloc_context3(mPOutVideoCodec);
        mPOutVideoCodecCtx->codec_id = mPOutVideoCodec->id;
        mPOutVideoCodecCtx->codec_type = mPOutVideoCodec->type;
        mPOutVideoCodecCtx->gop_size = 30;
        mPOutVideoCodecCtx->keyint_min = 60;
//        mPOutVideoCodecCtx->has_b_frames = 0;
//        mPOutVideoCodecCtx->max_b_frames = 0;
        mPOutVideoCodecCtx->pix_fmt = getImageInnerFormat(mTargetVideoParams.mPixFmt);
        mPOutVideoCodecCtx->width = mTargetVideoParams.mWidth;
        mPOutVideoCodecCtx->height = mTargetVideoParams.mHeight;
        mPOutVideoCodecCtx->framerate.num = 1;
        mPOutVideoCodecCtx->framerate.den = 30;
        mPOutVideoCodecCtx->time_base.num = 1;
        mPOutVideoCodecCtx->time_base.den = 30;
        mPOutVideoCodecCtx->bit_rate = 1500000;
//        mPOutVideoCodecCtx->qmin = 34;
//        mPOutVideoCodecCtx->qmax = 50;
        
        if (mPOutMediaFormatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
            /** 如果转成实时传输数据，就不能设置： AV_CODEC_FLAG_GLOBAL_HEADER  */
            mPOutVideoCodecCtx->flags = AV_CODEC_FLAG_GLOBAL_HEADER;
        }
        
//        if (mPOutVideoCodecCtx->codec_id == AV_CODEC_ID_H264)
//        {
//            av_opt_set(mPOutVideoCodecCtx->priv_data, "level", "4.1", 0);
//            av_opt_set(mPOutVideoCodecCtx->priv_data, "preset", "superfast", 0);
//            av_opt_set(mPOutVideoCodecCtx->priv_data, "tune", "zerolatency", 0);
//        }
        
        
        mVideoStreamIndex = pVideoStream->index;
        
        av_dump_format(mPOutMediaFormatCtx, 0, mTrgMediaFile, 1);
        
        if ((HBError = avio_open(&(mPOutMediaFormatCtx->pb), mTrgMediaFile, AVIO_FLAG_READ_WRITE)) < 0) {
            LOGE("Video encoder Could't open output file, %s !", makeErrorStr(HBError));
            goto VIDEO_ENCODER_INITIAL_END_LABEL;
        }
        
        AVDictionary *opts = NULL;
        av_dict_set(&opts, "threads", "auto", 0);
        av_dict_set(&opts, "profile", "main", 0);
        
         avcodec_parameters_from_context(pVideoStream->codecpar, mPOutVideoCodecCtx);
        
        if ((HBError = avcodec_open2(mPOutVideoCodecCtx, mPOutVideoCodec, &opts)) < 0) {
            LOGE("Video encoder open failed, %s!", makeErrorStr(HBError));
            goto VIDEO_ENCODER_INITIAL_END_LABEL;
        }

        if ((HBError = avformat_write_header(mPOutMediaFormatCtx, NULL)) < 0) {
            LOGE("Video encoder write format header failed, %s!", makeErrorStr(HBError));
            goto VIDEO_ENCODER_INITIAL_END_LABEL;
        }
    }
    return HB_OK;

VIDEO_ENCODER_INITIAL_END_LABEL:
    CSMediaBase::release();
    if (mPOutVideoCodecCtx) {
        if (avcodec_is_open(mPOutVideoCodecCtx)) {
            avcodec_close(mPOutVideoCodecCtx);
        }
        avcodec_free_context(&mPOutVideoCodecCtx);
    }
    mPOutVideoCodec = nullptr;
    return HB_ERROR;
}

int CSVideoEncoder::_mediaParamInitial() {
    mIsNeedTransfer = false;
    switch (mInMediaType) {
        case MD_TYPE_UNKNOWN:
            {
                if (mSrcMediaFile)
                    mInMediaType = MD_TYPE_COMPRESS;
                else {
                    LOGE("[%s] >>> [Type:%s] Video encoder input file is invalid !", __func__, getMediaDataTypeDescript(mInMediaType));
                    return HB_ERROR;
                }
            }
            break;
            
        case MD_TYPE_RAW_BY_FILE:
            LOGE("Remain to support");
            return HB_ERROR;
            
        case MD_TYPE_RAW_BY_MEMORY:
            {
                if (!mSrcFrameQueue || !mSrcFrameQueueIPC || !mEmptyFrameQueueIPC) {
                    LOGE("[%s] >>> [Type:%s]Video encoder output prepare failed !", __func__, getMediaDataTypeDescript(mOutMediaType));
                    return HB_ERROR;
                }
                
                if (mSrcVideoParams.mPixFmt == CS_PIX_FMT_NONE \
                    || mSrcVideoParams.mWidth == 0 \
                    || mSrcVideoParams.mHeight == 0) {
                    LOGE("[%s] >>> [Type:%s]Video encoder output params invalid !", __func__, getMediaDataTypeDescript(mOutMediaType));
                    return HB_ERROR;
                }
                
            }
            break;
            
        case MD_TYPE_COMPRESS:
            if (!mSrcMediaFile) {
                LOGE("[%s] >>> [Type:%s]Video encoder input file is invalid !", __func__, getMediaDataTypeDescript(mInMediaType));
                return HB_ERROR;
            }
            break;
            
        default:
            LOGE("Unknown media type !");
            return HB_ERROR;
    }
    
    switch (mOutMediaType) {
        case MD_TYPE_UNKNOWN:
            {
                if (mTrgMediaFile)
                    mOutMediaType = MD_TYPE_COMPRESS;
                else {
                    LOGE("[%s] >>> [Type:%s] Video encoder output file is invalid !", __func__, getMediaDataTypeDescript(mInMediaType));
                    return HB_ERROR;
                }
            }
            break;
            
        case MD_TYPE_RAW_BY_FILE:
        case MD_TYPE_RAW_BY_MEMORY:
            LOGE("Current not support the output type !");
            return HB_ERROR;
            
        case MD_TYPE_COMPRESS:
            break;

        default:
            LOGE("Unknown media type !");
            return HB_ERROR;
    }
    return HB_OK;
}

}
