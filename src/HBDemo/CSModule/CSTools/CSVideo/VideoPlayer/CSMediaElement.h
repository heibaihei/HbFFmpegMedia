//
//  CSMediaElement.h
//  Sample
//
//  Created by zj-db0519 on 2018/2/12.
//  Copyright © 2018年 meitu. All rights reserved.
//

#ifndef CSMediaElement_h
#define CSMediaElement_h

#include <stdio.h>

namespace HBMedia {

typedef class CSSpriteService CSSpriteService;

/**
 *  多媒体播放节点信息
 */
typedef struct EltMediaInfo {
    int64_t mStartPos;
    int64_t mDuration;
    int64_t mLength;
    int64_t mFileStartPos;
    float mSpeed;
} EltMediaInfo;

void eltMediaInfoInitial(EltMediaInfo *pInfo);

typedef class CSMediaElement {
public:
    CSMediaElement();
    ~CSMediaElement();
    
    int prepare();
    
    int start();
    
    int stop();
    
    int release();
    
    int fetchNextFrame(int64_t clock, CSSpriteService *pSpriteService);
    
public:
    /**
     *  获取媒体信息
     */
    int64_t getStartPos() { return mWorkMediaInfo.mStartPos; }
    int64_t getDuration() { return (mWorkMediaInfo.mDuration + mWorkMediaInfo.mLength); }
    int64_t getSpeed() { return mWorkMediaInfo.mSpeed; }
    
protected:
    
private:
    EltMediaInfo mOriginalMediaInfo;
    EltMediaInfo mWorkMediaInfo;
} CSMediaElement;
    
}

#endif /* CSMediaElement_h */
