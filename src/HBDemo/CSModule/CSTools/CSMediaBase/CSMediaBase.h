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
         *  提供外部取帧接口
         *  @param [in] pProvider 图像帧提供者
         *  @param [in] clock 当前外部时钟
         *  @param [in] type  媒体类型
         *  @param [out] pTargetFrame 得到的图像帧
         *  @return 1 表示拿到帧
         *          0 表示没有拿到帧，但是需要拿旧帧进行渲染
         *          -1 表示没有拿到帧，无需渲染,需要重入取帧
         *          -2 表示没有拿到帧，需要等待一段时间后再进入取帧
         *          -3 表示没有拿到帧，并且发生异常
         */
        static int fetchNextFrame(CSMediaBase *pProvider, int64_t clock, STREAM_TYPE type, AVFrame **pTargetFrame);
        
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

        /**
         *  音频参数参数信息
         */
        void setInAudioMediaParams(AudioParams& pParams);
        AudioParams *getInAudioMediaParams() { return &mSrcAudioParams; }
        
        void setOutAudioMediaParams(AudioParams& pParams);
        AudioParams *getOutAudioMediaParams() { return &mTargetAudioParams; }
        
        /**
         *  视频媒体参数信息
         */
        void setInImageMediaParams(ImageParams& pParams);
        ImageParams *getInImageMediaParams() { return &mSrcVideoParams; }
        
        void setOutImageMediaParams(ImageParams& pParams);
        ImageParams *getOutImageMediaParams() { return &mTargetVideoParams; }
        
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
        
        MediaRoleType mRoleType;
        bool    mAbort;
        /** 解码器状态 */
        uint64_t mState;
    } CSMediaBase;
}
#endif /* CSMediaBase_h */
