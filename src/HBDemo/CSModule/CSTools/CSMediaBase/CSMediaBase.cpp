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
        mSrcPicMediaFile = nullptr;
        mSrcPicFileHandle = nullptr;
        mPInVideoFormatCtx = nullptr;
        mTrgPicMediaFile = nullptr;
        mTrgPicFileHandle = nullptr;
        mPOutVideoFormatCtx = nullptr;
    }

    CSMediaBase::~CSMediaBase() {
        if (mSrcPicMediaFile)
            av_freep(&mSrcPicMediaFile);
        if (mTrgPicMediaFile)
            av_freep(&mTrgPicMediaFile);
    }

    int CSMediaBase::baseInitial() {
        av_register_all();
        avformat_network_init();
        return HB_OK;
    }
}
