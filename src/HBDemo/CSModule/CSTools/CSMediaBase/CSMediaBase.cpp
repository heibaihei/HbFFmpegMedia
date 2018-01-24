//
//  CSMediaBase.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/11/25.
//  Copyright © 2017 meitu. All rights reserved.
//

#include "CSMediaBase.h"
#include "CSVideoDecoder.h"
#include "CSVideoEncoder.h"
#include "CSAudioEncoder.h"
#include "CSAudioDecoder.h"

namespace HBMedia {

int CSMediaBase::fetchNextFrame(CSMediaBase *pProvider, int64_t clock, STREAM_TYPE type, AVFrame **pTargetFrame) {
    if (!pProvider || !pTargetFrame)
        return HB_ERROR;
    
    if (pProvider->mRoleType == T_VIDEO_DECODER) {
        CSVideoDecoder *pVideoProvider = (CSVideoDecoder *)pProvider;
        return pVideoProvider->receiveFrame(pTargetFrame);
    }
    return HB_ERROR;
}

CSMediaBase::CSMediaBase() {
    mIsNeedTransfer = false;
    mInMediaType = MD_TYPE_UNKNOWN;
    mOutMediaType = MD_TYPE_UNKNOWN;
    mSrcMediaFile = nullptr;
    mSrcMediaFileHandle = nullptr;
    mTrgMediaFile = nullptr;
    mTrgMediaFileHandle = nullptr;
    mPInMediaFormatCtx = nullptr;
    mPOutMediaFormatCtx = nullptr;
    memset(&mState, 0x00, sizeof(mState));
    audioParamInit(&mSrcAudioParams);
    audioParamInit(&mTargetAudioParams);
    imageParamInit(&mSrcVideoParams);
    imageParamInit(&mTargetVideoParams);
    mRoleType = T_ROLE_UNKNOWN;
    mAbort = false;
}

CSMediaBase::~CSMediaBase() {
    if (mSrcMediaFile)
        av_freep(&mSrcMediaFile);
    if (mTrgMediaFile)
        av_freep(&mTrgMediaFile);
}

int CSMediaBase::baseInitial() {
    mAbort = false;
    memset(&mState, 0x00, sizeof(mState));
    av_register_all();
    avformat_network_init();
    return HB_OK;
}

int CSMediaBase::_InMediaInitial() {
    switch (mInMediaType) {
        case MD_TYPE_COMPRESS : {
            mPInMediaFormatCtx = nullptr;
            int HBError = avformat_open_input(&mPInMediaFormatCtx, mSrcMediaFile, NULL, NULL);
            if (HBError != 0) {
                LOGE("Media base in media initial failed, %s !", av_err2str(HBError));
                goto IN_MEDIA_INITIAL_END_LABEL;
            }

            HBError = avformat_find_stream_info(mPInMediaFormatCtx, NULL);
            if (HBError < 0) {
                LOGE("Media base couldn't find stream information. <%s>", av_err2str(HBError));
                goto IN_MEDIA_INITIAL_END_LABEL;
            }
        }
            break;
        default:
            LOGE("Base media input media type:%s is not support !", getMediaDataTypeDescript(mInMediaType));
            return HB_ERROR;
    }

    return HB_OK;

IN_MEDIA_INITIAL_END_LABEL:
    if (mPInMediaFormatCtx) {
        avformat_close_input(&mPInMediaFormatCtx);
    }
    return HB_ERROR;
}

int CSMediaBase::release() {
    if (mPInMediaFormatCtx) {
        avformat_close_input(&mPInMediaFormatCtx);
    }
    if (mPOutMediaFormatCtx) {
        avformat_free_context(mPOutMediaFormatCtx);
        mPOutMediaFormatCtx = nullptr;
    }
    if (mSrcMediaFileHandle) {
        fclose(mSrcMediaFileHandle);
        mSrcMediaFileHandle = nullptr;
    }
    if (mTrgMediaFileHandle) {
        fclose(mTrgMediaFileHandle);
        mTrgMediaFileHandle = nullptr;
    }
    return HB_OK;
}

int CSMediaBase::stop(){
    switch (mOutMediaType) {
        case MD_TYPE_RAW_BY_FILE: {
            if (mTrgMediaFileHandle) {
                fclose(mTrgMediaFileHandle);
                mTrgMediaFileHandle = nullptr;
            }
        }
            break;
        case MD_TYPE_RAW_BY_MEMORY:
            break;
        case MD_TYPE_COMPRESS:
            avformat_free_context(mPOutMediaFormatCtx);
            mPOutMediaFormatCtx = nullptr;
            break;
        default:
            LOGE("Base media output media type:%s is not support !", getMediaDataTypeDescript(mOutMediaType));
            return HB_ERROR;
    }
    
    switch (mInMediaType) {
        case MD_TYPE_COMPRESS: {
            if (mPInMediaFormatCtx) {
                avformat_close_input(&mPInMediaFormatCtx);
            }
        }
            break;
        case MD_TYPE_RAW_BY_MEMORY:
            /** TODO: 待补充结束资源的释放 */
            break;
        default:
            LOGE("Base media input media type:%s is not support !", getMediaDataTypeDescript(mInMediaType));
            return HB_ERROR;
    }
    return HB_OK;
}

int CSMediaBase::_OutMediaInitial() {

    switch (mOutMediaType) {
        case MD_TYPE_RAW_BY_FILE: {
            mTrgMediaFileHandle = fopen(mTrgMediaFile, "wb");
            if (!mTrgMediaFileHandle) {
                LOGE("Base media output initial failed, couldn't open output file.");
                return HB_ERROR;
            }

        }
        break;
        case MD_TYPE_RAW_BY_MEMORY:
            break;
        case MD_TYPE_COMPRESS:
            {
                int HBErr = HB_OK;
                HBErr = avformat_alloc_output_context2(&mPOutMediaFormatCtx, NULL, NULL, mTrgMediaFile);
                if (HBErr < 0) {
                    LOGE("Base media alloc output avformat ctx failed, %s", av_err2str(HBErr));
                    return HB_ERROR;
                }
            }
            break;
        default:
            LOGE("Base media output media type:%s is not support !", getMediaDataTypeDescript(mOutMediaType));
            return HB_ERROR;
    }

    return HB_OK;
}

void CSMediaBase::setInMediaFile(char *file) {
    if (mSrcMediaFile)
        av_freep(&mSrcMediaFile);
    mSrcMediaFile = av_strdup(file);
}

void CSMediaBase::setOutMediaFile(char *file) {
    if (mTrgMediaFile)
        av_freep(&mTrgMediaFile);
    mTrgMediaFile = av_strdup(file);
}

void CSMediaBase::setInAudioMediaParams(AudioParams& pParams) {
    mSrcAudioParams = pParams;
}

void CSMediaBase::setOutAudioMediaParams(AudioParams& pParams) {
    mTargetAudioParams = pParams;
}
    
void CSMediaBase::setInImageMediaParams(ImageParams& pParams) {
    mSrcVideoParams = pParams;
}
    
void CSMediaBase::setOutImageMediaParams(ImageParams& pParams) {
    mTargetVideoParams = pParams;
}

}
