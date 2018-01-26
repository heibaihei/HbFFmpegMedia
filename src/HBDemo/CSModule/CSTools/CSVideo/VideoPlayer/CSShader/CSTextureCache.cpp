//
//  CSTextureCache.cpp
//  Sample
//
//  Created by zj-db0519 on 2018/1/26.
//  Copyright © 2018年 meitu. All rights reserved.
//

#include "CSTextureCache.h"
#include "CSLog.h"

namespace HBMedia {

const int CSTextureCache::MAX_TEXTURE_CACHE = 3;
int CSTextureCache::mCurrentIndex = 0;

CSTextureCache::CSTextureCache() {
    
}

CSTextureCache::~CSTextureCache() {
    
}

void CSTextureCache::setCurrentCache(int index)
{
    if(index >=0 && index < MAX_TEXTURE_CACHE)
        mCurrentIndex = index;
    else
        LOGE("TextureCache maxCache is %i, index is: %i", MAX_TEXTURE_CACHE, index);
}

}
