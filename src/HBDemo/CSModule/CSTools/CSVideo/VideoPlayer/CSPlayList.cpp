//
//  CSPlayList.cpp
//  Sample
//
//  Created by zj-db0519 on 2018/2/12.
//  Copyright © 2018年 meitu. All rights reserved.
//

#include "CSPlayList.h"
#include "CSCommon.h"
#include "CSMediaElement.h"
#include "CSSpriteService.h"

namespace HBMedia {
    
CSPlayList::CSPlayList() {
    mOpaque = nullptr;
    mTimeline.clear();
}

CSPlayList::~CSPlayList() {
    
}
    
int CSPlayList::prepare() {
    
    return HB_OK;
}

int CSPlayList::start() {
    
    return HB_OK;
}

int CSPlayList::stop() {
    
    return HB_OK;
}

int CSPlayList::release() {
    
    return HB_OK;
}

int CSPlayList::fetchNextFrame(int64_t clock, CSSpriteService *pSpriteService) {
    CSMediaElement* pElement = nullptr;
    std::list<CSMediaElement *>::iterator elementIterator = mTimeline.begin();
    for (; elementIterator != mTimeline.end(); elementIterator++) {
        pElement = *elementIterator;
        
        if (clock >= pElement->getStartPos() && clock < pElement->getDuration()) {
            pElement->fetchNextFrame(clock, pSpriteService);
        }
    }
    return HB_OK;
}

}

