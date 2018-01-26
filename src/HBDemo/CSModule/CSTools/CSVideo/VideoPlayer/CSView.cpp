//
//  CSView.cpp
//  Sample
//
//  Created by zj-db0519 on 2018/1/26.
//  Copyright © 2018年 meitu. All rights reserved.
//

#include "CSView.h"

/** Include GLEW */
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "CSDefine.h"
#include "CSCommon.h"

namespace HBMedia {
    
CSView::CSView() {
    mWindowsWidth = 480;
    mWindowsHeight = 480;
    mVideoGLFWWindow = nullptr;
    /**
     *  default context attributions are setted as follows
     */
    mGlContextAttrs = (GLContextAttrs){5, 6, 5, 0, 16, 0};
}

CSView::~CSView() {
    if (mVideoGLFWWindow) {
        glfwWindowShouldClose(mVideoGLFWWindow);
        glfwDestroyWindow(mVideoGLFWWindow);
        glfwTerminate();
        mVideoGLFWWindow = nullptr;
    }
}

int CSView::prepare() {
    if (_windowsInitial() != HB_OK) {
        LOGE("CSView >>> windows initial failed !");
        return HB_ERROR;
    }
    return HB_OK;
}

int CSView::_windowsInitial() {
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

void CSView::setGLContextAttrs(const GLContextAttrs& glContextAttrs) {
    mGlContextAttrs = glContextAttrs;
}

GLContextAttrs CSView::getGLContextAttrs() {
    return mGlContextAttrs;
}
    
void CSView::setViewSize(int width, int height) {
    mWindowsWidth = width;
    mWindowsHeight = height;
}

}
