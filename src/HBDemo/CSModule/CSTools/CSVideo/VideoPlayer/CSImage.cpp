//
//  CSImage.cpp
//  Sample
//
//  Created by zj-db0519 on 2018/1/24.
//  Copyright © 2018年 meitu. All rights reserved.
//

#include "CSImage.h"
#include "CSCommon.h"


namespace HBMedia {

CSImage::CSImage()
{
    mPixels = NULL;
    mPixelSize = 0;
    mWidth = mHeight = 0;
    mDataLineSize = 0;
    mPixelFormat = GL_INVALID_VALUE;
    mUpdated = false;
}

CSImage::~CSImage()
{
    freePixels();
}
    
bool CSImage::initWithImageInfo(int pWidth, int pHeight, GLenum format, bool needMallocPixels)
{
    mPixelFormat = format;
    mWidth = pWidth;
    mHeight = pHeight;
    
    size_t size;
    if(format == GL_RGBA)
    {
        mDataLineSize = mWidth * 4;
        size = mDataLineSize * mHeight;
    }
    else if(format == GL_RGB)
    {
        mDataLineSize = mWidth * 3;
        int ret = mDataLineSize % 4;
        if (ret) {
            mDataLineSize += (4 - ret);
        }
        size = mDataLineSize * mHeight;
    }
    else
    {
        LOGE("Invalid format: %d, format must be GL_RGB or GL_RGBA", format);
        return false;
    }
    
    if (needMallocPixels) {
        if(!mallocPixels(size)) {
            LOGE("CSImage malloc pixels failed !");
            return false;
        }
    }
    
    mPixelSize = size;
    return true;
}
    
bool CSImage::mallocPixels(size_t size)
{
    if(mPixels != NULL) {
        if(mPixelSize < size) {
            void* p = av_realloc(mPixels, size);
            if(!p) {
                LOGE("CSImage realloc failed (%p, %zu)", mPixels, size);
                return false;
            }
            else
                mPixels = p;
        }
    }
    else {
        if((mPixels = av_malloc(size)) == NULL) {
            LOGE("CSImage malloc failed (%zu)", size);
            return false;
        }
    }
    
    return true;
}

void CSImage::freePixels()
{
    if(mPixels != NULL) {
        av_free(mPixels);
        mPixels = NULL;
    }
}
    
bool CSImage::isUpdated() const {
    return mUpdated;
}

void CSImage::setUpdated(bool update) {
    mUpdated = update;
}

void* CSImage::getPixels() const {
    return mPixels;
}

int CSImage::getWidth() const {
    return mWidth;
}

int CSImage::getHeight() const {
    return mHeight;
}

int CSImage::getLineSize() const {
    return mDataLineSize;
}

GLenum CSImage::getFormat() const {
    return mPixelFormat;
}

}
