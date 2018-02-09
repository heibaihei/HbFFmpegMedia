//
//  CSView.hpp
//  Sample
//
//  Created by zj-db0519 on 2018/1/26.
//  Copyright © 2018年 meitu. All rights reserved.
//

#ifndef CSView_h
#define CSView_h

#include <stdio.h>

typedef struct GLFWwindow GLFWwindow;

namespace HBMedia {
    
    typedef struct GLContextAttrs
    {
        int redBits;
        int greenBits;
        int blueBits;
        int alphaBits;
        int depthBits;
        int stencilBits;
    } GLContextAttrs;
    
    typedef class CSView {
    public:
        CSView();
        
        virtual ~CSView();
        
        /**
         *  准备工作
         */
        int prepare();
        
        GLFWwindow *getWindows() { return mVideoGLFWWindow; }
        
    public: /** 功能性配置接口 */
        /**
         *  static method and member so that we can modify it on all platforms before create OpenGL context
         */
        void setGLContextAttrs(const GLContextAttrs& glContextAttrs);
        GLContextAttrs getGLContextAttrs();
        
        /**
         *  相关窗口资源的初始化
         */
        int _windowsInitial();
        
        void setViewSize(int width, int height);
        
    protected:
        
    private:
        int mWindowsWidth;
        int mWindowsHeight;
        GLFWwindow    *mVideoGLFWWindow;
        GLContextAttrs mGlContextAttrs;
    } CSView;
    
}

#endif /* CSView_h */
