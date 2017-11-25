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

        virtual int prepare() = 0;

        virtual int start() = 0;

        virtual int stop() = 0;

        virtual int release() = 0;

        /**
     *  配置输入的音频文件
     */
        void setInputPicMediaFile(char *file);
        char *getInputPicMediaFile() { return mSrcPicMediaFile; };

        /**
         *  配置输出的音频文件
         */
        void setOutputPicMediaFile(char *file);
        char *getOutputPicMediaFile() { return mTrgPicMediaFile; }

    private:
        /**
         *  媒体数据输入信息
         */
        char            *mSrcPicMediaFile;
        FILE            *mSrcPicFileHandle;

        /**
         *  媒体数据输出信息
         */
        char            *mTrgPicMediaFile;
        FILE            *mTrgPicFileHandle;
    } CSMediaBase;
}
#endif /* CSMediaBase_h */
