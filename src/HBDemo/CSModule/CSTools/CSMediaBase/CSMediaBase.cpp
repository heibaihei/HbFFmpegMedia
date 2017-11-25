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
