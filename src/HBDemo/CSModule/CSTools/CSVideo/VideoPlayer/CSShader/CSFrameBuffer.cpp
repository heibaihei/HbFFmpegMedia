//
//  CSFrameBuffer.cpp
//  Sample
//
//  Created by zj-db0519 on 2018/1/26.
//  Copyright © 2018年 meitu. All rights reserved.
//

#include "CSFrameBuffer.h"
#include "CSGlUtil.h"
#include "CSLog.h"

namespace HBMedia {

CSFrameBuffer::CSFrameBuffer(bool enableDepth)
:mWidth(0)
,mHeight(0)
,mFramebufferName(CSGlUtils::INVALID)
,mTexName(CSGlUtils::INVALID)
,mEnableDepth(enableDepth)
,mDepthBuffer(CSGlUtils::INVALID) {
    
}

CSFrameBuffer::~CSFrameBuffer() {
    
}

void CSFrameBuffer::setup(const int width, const int height, const int texture) {
    if (mWidth == width && mHeight == height) {
        if (texture <= 0 || mTexName == texture) {
            return;
        }
    }
    
    int args[1] = {0};
    // Check width height parameter is all right.
    
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, args);
    if (width > args[0] || height > args[0]) {
        LOGE("GL_MAX_TEXTURE_SIZE %d", args[0]);
    }
    
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, args);
    if (width > args[0] || height > args[0]) {
        LOGE("GL_MAX_RENDERBUFFER_SIZE %d", args[0]);
    }
    
    // Save current binding status。
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, args);
    const int saveFramebuffer = args[0];
    
    // do not need to delete fbo, just delete texture
    //    // Delete FBO that last time created。
    //    release();
    
    if (mFramebufferName != CSGlUtils::INVALID) {
        // release texture that last time created。
        releaseTexture();
    } else {
        // Generate a Framebuffer
        glGenFramebuffers(1, (GLuint*)args);
        mFramebufferName = args[0];
    }
    
    // Bind Framebuffer to GL_FRAMEBUFFER
    glBindFramebuffer(GL_FRAMEBUFFER, mFramebufferName);
    
    if (texture > 0) {
        needReleaseTexture = false;
        
        mTexName = texture;
    } else {
        needReleaseTexture = true;
        
        // Offscreen position framebuffer texture target
        glGenTextures(1, (GLuint*)args);
        mTexName = args[0];
        glBindTexture(GL_TEXTURE_2D, mTexName);
        CSGlUtils::setupSampler(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR);
        // Create a RGBA texture.
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    }
    
    // Bind the texture to Framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexName, 0);
    
    createDepthBuffer(width, height);
    // attach the renderbuffer to depth attachment point
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffer);
    
    mWidth = width;
    mHeight = height;
    
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Check all have done. And Complete without error.
    const int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("Failed to initialize framebuffer object %d", status);
        release();  // release all GL resource.
    }
    
    // Rebind old status buffers.
    glBindFramebuffer(GL_FRAMEBUFFER, saveFramebuffer);
}

void CSFrameBuffer::createDepthBuffer(int width, int height) {
    if (mEnableDepth) {
        if (mWidth == width && mHeight == height) {
            return;
        }
        
        releaseDepthBuffer();
        
        GLint oldRenderBuffer(0);
        glGetIntegerv(GL_RENDERBUFFER_BINDING, &oldRenderBuffer);
        
        glGenRenderbuffers(1, (GLuint*)&mDepthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
        
        glBindRenderbuffer(GL_RENDERBUFFER, oldRenderBuffer);
    }
}

void CSFrameBuffer::releaseDepthBuffer() {
    if(glIsRenderbuffer(mDepthBuffer)) {
        glDeleteRenderbuffers(1, (GLuint*)&mDepthBuffer);
        mDepthBuffer = CSGlUtils::INVALID;
    }
}

void CSFrameBuffer::releaseTexture() {
    if (mTexName != CSGlUtils::INVALID) {
        if (needReleaseTexture) {
            glDeleteTextures(1, (GLuint *)(&mTexName));
        }
        mTexName = CSGlUtils::INVALID;
    }
}

void CSFrameBuffer::release() {
    if (mFramebufferName != CSGlUtils::INVALID) {
        releaseTexture();
        
        releaseDepthBuffer();
        
        glDeleteFramebuffers(1, (GLuint *)(&mFramebufferName));
        mFramebufferName = CSGlUtils::INVALID;
        
        mWidth = 0;
        mHeight = 0;
    }
}


void CSFrameBuffer::enable() const {
    glBindFramebuffer(GL_FRAMEBUFFER, mFramebufferName);
    glViewport(0, 0, mWidth, mHeight);
}


const CSImage &CSFrameBuffer::getBitmap() {
    return getBitmap(0, false);
}

const CSImage &CSFrameBuffer::getBitmap(const int orientation) {
    return getBitmap(orientation, false);
}

const CSImage &CSFrameBuffer::getBitmap(const int orientation, const bool mirror) {
    if (image.getWidth() != mWidth || image.getHeight() != mHeight) {
        image.initWithImageInfo(mWidth, mHeight, GL_RGBA);
    }
    
    int args[1] = {0};
    // Save current binding status。
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, args);
    const int saveFramebuffer = args[0];
    
    // Catch FBO info to buffer.
    enable();
    glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, image.getPixels());
    
    // Rebind old status buffers.
    glBindFramebuffer(GL_FRAMEBUFFER, saveFramebuffer);
    
    return image;
}

void CSFrameBuffer::getRGBAPixels(void *pixels) {
    int args[1] = {0};
    // Save current binding status。
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, args);
    const int saveFramebuffer = args[0];
    
    enable();
    glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    
    // Rebind old status buffers.
    glBindFramebuffer(GL_FRAMEBUFFER, saveFramebuffer);
}
    
}

