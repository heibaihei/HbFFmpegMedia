//
//  CSSprite.h
//  Sample
//
//  Created by zj-db0519 on 2018/1/25.
//  Copyright © 2018年 meitu. All rights reserved.
//

#ifndef CSSprite_h
#define CSSprite_h

#include <stdio.h>


namespace HBMedia {

typedef class CSTexture CSTexture;
    
typedef class CSSprite {
public:
    CSSprite();
    
    virtual ~CSSprite();
    
    /** 设置外部纹理 */
    void setTexture(CSTexture* texture);
protected:
    
private:
    CSTexture *mTexture;
} CSSprite;
    
}

#endif /* CSSprite_h */
