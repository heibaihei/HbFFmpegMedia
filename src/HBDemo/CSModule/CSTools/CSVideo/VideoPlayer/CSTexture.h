//
//  CSTexture.h
//  Sample
//
//  Created by zj-db0519 on 2018/1/24.
//  Copyright © 2018年 meitu. All rights reserved.
//

#ifndef CSTexture_h
#define CSTexture_h

#include <stdio.h>
#include "CSImage.h"
#include "CSGeometry.h"

namespace HBMedia {
    
typedef class Texture : public Ref {
public:
    virtual ~Texture() {}
    /**
     * ReturnTexture resource id in OpenGL.
     *
     * @return return Texture。Invalidate Texture as {@code 0}
     */
    virtual int getTexName() const = 0;
    
    /**
     * Get Texture width.
     *
     * @return With of Texture.
     */
    virtual int getWidth() const = 0;
    
    /**
     * Get Texture height.
     *
     * @return Height of Texture
     */
    virtual int getHeight() const = 0;
    
    /**
     * Release all resource that Texture allocated
     */
    virtual void release() = 0;
    
} Texture;
    
typedef class CSTexture : public Texture
{
public:
    CSTexture();
    virtual ~CSTexture();
    
    virtual bool load(const CSImage& image, const glx::Size &out = glx::Size::ZERO);
protected:
    
private:
    int    mWidth;
    int    mHeight;
    GLuint mTextureID;
    GLenum mTextureFormat;
    bool   mIsNeedRelease;
} CSTexture;
    
}

#endif /* CSTexture_h */
