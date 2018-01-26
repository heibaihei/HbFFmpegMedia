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
    
    bool isLoaded() const;
    
    virtual int getTexName() const;
    virtual int getWidth() const;
    virtual int getHeight() const;
    void setWidthAndHeight(int pWidth, int pHeight);
    
    static void initMaxTextureSize();
    static GLint getMaxTextureSize();
    
    void bind() const;
    
    virtual bool load(int textureID, GLenum format, int w, int h);
    virtual bool load(const CSImage& image, const NS_GLX::Size &out = NS_GLX::Size::ZERO);
    
    virtual void release();
    
protected:
    void unLoad();

protected:
    static const int TEXTURE_ID_INVALID = 0;
    static GLint mMaxTextureSize;
    
private:
    int    mWidth;
    int    mHeight;
    GLuint mTextureID;
    GLenum mTextureFormat;
    bool   mIsNeedRelease;
} CSTexture;
    
}

#endif /* CSTexture_h */
