//
//  CSAudioDecoder.c
//  FFmpeg
//
//  Created by zj-db0519 on 2017/8/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSAudioDecoder.h"

namespace HBMedia {

int CSAudioDecoder::S_MAX_BUFFER_CACHE = 8;
void* CSAudioDecoder::ThreadFunc_Audio_Decoder(void *arg) {
    if (!arg) {
        LOGE("[Work task: <Decoder>] Thread param args is invalid !");
        return nullptr;
    }
    
    ThreadParam_t  *pThreadParams = (ThreadParam_t *)arg;
    CSAudioDecoder *pAudioDecoder = (CSAudioDecoder *)(pThreadParams->mThreadArgs);
    
    int HBError = HB_ERROR;
    AVFrame *pInFrame = nullptr;
    AVFrame *pOutFrame = nullptr;
    AVPacket* pNewPacket = av_packet_alloc();
    if (!pNewPacket) {
        pAudioDecoder->mState |= S_ABORT;
        LOGE("[Work task: <Decoder>] Malloc new packet failed !");
        goto DECODE_THREAD_EXIT_LABEL;
    }
    
    while (S_NOT_EQ(pAudioDecoder->mState, S_READ_PKT_END))
    {
        pInFrame = nullptr;
        pOutFrame = nullptr;
        
        if (S_EQ(pAudioDecoder->mState, S_ABORT) \
            || S_EQ(pAudioDecoder->mState, S_DECODE_ABORT)) {
            break;
        }
        
        HBError = av_read_frame(pAudioDecoder->mPInMediaFormatCtx, pNewPacket);
        if (HBError < 0) {
            if (HBError != AVERROR_EOF) {
                pAudioDecoder->mState |= S_READ_PKT_ABORT;
                LOGE("[Work task: <Decoder>] Read audio packet abort !");
            }
            else
                LOGW("[Work task: <Decoder>] Read audio packet reach file eof !");
            pAudioDecoder->mState |= S_READ_PKT_END;
            continue;
        }
        
        if (pNewPacket->stream_index != pAudioDecoder->mAudioStreamIndex) {
            av_packet_unref(pNewPacket);
            continue;
        }
        
        HBError = avcodec_send_packet(pAudioDecoder->mPInputAudioCodecCtx, pNewPacket);
        av_packet_unref(pNewPacket);
        if (HBError != 0) {
            if (HBError != AVERROR(EAGAIN)) {
                pAudioDecoder->mState |= S_DECODE_ABORT;
                LOGE("[Work task: <Decoder>] Send audio packet abort !");
            }
            continue;
        }
        
        AVFrame *pInFrame = av_frame_alloc();
        if (!pInFrame) {
            LOGE("[Work task: <Decoder>] Alloc new frame failed !");
            pAudioDecoder->mState |= S_ABORT;
            continue;
        }
        
        while (true)
        {
            HBError = avcodec_receive_frame(pAudioDecoder->mPInputAudioCodecCtx, pInFrame);
            if (HBError != 0) {
                if (HBError != AVERROR(EAGAIN)) {
                    pAudioDecoder->mState |= S_DECODE_ABORT;
                    LOGE("[Work task: <Decoder>] Decode audio frame abort !");
                }
                break;
            }
            
            if (pInFrame && pAudioDecoder->mIsNeedTransfer) {
                bool IsResampleSuccess = true;
                if (pAudioDecoder->_DoResample(pInFrame, &pOutFrame) != HB_OK) {
                    IsResampleSuccess = false;
                    pOutFrame = nullptr;
                    LOGE("Audio decoder resample failed !");
                }
                
                if (pInFrame->opaque)
                    av_freep(pInFrame->opaque);
                av_frame_free(&pInFrame);
                
                if (!IsResampleSuccess)
                    continue;
            }
            else {
                pOutFrame = pInFrame;
                pInFrame = nullptr;
            }
            
            if (pAudioDecoder->_DoExport(&pOutFrame) != HB_OK) {
                if (pOutFrame->opaque)
                    av_freep(pOutFrame->opaque);
                av_frame_free(&pOutFrame);
            }
        }
    }
    
    HBError = HB_OK;
DECODE_THREAD_EXIT_LABEL:
    if (pNewPacket)
        av_packet_free(&pNewPacket);
    
    pAudioDecoder->_flush();
    
    /** 将状态设置为结束状态后，唤醒外部可能等待中的外部接口调用 */
    pAudioDecoder->mState |= S_DECODE_END;
    pAudioDecoder->mTargetFrameQueueIPC->condP();
    
    return nullptr;
}

void CSAudioDecoder::_flush() {
    if (S_EQ(mState, S_ABORT))
        return;
    
    mState |= S_ENCODE_FLUSHING;
    avcodec_send_packet(mPInputAudioCodecCtx, NULL);
    
    int HbError = HB_OK;
    AVFrame *pInFrame = nullptr;
    AVFrame *pOutFrame = nullptr;
    while (true) {
        pInFrame = nullptr;
        pOutFrame = nullptr;
        
        pInFrame = av_frame_alloc();
        if (!pInFrame) {
            LOGE("[Work task: <Decoder>] Alloc new frame failed !");
            mState |= S_ABORT;
            continue;
        }
        
        HbError = avcodec_receive_frame(mPInputAudioCodecCtx, pInFrame);
        if (HbError != 0) {
            if (HbError != AVERROR_EOF) {
                mState |= S_DECODE_ABORT;
                LOGE("[Work task: <Decoder>] Flush audio frame abort !");
            }
            break;
        }
        
        if (pInFrame && mIsNeedTransfer) {
            bool IsResampleSuccess = true;
            if (_DoResample(pInFrame, &pOutFrame) != HB_OK) {
                IsResampleSuccess = false;
                pOutFrame = nullptr;
                LOGE("[Work task: <Decoder>] Audio decoder resample failed !");
            }
            
            if (pInFrame->opaque)
                av_freep(pInFrame->opaque);
            av_frame_free(&pInFrame);
            
            if (!IsResampleSuccess)
                continue;
        }
        else {
            pOutFrame = pInFrame;
            pInFrame = nullptr;
        }
        
        if (_DoExport(&pOutFrame) != HB_OK) {
            if (pOutFrame->opaque)
                av_freep(pOutFrame->opaque);
            av_frame_free(&pOutFrame);
        }
    }
    
    if (pInFrame) {
        if (pInFrame->opaque)
            av_freep(&(pInFrame->opaque));
        av_frame_free(&pInFrame);
    }
    
    if (pOutFrame) {
        if (pOutFrame->opaque)
            av_freep(&(pOutFrame->opaque));
        av_frame_free(&pOutFrame);
    }

    LOGI("[Work task: <Decoder>] Audio decoder flushed all buffer !");
}

CSAudioDecoder::CSAudioDecoder() {
    memset(&mState, 0x00, sizeof(mState));
    
    mAudioStreamIndex = INVALID_STREAM_INDEX;
    mPInMediaFormatCtx = nullptr;
    
    mPInputAudioCodecCtx = nullptr;
    mPInputAudioCodec = nullptr;
    mPAudioResampleCtx = nullptr;
    
    mTargetFrameQueue = new FiFoQueue<AVFrame *>(S_MAX_BUFFER_CACHE);
    mTargetFrameQueueIPC = new ThreadIPCContext(0);
    mEmptyFrameQueueIPC = new ThreadIPCContext(S_MAX_BUFFER_CACHE);
    mDecodeThreadCtx.setFunction(nullptr, nullptr);
}

CSAudioDecoder::~CSAudioDecoder() {
    release();
}
    
int CSAudioDecoder::prepare() {
    mIsNeedTransfer = false;
    
    if (baseInitial() != HB_OK) {
        LOGE("Audio base initial failed !");
        goto AUDIO_DECODER_PREPARE_END_LABEL;
    }
    
    if (_mediaParamInitial() != HB_OK) {
        LOGE("Check Audio decoder param failed !");
        goto AUDIO_DECODER_PREPARE_END_LABEL;
    }
    
    if (_DecoderInitial() != HB_OK) {
        LOGE("Audio decoder initial failed !");
        goto AUDIO_DECODER_PREPARE_END_LABEL;
    }
    
    if (_ExportInitial() != HB_OK) {
        LOGE("Audio decoder open failed !");
        goto AUDIO_DECODER_PREPARE_END_LABEL;
    }
    
    if (_ResampleInitial() != HB_OK) {
        LOGE("Audio resample initail failed !");
        goto AUDIO_DECODER_PREPARE_END_LABEL;
    }
    
    mState |= S_PREPARED;
    return HB_OK;
    
AUDIO_DECODER_PREPARE_END_LABEL:
    release();
    return HB_ERROR;
}

int CSAudioDecoder::start() {
    
    if (!(mState & S_PREPARED)) {
        LOGE("Media decoder is not prepared !");
        return HB_ERROR;
    }
    
    if (HB_OK != mDecodeThreadCtx.setFunction(ThreadFunc_Audio_Decoder, this)) {
        LOGE("Initial decode thread context failed !");
        return HB_ERROR;
    }
    
    if (mDecodeThreadCtx.start() != HB_OK) {
        LOGE("Start decoder thread context failed !");
        return HB_ERROR;
    }
    
    return HB_OK;
}

int CSAudioDecoder::stop() {
    if (S_EQ(mState, S_DECODE_END)) {
        mState |= (S_DECODE_ABORT | S_DECODE_END);
    }
    mState |= S_FINISHED;
    mEmptyFrameQueueIPC->condP();
    return HB_OK;
}

int CSAudioDecoder::release() {
    return HB_OK;
}

int CSAudioDecoder::receiveFrame(AVFrame **OutFrame) {
    int HBError = HB_ERROR;
    if (!OutFrame) {
        LOGE("Audio Decoder >>> invalid params !");
        return HB_ERROR;
    }
    
    *OutFrame = nullptr;
    if (!(mState & S_PREPARED) || mOutMediaType != MD_TYPE_RAW_BY_MEMORY) {
        LOGE("Audio Decoder >>> receive raw frame failed, invalid output media type !");
        return -3;
    }
    
RETRY_RECEIVE_FRAME:
    AVFrame *pNewFrame = nullptr;
    if (S_EQ(mState, S_DECODE_END) && (mTargetFrameQueue->queueLength() <= 0)) {
        mState |= S_FINISHED;
        HBError = -2;
        LOGI("Audio Decoder >>> Finish receive all valid frames !");
    }
    else {
        mTargetFrameQueueIPC->condV();
        if (mTargetFrameQueue->queueLength() <= 0)
            goto RETRY_RECEIVE_FRAME;
        
        if (!(pNewFrame = mTargetFrameQueue->get())) {
            HBError = -1;
            LOGE("Audio Decoder >>> Get audio frame failed !");
        }
        else
            mEmptyFrameQueueIPC->condP();
    }
    
    *OutFrame = pNewFrame;
    return HBError;
}

int CSAudioDecoder::syncWait() {
    return HB_OK;
}

int CSAudioDecoder::_DecoderInitial() {
    
    if (mInMediaType == MD_TYPE_COMPRESS) {
        int HBError = HB_OK;
        AVStream* pAudioStream = nullptr;
        mPInputAudioCodecCtx = nullptr;
        memset(&mState, 0x00, sizeof(mState));
        
        if (CSMediaBase::_InMediaInitial() != HB_OK) {
            LOGE("Media base in media initial failed !");
            goto VIDEO_DECODER_INITIAL_END_LABEL;
        }
        
        mAudioStreamIndex = av_find_best_stream(mPInMediaFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
        if (mAudioStreamIndex < 0) {
            LOGW("Audio decoder counldn't find valid audio stream, %s", av_err2str(mAudioStreamIndex));
            goto VIDEO_DECODER_INITIAL_END_LABEL;
        }
        
        pAudioStream = mPInMediaFormatCtx->streams[mAudioStreamIndex];
        mPInputAudioCodec = avcodec_find_decoder(pAudioStream->codecpar->codec_id);
        if (!mPInputAudioCodec) {
            LOGE("Codec <%d> not found !", pAudioStream->codecpar->codec_id);
            goto VIDEO_DECODER_INITIAL_END_LABEL;
        }
        
        mPInputAudioCodecCtx = avcodec_alloc_context3(mPInputAudioCodec);
        if (!mPInputAudioCodecCtx) {
            LOGE("Codec ctx <%d> not found !", pAudioStream->codecpar->codec_id);
            goto VIDEO_DECODER_INITIAL_END_LABEL;
        }
        avcodec_parameters_to_context(mPInputAudioCodecCtx, pAudioStream->codecpar);
        
        HBError = avcodec_open2(mPInputAudioCodecCtx, mPInputAudioCodec, NULL);
        if (HBError != 0) {
            LOGE("Could not open input codec context. <%s>", av_err2str(HBError));
            goto VIDEO_DECODER_INITIAL_END_LABEL;
        }
        
        /** 初始化原参数信息 */
        mSrcAudioParams.channels = mPInputAudioCodecCtx->channels;
        mSrcAudioParams.channel_layout = mPInputAudioCodecCtx->channel_layout;
        mSrcAudioParams.pri_sample_fmt = getAudioOuterFormat(mPInputAudioCodecCtx->sample_fmt);
        mSrcAudioParams.mbitRate = mPInputAudioCodecCtx->bit_rate;
        mSrcAudioParams.sample_rate = mPInputAudioCodecCtx->sample_rate;
        mSrcAudioParams.frame_size = mPInputAudioCodecCtx->frame_size;
        
        av_dump_format(mPInMediaFormatCtx, mAudioStreamIndex, mSrcMediaFile, false);
    }
    
    return HB_OK;
    
VIDEO_DECODER_INITIAL_END_LABEL:
    if (mPInputAudioCodec)
        avcodec_free_context(&mPInputAudioCodecCtx);
    mPInputAudioCodec = nullptr;
    CSMediaBase::release();
    return HB_ERROR;
}

int CSAudioDecoder::_ExportInitial() {
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
                LOGE("Audio decoder output buffer module initial failed !");
                return HB_ERROR;
            }
        }
            break;
        default:
            LOGE("output media initial failed, cur media type:%s", getMediaDataTypeDescript(mOutMediaType));
            return HB_ERROR;
    }
    
    if (mTargetAudioParams.channel_layout == 0)
        mTargetAudioParams.channel_layout = mSrcAudioParams.channel_layout;
    if (mTargetAudioParams.channels == 0)
        mTargetAudioParams.channels = mSrcAudioParams.channels;
    if (mTargetAudioParams.sample_rate == 0)
        mTargetAudioParams.sample_rate = mSrcAudioParams.sample_rate;
    if (getAudioInnerFormat(mTargetAudioParams.pri_sample_fmt) == AV_SAMPLE_FMT_NONE)
        mTargetAudioParams.pri_sample_fmt = mSrcAudioParams.pri_sample_fmt;
    if (mTargetAudioParams.frame_size == 0)
        mTargetAudioParams.frame_size = mSrcAudioParams.frame_size;
