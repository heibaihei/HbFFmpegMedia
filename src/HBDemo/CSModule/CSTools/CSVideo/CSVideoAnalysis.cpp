//
//  CSVideoAnalysis.cpp
//  Sample
//
//  Created by zj-db0519 on 2017/12/6.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSVideoAnalysis.h"

namespace HBMedia {

static void initialFrameInfoSummary(CSFrameSummaryInfo* summary) {
    summary->mAvgInteral = 0;
    summary->mTotalInteral = 0;
    summary->mMinInteral = 0;
    summary->mMaxInteral = 0;
}

CSVideoAnalysis::CSVideoAnalysis()
: CSVideoDecoder()
{
    mVideoFrameCount = 0;
    mAudioFrameCount = 0;
    memset(&mInfoSet, 0xFF, sizeof(mInfoSet));
    memset(&mVideoPictureTypeArry, 0x00, sizeof(int64_t));
    
    initialFrameInfoSummary(&mIFrameSummary);
    initialFrameInfoSummary(&mPFrameSummary);
}

CSVideoAnalysis::~CSVideoAnalysis() {

}

void CSVideoAnalysis::analysisFrame(AVFrame *pFrame, AVMediaType type) {
    _collectFrameInfo(pFrame, type);
}

void CSVideoAnalysis::_collectFrameInfo(AVFrame *pFrame, AVMediaType type) {
    if (type == AVMEDIA_TYPE_VIDEO)
    {
        IMAGE_PIX_FORMAT eLocalVideoPixFormat = getImageExternFormat((AVPixelFormat)pFrame->format);
        int64_t tiExternPts = av_rescale_q(pFrame->pts, \
                                    mPInVideoFormatCtx->streams[mVideoStreamIndex]->time_base, AV_TIME_BASE_Q);
        double  tfExternPts = ((double)tiExternPts / AV_TIME_BASE);
        
        if (mInfoSet & INFO_FRAME_TYPE) {
            /** 统计不同帧类型的数据情况 */
            mVideoFrameCount++;
            mVideoPictureTypeArry[pFrame->pict_type]++;
            
            CSFrameInfo newFrameInfoObj;
            newFrameInfoObj.mFrameIndex = mVideoPictureTypeArry[pFrame->pict_type];
            newFrameInfoObj.mPTS = tfExternPts;
            newFrameInfoObj.mFrameOffset = mVideoFrameCount;
            
            switch (pFrame->pict_type) {
                case AV_PICTURE_TYPE_I:
                    {
                        {
                            /** 计算 P 帧时间间隔 */
                            if (mPFrameSet.size() != 0) {
                                mPFrameSummary.mTotalInteral += mPFrameSet.size();
                                
                                if (mPFrameSummary.mMinInteral == 0)
                                    mPFrameSummary.mMinInteral = (int)mPFrameSet.size();
                                else if (mPFrameSummary.mMinInteral > mPFrameSet.size())
                                    mPFrameSummary.mMinInteral = (int)mPFrameSet.size();
                                
                                if (mPFrameSummary.mMaxInteral < mPFrameSet.size())
                                    mPFrameSummary.mMaxInteral = (int)mPFrameSet.size();
                            }
                            mPFrameSet.clear();
                        }
                        
                        {
                            /** 计算 B 帧时间间隔 */
                            if (mBFrameSet.size() != 0) {
                                mBFrameSummary.mTotalInteral += mBFrameSet.size();
                                
                                if (mBFrameSummary.mMinInteral == 0)
                                    mBFrameSummary.mMinInteral = (int)mBFrameSet.size();
                                else if (mBFrameSummary.mMinInteral > mBFrameSet.size())
                                    mBFrameSummary.mMinInteral = (int)mBFrameSet.size();
                                
                                if (mBFrameSummary.mMaxInteral < mBFrameSet.size())
                                    mBFrameSummary.mMaxInteral = (int)mBFrameSet.size();
                            }
                            mBFrameSet.clear();
                        }
                        
                        /** 计算 I 帧时间间隔 */
                        if (mIFrameSet.size() != 0) {
                            size_t iLastFrameInfoIndex = (mIFrameSet.size() - 1);
                            double currentFrameInteral = newFrameInfoObj.mFrameOffset - mIFrameSet[iLastFrameInfoIndex].mFrameOffset;
                            
                            mIFrameSummary.mTotalInteral += currentFrameInteral;
                            if (currentFrameInteral > mIFrameSummary.mMaxInteral)
                                mIFrameSummary.mMaxInteral = currentFrameInteral;
                            
                            if (mIFrameSummary.mMinInteral == 0)
                                mIFrameSummary.mMinInteral = currentFrameInteral;
                            else if (currentFrameInteral < mIFrameSummary.mMinInteral)
                                mIFrameSummary.mMinInteral = currentFrameInteral;
                        }
                        mIFrameSet.push_back(newFrameInfoObj);
                    }
                    break;
                case AV_PICTURE_TYPE_P:
                    {
                        mPFrameSet.push_back(newFrameInfoObj);
                    }
                    break;
                case AV_PICTURE_TYPE_B:
                    {
                        mBFrameSet.push_back(newFrameInfoObj);
                    }
                    break;
                default:
                    break;
            }
        }
        
        LOGI("[Echo Frame: <%s>]: Size<%d,%d> Format<%s> AspectRatio<%d:%d>", \
             getPictureTypeDescript(pFrame->pict_type), pFrame->width, pFrame->height, \
             getImagePixFmtDescript(eLocalVideoPixFormat), \
             pFrame->sample_aspect_ratio.num, pFrame->sample_aspect_ratio.den);
        
        LOGI("         (%lf) ===> PTS<%lld> BestPTS<%lld> DTS<%lld> codecd_no<%d> display_no<%d>", \
             tfExternPts,  pFrame->pts, pFrame->best_effort_timestamp, pFrame->pkt_dts, \
             pFrame->coded_picture_number, pFrame->display_picture_number);
    }
}
    
void CSVideoAnalysis::ExportAnalysisInfo() {
    LOGI(">>> ========================================");
    if (mInfoSet & INFO_FRAME_TYPE) {
        LOGI("Frame Type: I | P | B | S | SI | SP | BI | NONE >>> ");
        LOGI("  I-frame:    <no:%lld> Frame-Interal: <Max:%d> <Min:%d> <Avg:%d>", \
             mVideoPictureTypeArry[AV_PICTURE_TYPE_I], mIFrameSummary.mMaxInteral, mIFrameSummary.mMinInteral, (int)(mIFrameSummary.mTotalInteral / (int)(mIFrameSet.size() - 1)));
        
        LOGI("  P-frame:    <no:%lld> Per_I_Frame: <Max:%d> <Min:%d> <Avg:%d>", \
             mVideoPictureTypeArry[AV_PICTURE_TYPE_P], mPFrameSummary.mMaxInteral, mPFrameSummary.mMinInteral, (int)(mPFrameSummary.mTotalInteral / (int)(mIFrameSet.size() - 1)));
        
        LOGI("  B-frame:    <no:%lld> Per_I_Frame: <Max:%d> <Min:%d> <Avg:%d>", \
             mVideoPictureTypeArry[AV_PICTURE_TYPE_B], mBFrameSummary.mMaxInteral, mBFrameSummary.mMinInteral, (int)(mBFrameSummary.mTotalInteral / (int)(mIFrameSet.size() - 1)));
        LOGI("  S-frame:    <no:%lld>", mVideoPictureTypeArry[AV_PICTURE_TYPE_S]);
        LOGI("  SI-frame:   <no:%lld>", mVideoPictureTypeArry[AV_PICTURE_TYPE_SI]);
        LOGI("  SP-frame:   <no:%lld>", mVideoPictureTypeArry[AV_PICTURE_TYPE_SP]);
        LOGI("  BI-frame:   <no:%lld>", mVideoPictureTypeArry[AV_PICTURE_TYPE_BI]);
        LOGI("  NONE-frame: <no:%lld>", mVideoPictureTypeArry[AV_PICTURE_TYPE_NONE]);
        LOGI("=============================================== <<< ");
    }
    
    
    LOGI(">>> ========================================");
}

}
