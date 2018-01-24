//
//  CSImage.h
//  Sample
//
//  Created by zj-db0519 on 2018/1/24.
//  Copyright © 2018年 meitu. All rights reserved.
//

#ifndef CSImage_h
#define CSImage_h

#include <stdio.h>
#include <OpenGL/gl.h>

namespace HBMedia {
    
    class Ref {
    public:
        virtual ~Ref() {
        }
        
        void increaseRef() {
            ++mRef;
        }
        
        void decreaseRef() {
            --mRef;
            if (mRef == 0) {
                delete this;
            }
        }
        
        int getRef() const {
            return mRef;
        }
        
    protected:
        Ref() : mRef(1) {
        }
        
        int mRef;
    };
    
    typedef class CSImage : public Ref
    {
    public:
        CSImage();
        
        virtual ~CSImage();
        
        virtual bool initWithImageInfo(int pWidth, int pHeight, GLenum format, bool needMallocPixels = true);
        
        void* getPixels() const;
        int getWidth() const;
        int getHeight() const;
        int getLineSize() const;
        GLenum getFormat() const;
        bool isUpdated() const;
        void setUpdated(bool update);
        
    protected:
        void freePixels();
        bool mallocPixels(size_t size);
    
    private:
        void*  mPixels;
        size_t mPixelSize;
        int    mWidth;
        int    mHeight;
        GLenum mPixelFormat;
        
        // number of bytes per line
        int    mDataLineSize;
        //changed pixels, Texture2D need to reload
        bool   mUpdated;
    } CSImage;
}

#endif /* CSImage_h */
