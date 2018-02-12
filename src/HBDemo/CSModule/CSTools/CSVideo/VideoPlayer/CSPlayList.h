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
    
    int fetchNextFrame(int64_t clock, CSSpriteService *pSpriteService);
    
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
