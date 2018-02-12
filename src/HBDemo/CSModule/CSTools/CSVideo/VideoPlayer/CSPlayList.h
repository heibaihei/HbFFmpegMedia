//
//  CSPlayList.h
//  Sample
//
//  Created by zj-db0519 on 2018/2/12.
//  Copyright © 2018年 meitu. All rights reserved.
//

#ifndef CSPlayList_h
#define CSPlayList_h

#include <stdio.h>
#include <list>
#include "CSDefine.h"

namespace HBMedia {

typedef class CSMediaElement CSMediaElement;
typedef class CSSpriteService CSSpriteService;

typedef class CSPlayList {
public:
    CSPlayList();
    ~CSPlayList();
    
    int prepare();
    
    int start();
    
    int stop();
    
    int release();
    
    /**
     *  @func fetchNextFrame 获取帧，并绘制成纹理，存储到 CSSpriteService 对象中
     *  @param clock 当前要渲染的时钟
     *  @param pSpriteService 绘制纹理服务
     *  @return:
     *     1 表示拿到帧，需要进行渲染；
     *     0 表示没有拿到帧，但是需要拿上一帧进行渲染；
     *     -1 表示没有拿到帧，需要重新尝试去拿帧；
     *     -2 表示没有拿到帧，需要等待一定时间后再去拿帧；
     *     -3 表示没有拿到帧，并且发生了异常;
     */
    int fetchNextFrame(int64_t clock, CSSpriteService *pSpriteService, STREAM_TYPE eMediaType);
    
public:
    /** 设置上级对象 */
    void setOpaque(void *opaque) { mOpaque = opaque; };

protected:
    
private:
    /** 播放器上级对象 */
    void *mOpaque;
    
    /** 播放时间线 */
    std::list<CSMediaElement *> mTimeline;
} CSPlayList;

}

#endif /* CSPlayList_h */
