//
//  CSAudioDecoder.c
//  FFmpeg
//
//  Created by zj-db0519 on 2017/8/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSAudioDecoder.h"

namespace HBMedia {

static void EchoStatus(uint64_t status) {
    bool bReadEnd = ((status & S_READ_PKT_END) != 0 ? true : false);
    bool bDecodeEnd = ((status & DECODE_STATE_DECODE_END) != 0 ? true : false);
    bool bReadAbort = ((status & DECODE_STATE_READPKT_ABORT) != 0 ? true : false);
    bool bDecodeAbort = ((status & DECODE_STATE_DECODE_ABORT) != 0 ? true : false);
    bool bFlushMode = ((status & DECODE_STATE_FLUSH_MODE) != 0 ? true : false);
    
    LOGI("[Work task: <Decoder>] Status: Read<End:%d, Abort:%d> | <Flush:%d> | Decode<End:%d, Abort:%d>", bReadEnd, bReadAbort, bFlushMode, bDecodeEnd, bDecodeAbort);
}

int CSAudioDecoder::S_MAX_BUFFER_CACHE = 8;
void* CSAudioDecoder::ThreadFunc_Audio_Decoder(void *arg) {
    if (!arg) {
        LOGE("[Work task: <Decoder>] Thread param args is invalid !");
        return nullptr;
    }
    
    int HBError = HB_ERROR;
    AVFrame *pInFrame = nullptr;
    AVFrame *pOutFrame = nullptr;
    CSAudioDecoder *pAudioDecoder = (CSAudioDecoder *)arg;
    AVPacket* pNewPacket = av_packet_alloc();
    if (!pNewPacket) {
        pAudioDecoder->mState |= S_ABORT;
        LOGE("[Work task: <Decoder>] malloc new packet failed !");
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
        
        pAudioDecoder->mTargetFrameQueueIPC->condV();
        if (!(pInFrame = pAudioDecoder->mTargetFrameQueue->get())) {
            LOGE("[Work task: <Encoder:%p>] get queue[%p] length:%d frame failed !", pAudioDecoder, \
                 pAudioDecoder->mTargetFrameQueue, pAudioDecoder->mTargetFrameQueue->queueLength());
            continue;
        }
        pAudioDecoder->mEmptyFrameQueueIPC->condP();
        
        HBError = av_read_frame(pAudioDecoder->mPInMediaFormatCtx, pNewPacket);
        if (HBError < 0) {
            if (HBError != AVERROR_EOF)
                LOGE("[Work task: <Decoder>] Read audio frame abort !");
            else
                LOGW("[Work task: <Decoder>] Read audio frame reach file eof !");
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
                LOGE("[Work task: <Decoder>] Decode audio frame abort !");
            }
            continue;
        }
        
        AVFrame *pInFrame = av_frame_alloc();
        if (!pInFrame) {
            LOGE("Alloc new frame failed !");
            pAudioDecoder->mState |= S_ABORT;
            continue;
        }
        
        while (true)
        {
            HBError = avcodec_receive_frame(pAudioDecoder->mPInputAudioCodecCtx, pInFrame);
            if (HBError != 0) {
                if (HBError != AVERROR(EAGAIN))
                    pAudioDecoder->mState |= S_DECODE_ABORT;
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
                /** 验证是否需要拷贝部分数据 */
            }
            
            pAudioDecoder->_DoExport(&pOutFrame);
        }
        
    }
    
    HBError = HB_OK;
DECODE_THREAD_EXIT_LABEL:
    if (pNewPacket)
        av_packet_free(&pNewPacket);
    
    pAudioDecoder->_flush();
    pAudioDecoder->mState |= DECODE_STATE_DECODE_END;
    
    if (HBError != HB_OK) {
        pAudioDecoder->release();
    }
    return nullptr;
}

void CSAudioDecoder::_flush() {
}

CSAudioDecoder::CSAudioDecoder() {
    memset(&mState, 0x00, sizeof(mState));
    audioParamInit(&mSrcAudioParams);
    audioParamInit(&mTargetAudioParams);
    
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
    
    mState |= DECODE_STATE_PREPARED;
    return HB_OK;
    
AUDIO_DECODER_PREPARE_END_LABEL:
    release();
    return HB_ERROR;
}

int CSAudioDecoder::start() {
    
    if (!(mState & DECODE_STATE_PREPARED)) {
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
    return HB_OK;
}

int CSAudioDecoder::release() {
    return HB_OK;
}

int CSAudioDecoder::receiveFrame(AVFrame **OutFrame) {
    return HB_OK;
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
#if 0
    if (!OutData || !InData) {
        LOGE("Audio resample args invalid !");
        return HB_ERROR;
    }
    
    int outputSamplesPerChannel = ((InSamples * mTargetAudioParams.sample_rate) / mSrcAudioParams.sample_rate);
    *OutData = (uint8_t*)av_mallocz(av_samples_get_buffer_size(NULL, mTargetAudioParams.channels, outputSamplesPerChannel, getAudioInnerFormat(mTargetAudioParams.pri_sample_fmt), 1));
    if (!(OutData)) {
        LOGE("Fast malloc output sample buffer failed !");
        return HB_ERROR;
    }
    
    if (mPAudioSwrCtx)
        return swr_convert(mPAudioSwrCtx, OutData, outputSamplesPerChannel, (const uint8_t **)InData, InSamples);
#endif
    return HB_OK;
}

int CSAudioDecoder::_DoExport(AVFrame **pOutFrame) {
    return HB_OK;
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
