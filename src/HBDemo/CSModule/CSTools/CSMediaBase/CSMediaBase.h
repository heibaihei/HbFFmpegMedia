//
//  CSMediaBase.hpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/11/25.
//  Copyright © 2017 meitu. All rights reserved.
//

#ifndef CSMediaBase_h
#define CSMediaBase_h

#include <stdio.h>
#include "CSDefine.h"
#include "CSCommon.h"
#include "CSLog.h"

namespace HBMedia {

    typedef class CSMediaBase {
    public:
        CSMediaBase();

        ~CSMediaBase();

        int baseInitial();

        virtual int prepare() = 0;

        virtual int start() = 0;

        virtual int stop() = 0;

        virtual int release() = 0;

        /**
         *  配置输入媒体类型
         */
        void setInMediaType(MEDIA_DATA_TYPE type) { mInMediaType = type; };
        MEDIA_DATA_TYPE getInMediaType() { return mInMediaType; };

        /**
         *  配置输出的媒体类型
         */
        void setOutMediaType(MEDIA_DATA_TYPE type) { mOutMediaType = type; };
        MEDIA_DATA_TYPE getOutMediaType() { return mOutMediaType; }

        /**
         *  配置输入的媒体文件
         */
        void setInputPicMediaFile(char *file);
        char *getInputPicMediaFile() { return mSrcPicMediaFile; };

        /**
         *  配置输出的媒体文件
         */
        void setOutputPicMediaFile(char *file);
        char *getOutputPicMediaFile() { return mTrgPicMediaFile; }

    protected:
        /**
         *  媒体数据输入信息
         */
        MEDIA_DATA_TYPE  mInMediaType;
        AVFormatContext *mPInVideoFormatCtx;
        char            *mSrcPicMediaFile;
        FILE            *mSrcPicFileHandle;

        /**
         *  媒体数据输出信息
         */
        MEDIA_DATA_TYPE  mOutMediaType;
        AVFormatContext *mPOutVideoFormatCtx;
        char            *mTrgPicMediaFile;
        FILE            *mTrgPicFileHandle;
    } CSMediaBase;
}
#endif /* CSMediaBase_h */
