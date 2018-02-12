//
//  CSTextureCache.cpp
//  Sample
//
//  Created by zj-db0519 on 2018/1/26.
//  Copyright © 2018年 meitu. All rights reserved.
//

#include "CSTextureCache.h"
#include "CSLog.h"
#include "CSTexture.h"

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

    /**
     *  TODO: Remain to finished !
     */
void CSTextureCache::releaseTexture(CSTexture* texture)
{
    if(texture)
    {
        if(texture->getRef() == 1)
        {
#if 0
            //Release by FileHandle
            FileHandle* file = texture->getFileHandle();
            if (file) {
                auto it = _textures[currentIndex].find(file->getFullPathForFile());
                if (it != _textures[currentIndex].end()) {
                    _textures[currentIndex].erase(it);
                }
            }else{
                //Release by FileName
                const std::string &fileName = texture->getFileName();
                if(!fileName.empty()){
                    auto it = _textures[currentIndex].find(fileName);
                    if (it != _textures[currentIndex].end()) {
                        _textures[currentIndex].erase(it);
                    }
                }
            }
#endif
            texture->release();
        }
        
        texture->decreaseRef();
    }
}

}
