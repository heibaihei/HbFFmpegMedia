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

        /**
         *  基础初始化
         */
        int baseInitial();

        /**
         *  准备工作，对外接口
         */
        virtual int prepare() = 0;

        /**
         *  启动解码
         */
        virtual int start() = 0;

        /**
         *  关闭解码
         */
        virtual int stop();

        /**
         *  释放内部资源
         */
        virtual int release();

        uint64_t getStatus() { return mState; }
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
        void setInMediaFile(char *file);
        char *getInMediaFile() { return mSrcMediaFile; }

        /**
         *  配置输出的媒体文件
         */
        void setOutMediaFile(char *file);
        char *getOutMediaFile() { return mTrgMediaFile; }

        void setInAudioMediaParams(AudioParams& pParams);
        AudioParams *getInAudioMediaParams() { return &mSrcAudioParams; }
        
        void setOutAudioMediaParams(AudioParams& pParams);
        AudioParams *getOutAudioMediaParams() { return &mTargetAudioParams; }
    protected:
        int _InMediaInitial();

        int _OutMediaInitial();

        bool mIsNeedTransfer;
        
        /**
         *  媒体数据输入信息
         */
        MEDIA_DATA_TYPE  mInMediaType;
        AVFormatContext *mPInMediaFormatCtx;
        char            *mSrcMediaFile;
        FILE            *mSrcMediaFileHandle;
        AudioParams      mSrcAudioParams;
        ImageParams      mSrcVideoParams;
        
        /**
         *  媒体数据输出信息
         */
        MEDIA_DATA_TYPE  mOutMediaType;
        AVFormatContext *mPOutMediaFormatCtx;
        char            *mTrgMediaFile;
        FILE            *mTrgMediaFileHandle;
        AudioParams      mTargetAudioParams;
        ImageParams      mTargetVideoParams;
        
        bool    mAbort;
        /** 解码器状态 */
        uint64_t mState;
    } CSMediaBase;
}
#endif /* CSMediaBase_h */
