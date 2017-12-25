//
//  CSAudioEncoder.cpp
//  Sample
//
//  Created by zj-db0519 on 2017/12/25.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSAudioEncoder.h"

namespace HBMedia {

int CSAudioEncoder::S_MAX_BUFFER_CACHE = 8;
    
void* CSAudioEncoder::ThreadFunc_Audio_Encoder(void *arg) {
    return nullptr;
}

CSAudioEncoder::CSAudioEncoder() {
    memset(&mState, 0x00, sizeof(mState));
    
    mAudioStreamIndex = INVALID_STREAM_INDEX;
    
    mSrcFrameQueue = new FiFoQueue<AVFrame *>(S_MAX_BUFFER_CACHE);
    mSrcFrameQueueIPC = new ThreadIPCContext(0);
    mEmptyFrameQueueIPC = new ThreadIPCContext(S_MAX_BUFFER_CACHE);
    mEncodeThreadCtx.setFunction(nullptr, nullptr);
}
    
CSAudioEncoder::~CSAudioEncoder() {
}

int CSAudioEncoder::prepare() {
    if (baseInitial() != HB_OK) {
        LOGE("Audio base initial failed !");
        goto AUDIO_ENCODER_PREPARE_END_LABEL;
    }
    
    if (_mediaParamInitial() != HB_OK) {
        LOGE("Check Audio encoder param failed !");
        goto AUDIO_ENCODER_PREPARE_END_LABEL;
    }
    
    if (_InputInitial() != HB_OK) {
        LOGE("Audio decoder open failed !");
        goto AUDIO_ENCODER_PREPARE_END_LABEL;
    }
    
    if (_ResampleInitial() != HB_OK) {
        LOGE("Audio swscale initail failed !");
        goto AUDIO_ENCODER_PREPARE_END_LABEL;
    }
    
    if (_EncoderInitial() != HB_OK) {
        LOGE("Audio encoder initial failed !");
        goto AUDIO_ENCODER_PREPARE_END_LABEL;
    }
    
    mState |= S_PREPARED;
    return HB_OK;
    
AUDIO_ENCODER_PREPARE_END_LABEL:
    release();
    return HB_ERROR;
}
    
int CSAudioEncoder::start() {
    if (!(mState & S_PREPARED)) {
        LOGE("Media audio encoder is not prepared !");
        return HB_ERROR;
    }
    if (HB_OK != mEncodeThreadCtx.setFunction(ThreadFunc_Audio_Encoder, this)) {
        LOGE("Initial encode thread context failed !");
        return HB_ERROR;
    }
    
    if (mEncodeThreadCtx.start() != HB_OK) {
        LOGE("Start encode thread context failed !");
        return HB_ERROR;
    }
    
    return HB_OK;
}

int CSAudioEncoder::stop() {
    if (!(mState & S_DECODE_END)) {
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

int CSAudioEncoder::release() {
    if (mPOutAudioCodecCtx)
        avcodec_free_context(&mPOutAudioCodecCtx);
    mPOutAudioCodecCtx = nullptr;
    if (mPAudioConvertCtx) {
        sws_freeContext(mPAudioConvertCtx);
        mPAudioConvertCtx = nullptr;
    }
    CSMediaBase::release();
    return HB_OK;
}

int CSAudioEncoder::sendFrame(AVFrame **pSrcFrame) {
    return HB_OK;
}

int CSAudioEncoder::syncWait() {
    return HB_OK;
}

int CSAudioEncoder::_mediaParamInitial() {
    return HB_OK;
}

int CSAudioEncoder::_EncoderInitial() {
    return HB_OK;
}

int CSAudioEncoder::_InputInitial() {
    return HB_OK;
}

int CSAudioEncoder::_ResampleInitial() {
    return HB_OK;
}

int CSAudioEncoder::_DoResample(AVFrame *pInFrame, AVFrame **pOutFrame) {
    return HB_OK;
}

int CSAudioEncoder::_DoExport(AVPacket *pPacket) {
    return HB_OK;
}

void CSAudioEncoder::_flush() {
}

}
