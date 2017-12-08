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
    
    for (int i=0; i<MAX_PICTURE_TYPE; i++)
        initialFrameInfoSummary(&(mFrameSummaryInfoSet[i]));
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
            
            enum AVPictureType frameType = pFrame->pict_type;
            switch (frameType) {
                case AV_PICTURE_TYPE_I:
                    {
                        {
                            enum AVPictureType ePFrameType = AV_PICTURE_TYPE_P;
                            /** 计算 P 帧时间间隔 */
                            if (mFrameInfoSet[ePFrameType].size() != 0) {
                                mFrameSummaryInfoSet[ePFrameType].mTotalInteral += mFrameInfoSet[ePFrameType].size();
                                
                                if (mFrameSummaryInfoSet[ePFrameType].mMinInteral == 0)
                                    mFrameSummaryInfoSet[ePFrameType].mMinInteral = (int)mFrameInfoSet[ePFrameType].size();
                                else if (mFrameSummaryInfoSet[ePFrameType].mMinInteral > mFrameInfoSet[ePFrameType].size())
                                    mFrameSummaryInfoSet[ePFrameType].mMinInteral = (int)mFrameInfoSet[ePFrameType].size();
                                
                                if (mFrameSummaryInfoSet[ePFrameType].mMaxInteral < mFrameInfoSet[ePFrameType].size())
                                    mFrameSummaryInfoSet[ePFrameType].mMaxInteral = (int)mFrameInfoSet[ePFrameType].size();
                            }
                            mFrameInfoSet[ePFrameType].clear();
                        }
                        
                        {
                            enum AVPictureType eBFrameType = AV_PICTURE_TYPE_B;
                            /** 计算 B 帧时间间隔 */
                            if (mFrameInfoSet[eBFrameType].size() != 0) {
                                mFrameSummaryInfoSet[eBFrameType].mTotalInteral += mFrameInfoSet[eBFrameType].size();
                                
                                if (mFrameSummaryInfoSet[eBFrameType].mMinInteral == 0)
                                    mFrameSummaryInfoSet[eBFrameType].mMinInteral = (int)mFrameInfoSet[eBFrameType].size();
                                else if (mFrameSummaryInfoSet[eBFrameType].mMinInteral > mFrameInfoSet[eBFrameType].size())
                                    mFrameSummaryInfoSet[eBFrameType].mMinInteral = (int)mFrameInfoSet[eBFrameType].size();
                                
                                if (mFrameSummaryInfoSet[eBFrameType].mMaxInteral < mFrameInfoSet[eBFrameType].size())
                                    mFrameSummaryInfoSet[eBFrameType].mMaxInteral = (int)mFrameInfoSet[eBFrameType].size();
                            }
                            mFrameInfoSet[eBFrameType].clear();
                        }
                        
                        {
                            /** 计算 I 帧时间间隔 */
                            if (mFrameInfoSet[frameType].size() != 0) {
                                size_t iLastFrameInfoIndex = (mFrameInfoSet[frameType].size() - 1);
                                double currentFrameInteral = newFrameInfoObj.mFrameOffset - mFrameInfoSet[frameType][iLastFrameInfoIndex].mFrameOffset;
                                
                                
                                mFrameSummaryInfoSet[frameType].mTotalInteral += currentFrameInteral;
                                if (currentFrameInteral > mFrameSummaryInfoSet[frameType].mMaxInteral)
                                    mFrameSummaryInfoSet[frameType].mMaxInteral = currentFrameInteral;
                                
                                if (mFrameSummaryInfoSet[frameType].mMinInteral == 0)
                                    mFrameSummaryInfoSet[frameType].mMinInteral = currentFrameInteral;
                                else if (currentFrameInteral < mFrameSummaryInfoSet[frameType].mMinInteral)
                                    mFrameSummaryInfoSet[frameType].mMinInteral = currentFrameInteral;
                            }
                            mFrameInfoSet[frameType].push_back(newFrameInfoObj);
                        }
                    }
                    break;
                case AV_PICTURE_TYPE_P:
                    {
                        mFrameInfoSet[frameType].push_back(newFrameInfoObj);
                    }
                    break;
                case AV_PICTURE_TYPE_B:
                    {
                        mFrameInfoSet[frameType].push_back(newFrameInfoObj);
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
    AVStream* pWorkStream = mPInVideoFormatCtx->streams[mVideoStreamIndex];
    
    
    LOGI("\r\n");
    LOGI(">>> SUMMARY: #################################### <<<");
    
    LOGI(">>> [Video Stream Info] =============================");
    LOGI("Video: TimeBase:<%d,%d> (%lf) ", pWorkStream->time_base.num, pWorkStream->time_base.den, av_q2d(pWorkStream->time_base));
    LOGI("======================================= <<< ");
    
    LOGI(">>> [Frames Info] ===================================");
    if (mInfoSet & INFO_FRAME_TYPE) {
        LOGI("Frame Type: I | P | B | S | SI | SP | BI | NONE >>> ");
        LOGI("  I-frame:    <no:%lld> Frame-Interal: <Max:%d> <Min:%d> <Avg:%d>", \
             mVideoPictureTypeArry[AV_PICTURE_TYPE_I], mFrameSummaryInfoSet[AV_PICTURE_TYPE_I].mMaxInteral, mFrameSummaryInfoSet[AV_PICTURE_TYPE_I].mMinInteral, \
                    (int)(mFrameSummaryInfoSet[AV_PICTURE_TYPE_I].mTotalInteral / (int)(mFrameInfoSet[AV_PICTURE_TYPE_I].size() - 1)));
        
        LOGI("  P-frame:    <no:%lld> Per_I_Frame: <Max:%d> <Min:%d> <Avg:%d>", \
             mVideoPictureTypeArry[AV_PICTURE_TYPE_P], mFrameSummaryInfoSet[AV_PICTURE_TYPE_P].mMaxInteral, mFrameSummaryInfoSet[AV_PICTURE_TYPE_P].mMinInteral, \
                    (int)(mFrameSummaryInfoSet[AV_PICTURE_TYPE_P].mTotalInteral / (int)(mFrameInfoSet[AV_PICTURE_TYPE_I].size() - 1)));
        
        LOGI("  B-frame:    <no:%lld> Per_I_Frame: <Max:%d> <Min:%d> <Avg:%d>", \
             mVideoPictureTypeArry[AV_PICTURE_TYPE_B], mFrameSummaryInfoSet[AV_PICTURE_TYPE_B].mMaxInteral, mFrameSummaryInfoSet[AV_PICTURE_TYPE_B].mMinInteral, \
                    (int)(mFrameSummaryInfoSet[AV_PICTURE_TYPE_B].mTotalInteral / (int)(mFrameInfoSet[AV_PICTURE_TYPE_I].size() - 1)));
        LOGI("  S-frame:    <no:%lld>", mVideoPictureTypeArry[AV_PICTURE_TYPE_S]);
        LOGI("  SI-frame:   <no:%lld>", mVideoPictureTypeArry[AV_PICTURE_TYPE_SI]);
        LOGI("  SP-frame:   <no:%lld>", mVideoPictureTypeArry[AV_PICTURE_TYPE_SP]);
        LOGI("  BI-frame:   <no:%lld>", mVideoPictureTypeArry[AV_PICTURE_TYPE_BI]);
        LOGI("  NONE-frame: <no:%lld>", mVideoPictureTypeArry[AV_PICTURE_TYPE_NONE]);
        LOGI("======================================= <<< ");
    }
    
    
    LOGI(">>> ========================================");
}

}
