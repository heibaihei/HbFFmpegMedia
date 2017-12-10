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
void* CSVideoEncoder::ThreadFunc_Video_Encoder(void *arg) {
    
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
    return HB_OK;
}

int CSVideoEncoder::stop() {
    return HB_OK;
}

int CSVideoEncoder::release() {
    return HB_OK;
}

int CSVideoEncoder::sendFrame(AVFrame **OutFrame) {
    return HB_OK;
}

int CSVideoEncoder::syncWait() {
    return HB_OK;
}

int CSVideoEncoder::_InputInitial() {
    switch (mInMediaType) {
        case MD_TYPE_RAW_BY_MEMORY:
            if (!mSrcFrameQueue || !mSrcFrameQueueIPC || !mEmptyFrameQueueIPC) {
                LOGE("[%s] >>> [Type:%s]Video encoder input prepare failed !", __func__, getMediaDataTypeDescript(mInMediaType));
                return HB_ERROR;
            }
            break;
            
        default:
            LOGE("input media initial failed, cur media type:%s", getMediaDataTypeDescript(mInMediaType));
            return HB_ERROR;
    }
    
    return HB_OK;
}

int CSVideoEncoder::_SwscaleInitial() {
    mPVideoConvertCtx = nullptr;
    mIsNeedTransfer = false;
    
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

int CSVideoEncoder::_DoSwscale(AVFrame *pInFrame, AVFrame **pOutFrame) {
    return HB_OK;
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
        
        pVideoStream = avformat_new_stream(mPOutVideoFormatCtx, NULL);
        if (!pVideoStream) {
            LOGE("[%s] >>> Video encoder initial failed, new stream failed !", __func__);
            return HB_ERROR;
        }
        
        mPOutVideoCodec = avcodec_find_encoder(mPOutVideoFormatCtx->oformat->video_codec);
        if (!mPOutVideoCodec) {
            LOGE("[%s] >>> Video encoder initial failed, find valid encoder failed !", __func__);
            return HB_ERROR;
        }
        
        mPOutVideoCodecCtx = avcodec_alloc_context3(mPOutVideoCodec);
        mPOutVideoCodecCtx->codec_id = mPOutVideoCodec->id;
        mPOutVideoCodecCtx->codec_type = mPOutVideoCodec->type;
        mPOutVideoCodecCtx->pix_fmt = getImageInnerFormat(mTargetVideoParams.mPixFmt);
        mPOutVideoCodecCtx->width = mTargetVideoParams.mWidth;
        mPOutVideoCodecCtx->height = mTargetVideoParams.mHeight;
        mPOutVideoCodecCtx->time_base.num = 1;
        mPOutVideoCodecCtx->time_base.den = 30;
        
        avcodec_parameters_from_context(pVideoStream->codecpar, mPOutVideoCodecCtx);
        av_dump_format(mPOutVideoFormatCtx, 0, mTrgMediaFile, 1);
        
        AVDictionary *opts = NULL;
        av_dict_set(&opts, "threads", "auto", 0);
        
        if ((HBError = avio_open(&(mPOutVideoFormatCtx->pb), mTrgMediaFile, AVIO_FLAG_READ_WRITE)) < 0) {
            LOGE("Video encoder Could't open output file, %s !", makeErrorStr(HBError));
            return HB_ERROR;
        }
        
        if ((HBError = avcodec_open2(mPOutVideoCodecCtx, mPOutVideoCodec, &opts)) < 0) {
            LOGE("Video encoder open failed, %s!", makeErrorStr(HBError));
            return HB_ERROR;
        }
        
        if ((HBError = avformat_write_header(mPOutVideoFormatCtx, NULL)) < 0) {
            LOGE("Video encoder write format header failed, %s!", makeErrorStr(HBError));
            return HB_ERROR;
        }
    }
    return HB_OK;

VIDEO_ENCODER_INITIAL_END_LABEL:
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
            break;;

        default:
            LOGE("Unknown media type !");
            return HB_ERROR;
    }
    return HB_OK;
}

}
