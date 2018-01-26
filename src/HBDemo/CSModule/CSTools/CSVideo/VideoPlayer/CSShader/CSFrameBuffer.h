//
//  CSFrameBuffer.hpp
//  Sample
//
//  Created by zj-db0519 on 2018/1/26.
//  Copyright © 2018年 meitu. All rights reserved.
//

#ifndef CSFrameBuffer_h
#define CSFrameBuffer_h

#include <stdio.h>
#include "CSImage.h"

namespace HBMedia {
    
typedef class CSFrameBuffer {
public:
    CSFrameBuffer(bool enableDepth = false);
    virtual ~CSFrameBuffer();
    
    /**
     * Get framebuffer object width.
     *
     * @return framebuffer width.
     */
    int getWidth() const { return mWidth; }
    
    /**
     * Get framebuffer object height.
     *
     * @return framebuffer height.
     */
    int getHeight() const { return mHeight; }
    
    /**
     * Get texture id in OpenG.
     *
     * @return Texture id.
     */
    int getTexName() const { return mTexName; }
    
    /**
     * Create specify size FBO.<p>
     * If FBO have not been created, just create a new FBO、
     * If (FBO) has been created, first release old (FBO) and then create new size FBO。
     *
     * @param width FBO width
     * @param height FBO height
     * @param texture if > 0, bind the texture and do not release, else create new texture
     * @throws IllegalArgumentException {@code width} and {@code height} is bigger than {@code GL_MAX_TEXTURE_SIZE} or {@code GL_MAX_RENDERBUFFER_SIZE} よりも大きい場合。
     * @throws RuntimeException FBO create fail!。
     */
    void setup(const int width, const int height, const int texture = -1);
    
    /**
     * Release OpenGL resource that have requested by {@link GLES20FramebufferObject}.
     */
    void release();
    
    
    /**
     * Make Framebuffer active.
     */
    void enable() const;
    
    /**
     * Get Framebuffer info as {@link Image} to return.
     *
     * @return {@link Image}
     */
    const CSImage &getBitmap();
    
    /**
     * Get Framebuffer info as {@link Bitmap} to return.
     *
     * @param orientation orientation in degree
     * @return {@link Bitmap}
     */
    const CSImage &getBitmap(const int orientation);
    
    /**
     * Get Framebuffer info as {@link Image} to return.
     *
     * @param orientation orientation want to save.
     * @param mirror
     * @return {@link Image}
     */
    const CSImage &getBitmap(const int orientation, const bool mirror);
    
    void getRGBAPixels(void *pixels);
    
    void setReleaseTexture(bool value){ needReleaseTexture = value; }
    
private:
    void releaseTexture();
    
    void createDepthBuffer(int width, int height);
    
    void releaseDepthBuffer();
    
private:
    /**
     * framebuffer width
     */
    int mWidth;
    
    /**
     * Framebuffer height。
     */
    int mHeight;
    
    /**
     * framebuffer OpenGL resource name 。
     */
    int mFramebufferName;
    
    /**
     * texture OpenGL name。
     */
    int mTexName;
    
    bool mEnableDepth;
    
    /**
     * depth renderbuffer
     */
    int mDepthBuffer;
    
    CSImage image;
    
    bool needReleaseTexture;
} CSFrameBuffer;
    
}

#endif /* CSFrameBuffer_h */
