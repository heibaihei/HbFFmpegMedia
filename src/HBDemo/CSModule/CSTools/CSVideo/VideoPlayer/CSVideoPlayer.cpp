//
//  CSVideoPlayer.cpp
//  Sample
//
//  Created by zj-db0519 on 2018/1/10.
//  Copyright © 2018年 meitu. All rights reserved.
//

/** Include GLEW */
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "CSVideoPlayer.h"
#include "CSMathBase.h"
#include "CSImage.h"
#include "CSTexture.h"
#include "CSSprite.h"
#include "CSSpriteService.h"
#include "CSView.h"



namespace HBMedia {
 
    CSVideoPlayer::CSVideoPlayer() {
        mWindowsWidth = 480;
        mWindowsHeight = 480;
        mFrameProvider = nullptr;
        mRenderService = new CSSpriteService();
        mView = new CSView();
    }
    
    CSVideoPlayer::~CSVideoPlayer() {
        release();
    }
    
    int CSVideoPlayer::release() {
        mFrameProvider = nullptr;
        
        return HB_OK;
    }
    
    int CSVideoPlayer::doShow() {
        
        do {
            glClear(GL_COLOR_BUFFER_BIT);

            _renderFrame();

            glfwSwapBuffers(mView->getWindows());
            glfwPollEvents();
            if (glfwGetKey(mView->getWindows(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(mView->getWindows(), GLFW_TRUE);
            }
        }
        while(glfwWindowShouldClose(mView->getWindows()) == 0);
        
        glfwTerminate();
        return HB_OK;
    }
    
    int CSVideoPlayer::prepare() {
        mView->setGLContextAttrs((GLContextAttrs){8, 8, 8, 8, 0, 0});
        mView->setViewSize(mWindowsWidth, mWindowsHeight);
        
        if (mView->prepare() != HB_OK) {
            LOGE("Video player >>> view prepare failed !");
            return HB_ERROR;
        }
        
        mRenderService->setScreenSize(mWindowsWidth, mWindowsHeight);
        if (mRenderService->prepare() != HB_OK) {
            LOGE("Video player >>> render service prepare failed !");
            return HB_ERROR;
        }

        return HB_OK;
    }
    
    int CSVideoPlayer::_renderFrame() {
        int HBErr = HB_OK;
        AVFrame *pNewFrame = av_frame_alloc();
        if (!pNewFrame) {
            LOGE("Video player >>> alloc new frame failed !");
            return HB_ERROR;
        }
        HBErr = CSMediaBase::fetchNextFrame(mFrameProvider, 0, CS_STREAM_TYPE_VIDEO, &pNewFrame);
        if (HBErr < 0) {
            LOGE("Video player >>> fetch next frame failed, %d !", HBErr);
            return HB_ERROR;
        }
        CSTexture *pTargetTexture = new CSTexture();
        CSSprite  *pTargetSprite = new CSSprite();
        
        CSImage *pTargetImage = new CSImage();
        pTargetImage->initWithImageInfo(mSrcVideoParams.mWidth, mSrcVideoParams.mHeight, GL_RGBA);
        uint8_t *pData[4] = {0};
        int dataLineSize[4] = {0};
        pData[0] = (uint8_t*)(pTargetImage->getPixels());
        dataLineSize[0] = pTargetImage->getLineSize();
        memcpy(pData[0], pNewFrame->data[0], pNewFrame->linesize[0]);
        pTargetImage->setUpdated(true);
        
        pTargetTexture->load(*pTargetImage);
        
        pTargetSprite->setTexture(pTargetTexture);
        mRenderService->pushSprite(pTargetSprite);
        
        /**
         *  更新纹理
         */
        mRenderService->begin();
        mRenderService->update();
        mRenderService->end();
        
        return HB_OK;
    }
    
    int CSVideoPlayer::_glPrepare() {
        /** 参照： GraphicsService::start 接口的实现 */
        
        return HB_OK;
    }
}
