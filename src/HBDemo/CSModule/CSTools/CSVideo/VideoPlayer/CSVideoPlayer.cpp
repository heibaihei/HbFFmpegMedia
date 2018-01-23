//
//  CSVideoPlayer.cpp
//  Sample
//
//  Created by zj-db0519 on 2018/1/10.
//  Copyright © 2018年 meitu. All rights reserved.
//

#include "CSVideoPlayer.h"

namespace HBMedia {
 
    CSVideoPlayer::CSVideoPlayer() {
        mVideoGLFWWindow = nullptr;
        mWindowsWidth = 480;
        mWindowsHeight = 480;
    }

    
    CSVideoPlayer::~CSVideoPlayer() {
        if (mVideoGLFWWindow) {
            glfwWindowShouldClose(mVideoGLFWWindow);
            glfwDestroyWindow(mVideoGLFWWindow);
            glfwTerminate();
            mVideoGLFWWindow = nullptr;
        }
        
    }
    
    int CSVideoPlayer::release() {
        
        return HB_OK;
    }
    
    int CSVideoPlayer::doShow() {
        
        do {
            glClear(GL_COLOR_BUFFER_BIT);
            
            _renderFrame();
            
            glfwSwapBuffers(mVideoGLFWWindow);
            glfwPollEvents();
            if (glfwGetKey(mVideoGLFWWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(mVideoGLFWWindow, GLFW_TRUE);
            }
        }
        while(glfwWindowShouldClose(mVideoGLFWWindow) == 0);
        
        glfwTerminate();
        return HB_OK;
    }
    
    int CSVideoPlayer::prepare() {
    
        if (_windowsInitial() != HB_OK) {
            LOGE("Video player >>> windows initial failed !");
            return HB_ERROR;
        }
        return HB_OK;
    }
    
    int CSVideoPlayer::_renderFrame() {
        
        return HB_OK;
    }
    
    int CSVideoPlayer::_windowsInitial() {
        if (!glfwInit()) {
            LOGE("Video player >>> initial glfw failed !");
            return HB_ERROR;
        }
        
        glfwWindowHint(GLFW_SAMPLES, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        
        mVideoGLFWWindow = glfwCreateWindow(mWindowsWidth, mWindowsHeight, "CS Video Player", NULL, NULL);
        if (mVideoGLFWWindow == NULL){
            LOGE("Video player >>> Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
            glfwTerminate();
            return HB_ERROR;
        }
        
        glfwMakeContextCurrent(mVideoGLFWWindow);
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            LOGE("Video player >>> Failed to initialize GLEW\n");
            glfwTerminate();
            return HB_ERROR;
        }
        
        glfwSetInputMode(mVideoGLFWWindow, GLFW_STICKY_KEYS, GL_TRUE);
        glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
        
        return HB_OK;
    }
    
}
