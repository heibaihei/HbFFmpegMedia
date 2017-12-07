//
//  CSVideoAnalysis.cpp
//  Sample
//
//  Created by zj-db0519 on 2017/12/6.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSVideoAnalysis.h"

namespace HBMedia {

CSVideoAnalysis::CSVideoAnalysis()
: CSVideoDecoder()
{
    memset(&mInfoSet, 0xFF, sizeof(mInfoSet));
    memset(&mVideoPictureTypeArry, 0x00, sizeof(int64_t));
}

CSVideoAnalysis::~CSVideoAnalysis() {

}

void CSVideoAnalysis::analysisFrame(AVFrame *pFrame, AVMediaType type) {
    
    EchoFrameInfo(pFrame, type);

    if (mInfoSet & INFO_FRAME_TYPE) {
        /** 统计不同帧类型的数据情况 */
        mVideoPictureTypeArry[pFrame->pict_type]++;
    }
}

void CSVideoAnalysis::EchoFrameInfo(AVFrame *pFrame, AVMediaType type) {
    if (type == AVMEDIA_TYPE_VIDEO) {
        IMAGE_PIX_FORMAT eLocalVideoPixFormat = getImageExternFormat((AVPixelFormat)pFrame->format);
        LOGI("[Echo Frame: <%s>]: Size<%d,%d> Format<%s> AspectRatio<%d:%d>", \
             getPictureTypeDescript(pFrame->pict_type), pFrame->width, pFrame->height, \
             getImagePixFmtDescript(eLocalVideoPixFormat), \
             pFrame->sample_aspect_ratio.num, pFrame->sample_aspect_ratio.den);
        
        
        int64_t tiExternPts = av_rescale_q(pFrame->pts, mPInVideoFormatCtx->streams[mVideoStreamIndex]->time_base, \
                                        AV_TIME_BASE_Q);
        double  tfExternPts = ((double)tiExternPts / AV_TIME_BASE);
        
        LOGI("         (%lf) ===> PTS<%lld> BestPTS<%lld> DTS<%lld> codecd_no<%d> display_no<%d>", \
             tfExternPts,  pFrame->pts, pFrame->best_effort_timestamp, pFrame->pkt_dts, \
             pFrame->coded_picture_number, pFrame->display_picture_number);
    }
}
    
void CSVideoAnalysis::ExportAnalysisInfo() {
    LOGI(">>> ========================================");
    if (mInfoSet & INFO_FRAME_TYPE) {
        LOGI("Frame Type: I | P | B | S | SI | SP | BI | NONE >>> ");
        LOGI("  I-frame: <no:%lld>", mVideoPictureTypeArry[AV_PICTURE_TYPE_I]);
        LOGI("  P-frame: <no:%lld>", mVideoPictureTypeArry[AV_PICTURE_TYPE_P]);
        LOGI("  B-frame: <no:%lld>", mVideoPictureTypeArry[AV_PICTURE_TYPE_B]);
        LOGI("  S-frame: <no:%lld>", mVideoPictureTypeArry[AV_PICTURE_TYPE_S]);
        LOGI("  SI-frame: <no:%lld>", mVideoPictureTypeArry[AV_PICTURE_TYPE_SI]);
        LOGI("  SP-frame: <no:%lld>", mVideoPictureTypeArry[AV_PICTURE_TYPE_SP]);
        LOGI("  BI-frame: <no:%lld>", mVideoPictureTypeArry[AV_PICTURE_TYPE_BI]);
        LOGI("  NONE-frame: <no:%lld>", mVideoPictureTypeArry[AV_PICTURE_TYPE_NONE]);
        LOGI("=============================================== <<< ");
    }
    
    
    LOGI(">>> ========================================");
}

}
