//
//  CSVideoAnalysis.hpp
//  Sample
//
//  Created by zj-db0519 on 2017/12/6.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSVideoAnalysis_h
#define CSVideoAnalysis_h

#include <stdio.h>
#include <vector>
#include "CSVideoDecoder.h"

namespace HBMedia {

/** 根据 AVPictureType 类型的最大个数来定 */
#define MAX_PICTURE_TYPE  (8)

#define INFO_FRAME_TYPE    (1 << 0)
#define INFO_FRAME_ENCODER (1 << 1)
#define INFO_FRAME_DECODER (1 << 2)
#define INFO_FRAME_OTHER   (1 << 3)

    typedef struct CSFrameInfo {
        int64_t mFrameIndex;
        int64_t mFrameOffset;
        double mPTS;
    } CSFrameInfo;

    typedef struct CSFrameSummaryInfo {
        int mMaxInteral;
        int mMinInteral;
        int mAvgInteral;
        int mTotalInteral;
    } CSFrameSummaryInfo;

/** 结构功能解析:
 *  视频文件解析
 */
    typedef class CSVideoAnalysis : public CSVideoDecoder {
    public:
        CSVideoAnalysis();

        ~CSVideoAnalysis();

        /**
         *  对外接口，分析输入的上一帧图像数据
         */
        void analysisFrame(AVFrame *pFrame, AVMediaType type);

        /**
         *  输出汇总后视频解析信息
         */
        void ExportAnalysisInfo();

        void setAnalysisModule(uint64_t infoSet) {
            mInfoSet = infoSet;
        }

    protected:

        /**
         *  内部输出当前帧的一些信息
         */
        void _collectFrameInfo(AVFrame *pFrame, AVMediaType type);

    private:
        uint64_t mInfoSet;
        
        /** Video info  */
        int64_t mVideoPictureTypeArry[MAX_PICTURE_TYPE];
        int64_t mVideoFrameCount;
        int64_t mAudioFrameCount;
        
        std::vector<CSFrameInfo> mFrameInfoSet[MAX_PICTURE_TYPE];
        CSFrameSummaryInfo       mFrameSummaryInfoSet[MAX_PICTURE_TYPE];

        /** Audio info  */

    } CSVideoAnalysis;

}

#endif /* CSVideoAnalysis_h */
