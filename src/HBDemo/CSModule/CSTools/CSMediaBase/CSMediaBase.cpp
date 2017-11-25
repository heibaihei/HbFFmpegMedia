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
        mSrcPicMediaFile = nullptr;
        mSrcPicFileHandle = nullptr;

        mTrgPicMediaFile = nullptr;
        mTrgPicFileHandle = nullptr;
    }

    CSMediaBase::~CSMediaBase() {
        if (mSrcPicMediaFile)
            av_freep(&mSrcPicMediaFile);
        if (mTrgPicMediaFile)
            av_freep(&mTrgPicMediaFile);
    }
}