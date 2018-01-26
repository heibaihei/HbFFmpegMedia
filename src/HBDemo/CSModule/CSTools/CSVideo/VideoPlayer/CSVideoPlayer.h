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

#include "CSMediaBase.h"
#include "CSThreadContext.h"
#include "frame.h"
/**
 *  如何使用 SDL 画布进行渲染:
 *     ===> http://blog.csdn.net/vagrxie/article/details/5740993
 */

namespace HBMedia {

typedef class CSView CSView;
typedef class CSSpriteService CSSpriteService;
    
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
    
public: /** 配置下相关 */
    
    
protected:
    
    /**
     *  渲染图像接口
     */
    int _renderFrame();
    
    int _glPrepare();
private:
    
private:
    /**
     *  视频画面宽高，即显示屏幕的宽高
     */
    int mWindowsWidth;
    int mWindowsHeight;
    
    CSView *mView;
    CSMediaBase* mFrameProvider;
    CSSpriteService *mRenderService;
} CSVideoPlayer;
    
}

#endif /* CSVideoPlayer_h */
