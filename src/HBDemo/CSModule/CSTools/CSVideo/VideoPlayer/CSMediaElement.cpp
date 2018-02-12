//
//  CSMediaElement.cpp
//  Sample
//
//  Created by zj-db0519 on 2018/2/12.
//  Copyright © 2018年 meitu. All rights reserved.
//

#include "CSMediaElement.h"
#include "CSCommon.h"
#include "CSSprite.h"
#include "CSSpriteService.h"
#include "CSMediaBase.h"
#include "CSTexture.h"

namespace HBMedia {

void eltMediaInfoInitial(EltMediaInfo *pInfo) {
    if (!pInfo)
        return;

    pInfo->mStartPos = 0;
    pInfo->mDuration = 0;
    pInfo->mFileStartPos = 0;
    pInfo->mSpeed = 0.0f;
    pInfo->mLength = 0;
    pInfo->mRotateAngle = 0;
    
    pInfo->mCenterX = 0.0f;
    pInfo->mCenterY = 0.0f;
    pInfo->mWidth = 0;
    pInfo->mHeight = 0;
    pInfo->mContentRotateAngle = 0;
}

CSMediaElement::CSMediaElement() {
    eltMediaInfoInitial(&mOriginalMediaInfo);
    eltMediaInfoInitial(&mWorkMediaInfo);
    mSprite = nullptr;
}

CSMediaElement::~CSMediaElement() {
    
    
}

int CSMediaElement::_glPrepare(CSSpriteService *pSpriteService) {
    if (mSprite)
        return HB_OK;
    
    mTexture = new CSTexture();
    mSprite = new CSSprite();
    
    mSprite->setTrackType(mTrackType);
    mSprite->setTexture(mTexture);
    mSprite->rotateTo(mWorkMediaInfo.mRotateAngle);
    mSprite->moveTo(mWorkMediaInfo.mCenterX, mWorkMediaInfo.mCenterY);
#if 0
    /** TODO: huangcl */
    int rotate = mMediaProvider->getVideoRotate();
    // codec rotate is clockwise, sprite is Counterclockwise
    rotate = -rotate + contentRotateAngle;
    if (rotate > 0) {
        rotate -= 360;
    }
    sprite->setOrigRotateAngle(rotate);
#endif
    
    if (!mOriginalMediaInfo.mWidth || !mOriginalMediaInfo.mHeight)
        setOriginalWidthAndHeight(mWorkMediaInfo.mWidth, mWorkMediaInfo.mHeight);
    
    mSprite->setOrigWidthAndHeight(mOriginalMediaInfo.mWidth, mOriginalMediaInfo.mHeight);
    mSprite->setWidthAndHeight(mWorkMediaInfo.mWidth, mWorkMediaInfo.mHeight);
    mSprite->setZOrder(mZOrder);
    mSprite->setShader(mShader);
    mSprite->setVisible(false);
    mSprite->setUV(startU, endU, startV, endV);
    mSprite->setScissorBox(mScissorLowerLeft, mScissorBox, mEnableScissor);
    mSprite->setUseColor(mUseColor);
    mSprite->setTextColor(mTextureColor);
    mSprite->scaleTo(mScaleX, mScaleY);
    
    pSpriteService->pushSprite(mSprite);
    return HB_OK;
}

void CSMediaElement::setShaderOnGL() {
    
}

int CSMediaElement::prepare() {
    
    return HB_OK;
}

int CSMediaElement::start() {
    
    return HB_OK;
}
    
int CSMediaElement::fetchNextFrame(int64_t clock, CSSpriteService *pSpriteService, STREAM_TYPE eMediaType) {
    int ret = 0;
    if (eMediaType == CS_STREAM_TYPE_VIDEO)
    {   /** 获取视频片段信息 */
        AVFrame *pNextFrame = nullptr;
        ret = CSMediaBase::fetchNextFrame(mMediaProvider, clock, eMediaType, &pNextFrame);
        if (ret == 1) {
            if (!mSprite) {
                _glPrepare(pSpriteService);
            }
        }
    }
    
    return HB_OK;
}

int CSMediaElement::stop() {
    
    return HB_OK;
}

int CSMediaElement::release() {
    
    return HB_OK;
}

void CSMediaElement::setOriginalWidthAndHeight(int width, int height) {
    mOriginalMediaInfo.mWidth = width;
    mOriginalMediaInfo.mHeight = height;
}

}
