//
//  CSSprite.cpp
//  Sample
//
//  Created by zj-db0519 on 2018/1/25.
//  Copyright © 2018年 meitu. All rights reserved.
//

#include "CSSprite.h"
#include "CSTexture.h"

namespace HBMedia {
    
CSSprite::CSSprite(){
    mTexture = nullptr;
}

CSSprite::~CSSprite() {
    if (mTexture) {
        /** 释放纹理逻辑 */
    }
}
    
void CSSprite::setTexture(CSTexture* texture) {
    if (!texture)
        return;
    
    if (mTexture != texture) {
        mTexture = texture;
    }
    
}

}
