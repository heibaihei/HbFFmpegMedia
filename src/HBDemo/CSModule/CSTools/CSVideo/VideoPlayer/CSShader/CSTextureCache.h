//
//  CSTextureCache.h
//  Sample
//
//  Created by zj-db0519 on 2018/1/26.
//  Copyright © 2018年 meitu. All rights reserved.
//

#ifndef CSTextureCache_h
#define CSTextureCache_h

#include <stdio.h>

namespace HBMedia {
    
    typedef class CSTexture CSTexture;
    
    typedef class CSTextureCache {
    public:
        static const int MAX_TEXTURE_CACHE;
        
        CSTextureCache();
        
        ~CSTextureCache();
        
        static void setCurrentCache(int index);
        
        static void releaseTexture(CSTexture* texture);
        
    private:
        static int mCurrentIndex;
    } CSTextureCache;
}
#endif /* CSTextureCache_h */
