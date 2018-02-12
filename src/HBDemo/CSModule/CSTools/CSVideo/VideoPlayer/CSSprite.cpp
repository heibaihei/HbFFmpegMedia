//
//  CSSprite.cpp
//  Sample
//
//  Created by zj-db0519 on 2018/1/25.
//  Copyright © 2018年 meitu. All rights reserved.
//

#include "CSSprite.h"
#include "CSShader.h"
#include "CSTexture.h"

namespace HBMedia {
    
CSSprite::CSSprite() {
    mTexture = nullptr;
    mVisible = true;
    mNeedUpdatePosition = true;
    mNeedUpdateUV = true;
    mTrackType = UNKNOW_TRACK;
}

CSSprite::~CSSprite() {
    if (mTexture) {
        /** 释放纹理逻辑 */
    }
}

int CSSprite::update() {
    if (!mVisible)
        return HB_OK;
    
    if (mNeedUpdatePosition) {
        _updatePosition();
        mNeedUpdatePosition = false;
    }
    
    if (mNeedUpdateUV) {
        _updateUV();
        mNeedUpdateUV = false;
    }
    return HB_OK;
}

int CSSprite::_updatePosition() {
    // TODO: for scale, 1. width or height is odd
    // 2. width:height != 1:1 how to scale ?
    // width, height float -> int ?
    double halfW = (mWidth * aWScale + aWChange) / 2.0;
    double halfH = (mHeight * aHScale + aHChange) / 2.0;
    
    double angle = (rotateAngle + aRotateChange) / 180.0 * M_PI;
    double sinAngle = sin(angle);
    double cosAngle = cos(angle);
    double halfWCos = halfW * cosAngle;
    double halfHSin = halfH * sinAngle;
    double halfWSin = halfW * sinAngle;
    double halfHCos = halfH * cosAngle;
    
    // moving along the direction of the rotation angle
    double rotateRadian = (rotateAngle) / 180.0 * M_PI;
    double sinRotate = sin(rotateRadian);
    double cosRotate = cos(rotateRadian);
    double xChange = aXChange * cosRotate - aYChange * sinRotate;
    double yChange = aXChange * sinRotate + aYChange * cosRotate;
    
    double tmpX = mCenterX + xChange;
    double tmpY = mCenterY + yChange;
    
    if (aUseRotateCenter) {
        double changeX = tmpX - aRotateCenter.x;
        double changeY = tmpY - aRotateCenter.y;
        double r_changeX = changeX * cosAngle - changeY * sinAngle;
        double r_changeY = changeX * sinAngle + changeY * cosAngle;
        // rotate around a porint
        tmpX += (r_changeX - changeX);
        tmpY += (r_changeY - changeY);
    }
    
    mQuads.tl.vertices.x = tmpX - halfWCos - halfHSin;
    mQuads.tl.vertices.y = tmpY + halfHCos - halfWSin;
    mQuads.tl.vertices.z = 0.0f;
    
    mQuads.bl.vertices.x = tmpX - halfWCos + halfHSin;
    mQuads.bl.vertices.y = tmpY - halfHCos - halfWSin;
    mQuads.bl.vertices.z = 0.0f;
    
    mQuads.tr.vertices.x = tmpX + halfWCos - halfHSin;
    mQuads.tr.vertices.y = tmpY + halfHCos + halfWSin;
    mQuads.tr.vertices.z = 0.0f;
    
    mQuads.br.vertices.x = tmpX + halfWCos + halfHSin;
    mQuads.br.vertices.y = tmpY - halfHCos + halfWSin;
    mQuads.br.vertices.z = 0.0f;

    return HB_OK;
}

int CSSprite::_updateUV() {
    float reqSU = startU + aStartUChange;
    float reqEU = endU + aEndUChange;
    float reqSV = startV + aStartVChange;
    float reqEV = endV + aEndVChange;
    
    if (origRotateAngle == -90) {
        float tmpSU = 1 - reqEV;
        float tmpEU = 1 - reqSV;
        float tmpSV = 1 - reqSU;
        float tmpEV = 1- reqEU;
        // tl -> tr
        mQuads.tr.texCoords.u = tmpSU;//startU;
        mQuads.tr.texCoords.v = tmpEV;//endV;
        // bl -> tl
        mQuads.tl.texCoords.u = tmpSU;//startU;
        mQuads.tl.texCoords.v = tmpSV;//startV;
        // tr -> br
        mQuads.br.texCoords.u = tmpEU;//endU;
        mQuads.br.texCoords.v = tmpEV;//endV;
        // br -> bl
        mQuads.bl.texCoords.u = tmpEU;//endU;
        mQuads.bl.texCoords.v = tmpSV;//startV;
    } else if (origRotateAngle == -180) {
        float tmpSU = 1- reqEU;
        float tmpEU = 1 - reqSU;
        float tmpSV = reqEV;
        float tmpEV = reqSV;
        // tl -> br
        mQuads.br.texCoords.u = tmpSU;//startU;
        mQuads.br.texCoords.v = tmpEV;//endV;
        // bl -> tr
        mQuads.tr.texCoords.u = tmpSU;//startU;
        mQuads.tr.texCoords.v = tmpSV;//startV;
        // tr -> bl
        mQuads.bl.texCoords.u = tmpEU;//endU;
        mQuads.bl.texCoords.v = tmpEV;//endV;
        // br -> tl
        mQuads.tl.texCoords.u = tmpEU;//endU;
        mQuads.tl.texCoords.v = tmpSV;//startV;
    } else if (origRotateAngle == -270) {
        float tmpSU = reqSV;
        float tmpEU = reqEV;
        float tmpSV = reqEU;
        float tmpEV = reqSU;
        // tl -> bl
        mQuads.bl.texCoords.u = tmpSU;//startU;
        mQuads.bl.texCoords.v = tmpEV;//endV;
        // bl -> br
        mQuads.br.texCoords.u = tmpSU;//startU;
        mQuads.br.texCoords.v = tmpSV;//startV;
        // tr -> tl
        mQuads.tl.texCoords.u = tmpEU;//endU;
        mQuads.tl.texCoords.v = tmpEV;//endV;
        // br -> tr
        mQuads.tr.texCoords.u = tmpEU;//endU;
        mQuads.tr.texCoords.v = tmpSV;//startV;
    } else {
        // original
        float tmpSU = reqSU;
        float tmpEU = reqEU;
        float tmpSV = 1 - reqSV;
        float tmpEV = 1 - reqEV;
        
        mQuads.tl.texCoords.u = tmpSU;//startU;
        mQuads.tl.texCoords.v = tmpEV;//endV;
        
        mQuads.bl.texCoords.u = tmpSU;//startU;
        mQuads.bl.texCoords.v = tmpSV;//startV;
        
        mQuads.tr.texCoords.u = tmpEU;//endU;
        mQuads.tr.texCoords.v = tmpEV;//endV;
        
        mQuads.br.texCoords.u = tmpEU;//endU;
        mQuads.br.texCoords.v = tmpSV;//startV;
    }
    
    return HB_OK;
}
    
void CSSprite::setTexture(CSTexture* texture) {
    if (!texture)
        return;
    
    if (mTexture != texture) {
        mTexture = texture;
    }
}

void CSSprite::setUseColor(bool use) {
    mUseColor = use;
}
    
void CSSprite::setTextColor(const Vec3& color) {
    mTextureColor = color;
}

void CSSprite::setAnimationAlpha(float alpha) {
    aAlpha = alpha;
}

void CSSprite::setAlphaPremultiplied(bool premultiplied) {
    mAlphaPremultiplied = premultiplied;
}
    
bool CSSprite::hasShader() const {
    return ((mShader != NULL) || (!mAnimationShaders.empty()));
}

void CSSprite::setShader(CSShader* pShader) {
    mShader = pShader;
}

CSShader* CSSprite::getShader() const {
    return mShader;
}
    
void CSSprite::setOrigRotateAngle(int angle) {
    origRotateAngle = angle;
}

void CSSprite::setTrackType(SpriteType type) {
    mTrackType = type;
}

}
