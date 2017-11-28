//
//  CSMediaBase.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/11/25.
//  Copyright © 2017 meitu. All rights reserved.
//

#include "CSMediaBase.h"

namespace HBMedia {

CSMediaBase::CSMediaBase() {
    mIsNeedTransfer = false;
    mInMediaType = MD_TYPE_UNKNOWN;
    mOutMediaType = MD_TYPE_UNKNOWN;
    mSrcMediaFile = nullptr;
    mSrcMediaFileHandle = nullptr;
    mTrgMediaFile = nullptr;
    mTrgMediaFileHandle = nullptr;
    mPInVideoFormatCtx = nullptr;
    mPOutVideoFormatCtx = nullptr;
}

CSMediaBase::~CSMediaBase() {
    if (mSrcMediaFile)
        av_freep(&mSrcMediaFile);
    if (mTrgMediaFile)
        av_freep(&mTrgMediaFile);
}

int CSMediaBase::baseInitial() {
    av_register_all();
    avformat_network_init();
    return HB_OK;
}

int CSMediaBase::_InMediaInitial() {
    switch (mInMediaType) {
        case MD_TYPE_COMPRESS : {
            mPInVideoFormatCtx = nullptr;
            int HBError = avformat_open_input(&mPInVideoFormatCtx, mSrcMediaFile, NULL, NULL);
            if (HBError != 0) {
                LOGE("Media base in media initial failed, %s !", av_err2str(HBError));
                goto IN_MEDIA_INITIAL_END_LABEL;
            }

            HBError = avformat_find_stream_info(mPInVideoFormatCtx, NULL);
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
    if (mPInVideoFormatCtx) {
        avformat_close_input(&mPInVideoFormatCtx);
    }
    return HB_ERROR;
}

int CSMediaBase::release() {
    if (mPInVideoFormatCtx) {
        avformat_close_input(&mPInVideoFormatCtx);
    }
    if (mTrgMediaFileHandle) {
        fclose(mTrgMediaFileHandle);
        mTrgMediaFileHandle = nullptr;
    }
    return HB_OK;
}

int CSMediaBase::_OutMediaInitial() {

    switch (mOutMediaType) {
        case MD_TYPE_RAW_BY_FILE: {
            mTrgMediaFileHandle = fopen(mTrgMediaFile, "wb");
            if (!mTrgMediaFileHandle) {
                LOGE("Audio decoder couldn't open output file.");
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

}