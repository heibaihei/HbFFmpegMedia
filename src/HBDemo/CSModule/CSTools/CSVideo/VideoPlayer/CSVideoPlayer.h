//
//  CSVideoPlayer.h
//  Sample
//
//  Created by zj-db0519 on 2018/1/10.
//  Copyright © 2018年 meitu. All rights reserved.
//

#ifndef CSVideoPlayer_h
#define CSVideoPlayer_h

#include <stdio.h>

/** Include GLEW */
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "CSMediaBase.h"
#include "CSThreadContext.h"
#include "frame.h"
/**
 *  如何使用 SDL 画布进行渲染:
 *     ===> http://blog.csdn.net/vagrxie/article/details/5740993
 */

namespace HBMedia {

typedef class CSVideoPlayer : public CSMediaBase {
public:
    CSVideoPlayer();
    ~CSVideoPlayer();
    
    virtual int start() { return HB_OK; }
    /**
     *  必须主线程调用这个接口
     *  完成相关 gl 资源的准备工作
     */
    int prepare();
    
    /**
     *  必须主线程调用
     *  进入渲染，必须等所有准备工作都完成后，调用 player 的show 接口
     *  进行相关的图像展示工作
     */
    int doShow();
    
    /**
     *  释放 prepare 时候创建的资源
     */
    int release();
    
    void setVideoProvider(CSMediaBase *pProvider) { mFrameProvider = pProvider; }
protected:
    
    /**
     *  相关窗口资源的初始化
     */
    int _windowsInitial();
    
    /**
     *  渲染图像接口
     */
    int _renderFrame();
    
private:
    
private:
    int mWindowsWidth;
    int mWindowsHeight;
    GLFWwindow* mVideoGLFWWindow;
    CSMediaBase* mFrameProvider;
    
} CSVideoPlayer;
    
}

#endif /* CSVideoPlayer_h */
