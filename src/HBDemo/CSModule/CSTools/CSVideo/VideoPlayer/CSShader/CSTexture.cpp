//
//  CSTexture.cpp
//  Sample
//
//  Created by zj-db0519 on 2018/1/24.
//  Copyright © 2018年 meitu. All rights reserved.
//

#include "CSTexture.h"
#include "LogHelper.h"

namespace HBMedia {

GLint CSTexture::mMaxTextureSize = 0;

CSTexture::CSTexture() {
    mWidth = mHeight = 0;
    mTextureID = TEXTURE_ID_INVALID;
    mTextureFormat = GL_INVALID_VALUE;
    mIsNeedRelease = true;
}

CSTexture::~CSTexture() {
    mTextureID = TEXTURE_ID_INVALID;
}

bool CSTexture::load(int textureID, GLenum format, int w, int h) {
    mWidth = w;
    mHeight = h;
    mTextureID = textureID;
    mTextureFormat = format;
    mIsNeedRelease = false;
    return true;
}

bool CSTexture::load(const CSImage& image, const NS_GLX::Size &out) {
    
    if(image.getPixels() == NULL) {
        LOGE("Image null pixels!");
        return false;
    }
    
    int imageWidth = image.getWidth();
    int imageHeight = image.getHeight();
    
    if ((imageWidth > mMaxTextureSize) || (imageHeight > mMaxTextureSize)) {
        LOGE("Image (%d x %d) is bigger than the supported (%d x %d)", imageWidth, imageHeight, maxTextureSize, maxTextureSize);
        return false;
    }
    
    if(mWidth != imageWidth \
       || mHeight != imageHeight
       || mTextureFormat != image.getFormat() \
       || mTextureID == TEXTURE_ID_INVALID)
    {
        unLoad();
        GLuint textures[1];
        glGenTextures(1, textures);
        if(textures[0] != 0) {
            mTextureFormat = image.getFormat();
            setWidthAndHeight(imageWidth, imageHeight);
            glBindTexture(GL_TEXTURE_2D, textures[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, mTextureFormat, mWidth, mHeight, 0, mTextureFormat, GL_UNSIGNED_BYTE, image.getPixels());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            mTextureID = textures[0];
            return true;
        }
        else {
            LOGE("ERROR in loadTexture!");
            return false;
        }
    }
    else {
        bind();
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mWidth, mHeight, mTextureFormat, GL_UNSIGNED_BYTE, image.getPixels());
        return true;
    }
}
    
void CSTexture::bind() const {
    glBindTexture(GL_TEXTURE_2D, getTexName());
}

void CSTexture::unLoad() {
    if (mTextureID != TEXTURE_ID_INVALID)
    {
        if (mIsNeedRelease) {
            glDeleteTextures(1, &mTextureID);
        }
        mTextureID = TEXTURE_ID_INVALID;
    }
}

bool CSTexture::isLoaded() const {
    return getTexName() != TEXTURE_ID_INVALID;
}

void CSTexture::initMaxTextureSize() {
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mMaxTextureSize);
}

GLint CSTexture::getMaxTextureSize() {
    return mMaxTextureSize;
}

void CSTexture::release() {
    unLoad();
}

int CSTexture::getTexName() const {
    return mTextureID;
}

int CSTexture::getWidth() const {
    return mWidth;
}

int CSTexture::getHeight() const {
    return mHeight;
}

void CSTexture::setWidthAndHeight(int pWidth, int pHeight) {
    mWidth = pWidth;
    mHeight = pHeight;
}
    
}
