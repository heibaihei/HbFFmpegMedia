//
//  CSMediaElement.cpp
//  Sample
//
//  Created by zj-db0519 on 2018/2/12.
//  Copyright © 2018年 meitu. All rights reserved.
//

#include "CSMediaElement.h"
#include "CSCommon.h"
#include "CSSpriteService.h"

namespace HBMedia {

void eltMediaInfoInitial(EltMediaInfo *pInfo) {
    if (!pInfo)
        return;

    pInfo->mStartPos = 0;
    pInfo->mDuration = 0;
    pInfo->mFileStartPos = 0;
    pInfo->mSpeed = 0.0f;
    pInfo->mLength = 0;
}

CSMediaElement::CSMediaElement() {
    eltMediaInfoInitial(&mOriginalMediaInfo);
    eltMediaInfoInitial(&mWorkMediaInfo);
}

CSMediaElement::~CSMediaElement() {
    
    
}

int CSMediaElement::prepare() {
    
    return HB_OK;
}

int CSMediaElement::start() {
    
    return HB_OK;
}
    
int CSMediaElement::fetchNextFrame(int64_t clock, CSSpriteService *pSpriteService) {
    
    return HB_OK;
}

int CSMediaElement::stop() {
    
    return HB_OK;
}

int CSMediaElement::release() {
    
    return HB_OK;
}

}
