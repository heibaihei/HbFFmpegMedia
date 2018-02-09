//
//  CSSpriteService.cpp
//  Sample
//
//  Created by zj-db0519 on 2018/1/25.
//  Copyright © 2018年 meitu. All rights reserved.
//

#include "CSSpriteService.h"

#include <OpenGL/gl.h>

#include "CSDefine.h"
#include "CSCommon.h"
#include "CSSprite.h"
#include "CSTexture.h"
#include "CSTextureCache.h"
#include "CSTextureShader.h"
#include "CSProgramCache.h"

#include "CSMatrixShader.h"

namespace HBMedia {

int CSSpriteService::mTextureCachNum = 0;
    
CSSpriteService::CSSpriteService() {
    mScreenWidth = 0;
    mScreenHeight = 0;
    mBDrawing = false;
    mEnableFilpShader = true;
}

CSSpriteService::~CSSpriteService() {
    
}
    
void CSSpriteService::pushSprite(CSSprite* pSprite) {
    if (!pSprite)
        return;
    
    mSpritesArray.push_back(pSprite);
}

void CSSpriteService::popSprite(CSSprite* pSprite) {
    if (!pSprite)
        return;
    
    for (std::list<CSSprite *>::iterator iter = mSpritesArray.begin(); iter != mSpritesArray.end(); ++iter) {
        CSSprite* pCurrent = *iter;
        if (pCurrent == pSprite) {
            mSpritesArray.erase(iter);
            break;
        }
    }
    SAFE_DELETE(pSprite);
}
    
int CSSpriteService::_innerGlPrepare() {
    mBDrawing = false;
    setAlphaBlending(false);
    setDepthTest(false);
    
    /**
     *  根据需要进行相应配置
     */
    CSTexture::initMaxTextureSize();
    mTextureCacheIndex = mTextureCachNum;
    mTextureCachNum++;
    if(mTextureCachNum > CSTextureCache::MAX_TEXTURE_CACHE) {
        LOGE("CSTextureCache >>> Max texture cache is %i", CSTextureCache::MAX_TEXTURE_CACHE);
        return false;
    }
    
    Mat4::createLookAt(Vec3(0, 0, 1), Vec3(0, 0, 0), Vec3(0, 1, 0), &vMat);
    for( int i=0; i < mTextureVboSize/4; i++)
    {
        mQuadIndices[i*6+0] = (GLushort) (i*4+0);
        mQuadIndices[i*6+1] = (GLushort) (i*4+1);
        mQuadIndices[i*6+2] = (GLushort) (i*4+2);
        mQuadIndices[i*6+3] = (GLushort) (i*4+3);
        mQuadIndices[i*6+4] = (GLushort) (i*4+2);
        mQuadIndices[i*6+5] = (GLushort) (i*4+1);
    }
    
    _bindTextureCache();
    setupVBO();
    
    mCommonShader = new CSTextureShader();
    mCommonShader->link();
    
    mMatrixShader = new CSMatrixShader(false);
    mMatrixShader->setup();
    
    _screenSizeChanged(mScreenWidth, mScreenHeight);
     CSProgramCache::loadDefaultGLPrograms();
    
    mFramebufferObject.setup(mScreenWidth, mScreenHeight);
    mBitmapFBO.setup(mScreenWidth, mScreenHeight);
    mFilpShader.setup();
    return HB_OK;
}

void CSSpriteService::_screenSizeChanged(int width, int height)
{
    _bindTextureCache();
    
    if (!width || !height) {
        LOGE("Sprite service >>> Screen size change with invalid width %d, height %d !", width, height);
        return;
    }
    
//    if ((width == mScreenWidth && height == mScreenHeight)) {
//        LOGE("Sprite service >>> Screen size no change, width %d, height %d !", width, height);
//        return;
//    }
    
    mScreenWidth  = width;
    mScreenHeight = height;
    mRenderWidth = width;
    mRenderHeight = height;
    
    // setup fbo
    mFramebufferObject1.setup(mRenderWidth, mRenderHeight);
    mFramebufferObject2.setup(mRenderWidth, mRenderHeight);
    mFragmentFBO.setup(mRenderWidth, mRenderHeight);
    
    glViewport(0, 0, mRenderWidth, mRenderHeight);
    
    Mat4::createOrthographicOffCenter(0.0f, (float)mRenderWidth, 0.0f, (float)mRenderHeight, -1.0f, 1.0f, &pMat);
    // mvp = Projection * View * Model
    Mat4::multiply(pMat, vMat, &mvpMat);
    mCommonShader->setMatrix(mvpMat);
}

int CSSpriteService::prepare() {
    if (mScreenWidth==0 || mScreenHeight == 0) {
        LOGE("Sprite service >>> Prepare with invalid width:%d and height:%d", mScreenWidth, mScreenHeight);
        return HB_ERROR;
    }
    mRenderWidth = mScreenWidth;
    mRenderHeight = mScreenHeight;
    
    _innerGlPrepare();
    
    // Initialize View Matrix
    Mat4::createLookAt(0.0f, 0.0f, 5.0f,    // Camera see (x, y, z)
                       0.0f, 0.0f, 0.0f,    // Camera focus (center-x, center-y, center-z)
                       0.0f, 1.0f, 0.0f,    // Camera up direction (up-x, ip-y, up-z)
                       &mVMatrix);
    // the aspect ratio of screen
    float aspectRatio = (float) mScreenWidth / mScreenHeight;
    
    if (mScreenWidth > mScreenHeight) {
        Mat4::createOrthographicOffCenter(-aspectRatio, aspectRatio, -1, 1, 5, 7, &mProjMatrix);
        targetScreen = mProjMatrix.getInversed();
        targetScreen.scale(mScreenHeight/2.0f, mScreenHeight/2.0f, 1.0f);
        targetScreen.translate(1.0f, 1.0f, 0.f);
        
    } else {
        Mat4::createOrthographicOffCenter(-1, 1,-1/aspectRatio, 1/aspectRatio,  5, 7, &mProjMatrix);
        targetScreen = mProjMatrix.getInversed();
        targetScreen.scale(mScreenWidth/2.0f, mScreenHeight/2.0f, 1.0f);
        targetScreen.translate(1.0f, 1.0f, 0.f);
    }
    
    // the aspect ratio of video
    float vhRate = mRenderWidth / (float)mRenderHeight;
    float vhDstRate = aspectRatio;
    
    /* 初始化 mMMatrix 成单位矩阵 */
    mMMatrix = Mat4::IDENTITY;
    if (mScreenWidth >= mScreenHeight) { // 屏幕宽度大于高度
        if (vhRate <= vhDstRate) {
            float scale = (mScreenHeight/mRenderHeight) * (mRenderWidth/mScreenWidth);
            mMMatrix.scale(scale*aspectRatio, 1.0f, 1.0f);
        } else {
            float scale = (mRenderHeight/mScreenHeight)*(mScreenWidth/mRenderWidth);
            mMMatrix.scale(aspectRatio, scale, 1.0f);
        }
    } else {
        if (vhRate <= vhDstRate) {
            float scale = (mScreenHeight/mRenderHeight)*(mRenderWidth/mScreenWidth);
            mMMatrix.scale(scale, 1.0f/aspectRatio, 1.0f);
        } else {
            float scale = (mRenderHeight/mScreenHeight)*(mScreenWidth/mRenderWidth);
            mMMatrix.scale(1.0f, scale/aspectRatio, 1.0f);
        }
    }
    
    // MVP = Projection * View * Model;
    mMVPMatrix = mVMatrix*mMMatrix;
    mMVPMatrix = mProjMatrix*mMVPMatrix;
    _setMVPMatrix(mMVPMatrix);
    return HB_OK;
}

void CSSpriteService::_bindTextureCache() {
    CSTextureCache::setCurrentCache(mTextureCacheIndex);
}

void CSSpriteService::setScreenSize(int width, int height) {
    mScreenWidth = width;
    mScreenHeight = height;
}

void CSSpriteService::_setMVPMatrix (const Mat4& mvp) {
    mMatrixShader->setMVPMatrix(mvp);
}

void CSSpriteService::setupVBO()
{
    glGenBuffers(2, mQuadbuffersVBO);
    
    glBindBuffer(GL_ARRAY_BUFFER, mQuadbuffersVBO[0]);
    glBufferData(GL_ARRAY_BUFFER, V3F_C4F_T2F_SIZE * mTextureVboSize, mQuadVerts, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mQuadbuffersVBO[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(mQuadIndices[0]) * mTextureVboIndex, mQuadIndices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    CHECK_GL_ERROR_DEBUG();
}

int CSSpriteService::begin() {
    if (mBDrawing) {
        LOGE("Sprite Service >>> reinto drawing !");
        return HB_ERROR;
    }
    ////////////////////////////////////////////////////////////
    // Begin to draw
    
    // Save old fbo.
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &mOldFramebufferObject);
    
    /** Active  mFramebufferObject */
    mFramebufferObject.enable();
    mBDrawing = true;
    return HB_OK;
}

int CSSpriteService::update() {
    return HB_OK;
}

int CSSpriteService::end() {
    if (mEnableFilpShader) {
        mBitmapFBO.enable();
        mFilpShader.draw(mFramebufferObject.getTexName(), nullptr);
    }
    
    /** clean screen */
    glBindFramebuffer(GL_FRAMEBUFFER, mOldFramebufferObject);
    glViewport(0, 0, mScreenWidth, mScreenHeight);
    
    glClearColor(mFramebufferBackgroundColor.x, mFramebufferBackgroundColor.y, mFramebufferBackgroundColor.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    if (!mEnableFilpShader) {
        // Draw final result mFramebufferObject to screen. */
        mMatrixShader->draw(mFramebufferObject.getTexName(), nullptr);
    }
    
    mBDrawing = false;
    return HB_OK;
}
    
void CSSpriteService::releaseVBO() {
    glDeleteBuffers(2, mQuadbuffersVBO);
}

void CSSpriteService::setAlphaBlending(bool enable)
{   /**
     * @def MT_BLEND_SRC
     * default gl blend src function. Compatible with premultiplied alpha images.
     */
#define CS_BLEND_SRC GL_ONE
#define CS_BLEND_DST GL_ONE_MINUS_SRC_ALPHA
    if (enable)
        glBlendFunc(CS_BLEND_SRC, CS_BLEND_DST);
    else
        glBlendFunc(GL_ONE, GL_ZERO);
    CHECK_GL_ERROR_DEBUG();
}

void CSSpriteService::setDepthTest(bool enable)
{
    if (enable) {
        glClearDepth(1.0f);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        //        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    }
    else
        glDisable(GL_DEPTH_TEST);
    
    CHECK_GL_ERROR_DEBUG();
}

}