//    mSrcAudioParams.mbitRate = mPInputAudioCodecCtx->bit_rate;
    
    return HB_OK;
}

int CSAudioDecoder::_ResampleInitial() {
    mIsNeedTransfer = false;
    if (mSrcAudioParams.channel_layout != mTargetAudioParams.channel_layout \
        || mSrcAudioParams.pri_sample_fmt != mTargetAudioParams.pri_sample_fmt \
        || mSrcAudioParams.sample_rate != mTargetAudioParams.sample_rate) {
        
        mIsNeedTransfer = true;
        mPAudioResampleCtx = swr_alloc_set_opts(nullptr, \
                mTargetAudioParams.channel_layout, getAudioInnerFormat(mTargetAudioParams.pri_sample_fmt), mTargetAudioParams.sample_rate,\
                mSrcAudioParams.channel_layout, getAudioInnerFormat(mSrcAudioParams.pri_sample_fmt), mSrcAudioParams.sample_rate, \
                                0, nullptr);
        if (!mPAudioResampleCtx) {
            LOGE("Set audio resample context info failed !");
            return HB_ERROR;
        }
        
        swr_init(mPAudioResampleCtx);
    }

    return HB_OK;
}

int CSAudioDecoder::_DoResample(AVFrame *pInFrame, AVFrame **pOutFrame) {
    *pOutFrame = nullptr;
    if (!pInFrame || !mPAudioResampleCtx) {
        LOGE("[Work task: <Decoder>] Audio resample args invalid !");
        return HB_ERROR;
    }
    
    int HBError = HB_OK;
    uint8_t *pInAudioBuffer[CS_SWR_CH_MAX] = {NULL};
    AVFrame* pTargetFrame = av_frame_alloc();
    if (!pTargetFrame) {
        LOGE("[Work task: <Decoder>] Audio resample malloc audio target frame failed !");
        goto AUDIO_RESAMPLE_END_LABEL;
    }
    
    /**
     *  av_rescale_rnd(int64_t a, int64_t b, int64_t c, enum AVRounding rnd);
     *  将以 "时钟基c" 表示的 数值a 转换成以 "时钟基b" 来表示, "a * b / c" 的值并分五种方式来取整
     */
    pTargetFrame->nb_samples = \
       (int)av_rescale_rnd(swr_get_delay(mPAudioResampleCtx, mTargetAudioParams.sample_rate) + pInFrame->nb_samples,\
                                    mTargetAudioParams.sample_rate, mSrcAudioParams.sample_rate, AV_ROUND_UP);
    
    HBError = av_samples_alloc(pTargetFrame->data,
                           &pTargetFrame->linesize[0],
                           mTargetAudioParams.channels,
                           pTargetFrame->nb_samples,
                           getAudioInnerFormat(mTargetAudioParams.pri_sample_fmt), 0);
    if (HBError < 0) {
        LOGE("[Work task: <Decoder>] Audio resample malloc output samples buffer failed !");
        goto AUDIO_RESAMPLE_END_LABEL;
    }
    
    pTargetFrame->opaque = pTargetFrame->data[0];
    if (av_sample_fmt_is_planar(getAudioInnerFormat(mSrcAudioParams.pri_sample_fmt))) {
        /** TODO:  huangcl  */
//        int plane_size= pInFrame->nb_samples \
//                            * av_get_bytes_per_sample((enum AVSampleFormat)(getAudioInnerFormat(mSrcAudioParams.pri_sample_fmt) & 0xFF));
//        pInFrame->data[0]+i*plane_size;
        for (int i=0; i<mSrcAudioParams.channels; i++) {
            pInAudioBuffer[i] = pInFrame->data[i];
        }
    }
    else
        pInAudioBuffer[0] = pInFrame->data[0];
    
    HBError = swr_convert(mPAudioResampleCtx, pTargetFrame->data, pTargetFrame->nb_samples, \
                          (const uint8_t **)pInAudioBuffer, pInFrame->nb_samples);
    if (HBError < 0) {
        LOGE("[Work task: <Decoder>] Audio resample swr convert failed ! %s", av_err2str(HBError));
        goto AUDIO_RESAMPLE_END_LABEL;
    }

    pTargetFrame->pts = (int64_t)av_rescale_q(pInFrame->pts, \
                                              (AVRational){1, mSrcAudioParams.sample_rate},
                                              (AVRational){1, mTargetAudioParams.sample_rate});
    *pOutFrame = pTargetFrame;
    return HB_OK;

AUDIO_RESAMPLE_END_LABEL:
    if (pTargetFrame) {
        if (pTargetFrame->opaque)
            av_freep(pTargetFrame->opaque);
        av_frame_free(&pTargetFrame);
    }
    return HB_ERROR;
}

int CSAudioDecoder::_DoExport(AVFrame **pOutFrame) {
    if (!pOutFrame) {
        LOGE("Audio decoder export with invalid params !");
        return HB_ERROR;
    }
    
    AVFrame *pNewFrame = *pOutFrame;
    switch (mOutMediaType) {
        case MD_TYPE_RAW_BY_MEMORY:
        {
            if (mTargetFrameQueue) {
                /** 阻塞等待空间帧缓冲区存在可用资源 */
                mEmptyFrameQueueIPC->condV();
                {
                    /** 重新计算时间 */
                    pNewFrame->pts = (int64_t)av_rescale_q(pNewFrame->pts, \
                                                 (AVRational){1, mTargetAudioParams.sample_rate}, AV_TIME_BASE_Q);
                }
                if (mTargetFrameQueue->push(pNewFrame) > 0) {
                    LOGD("[Work task: <Decoder>] Push frame:%lld, %lf !", pNewFrame->pts, (pNewFrame->pts * av_q2d(AV_TIME_BASE_Q)));
                    mTargetFrameQueueIPC->condP();
                }
                else {
                    LOGE("[Work task: <Decoder>] Push frame to queue failed !");
                    mEmptyFrameQueueIPC->condP();
                    if (pNewFrame->opaque)
                        av_freep(pNewFrame->opaque);
                    av_frame_free(&pNewFrame);
                    *pOutFrame = nullptr;
                }
            }
            else {
                LOGE("[Work task: <Decoder>] Frame buffer queue is invalid !");
                break;
            }
            
            return HB_OK;
        }
            break;
        case MD_TYPE_RAW_BY_FILE:
            break;
        default:
            break;
    }
    return HB_ERROR;
}

int CSAudioDecoder::_mediaParamInitial() {
    switch (mInMediaType) {
        case MD_TYPE_UNKNOWN: {
            if (mSrcMediaFile)
                mInMediaType = MD_TYPE_COMPRESS;
            else {
                LOGE("[%s] >>> [Type:%s] Audio decoder input file is invalid !", __func__, getMediaDataTypeDescript(mInMediaType));
                return HB_ERROR;
            }
        }
            break;
        case MD_TYPE_COMPRESS:
            if (!mSrcMediaFile) {
                LOGE("[%s] >>> [Type:%s]Audio decoder input file is invalid !", __func__, getMediaDataTypeDescript(mInMediaType));
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
                LOGE("[%s] >>> [Type:%s]Audio decoder output file is invalid !", __func__, getMediaDataTypeDescript(mOutMediaType));
                return HB_ERROR;
            }
            break;
        case MD_TYPE_RAW_BY_MEMORY:
            if (!mTargetFrameQueue || !mEmptyFrameQueueIPC) {
                LOGE("[%s] >>> [Type:%s]Audio decoder output prepare failed !", __func__, getMediaDataTypeDescript(mOutMediaType));
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

}
