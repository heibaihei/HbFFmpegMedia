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
    
}

CSVideoAnalysis::~CSVideoAnalysis() {

}

void CSVideoAnalysis::analysisFrame(AVFrame *pFrame, AVMediaType type) {
    EchoFrameInfo(pFrame, type);
    
}

void CSVideoAnalysis::EchoFrameInfo(AVFrame *pFrame, AVMediaType type) {
    if (type == AVMEDIA_TYPE_VIDEO) {
        IMAGE_PIX_FORMAT eLocalVideoPixFormat = getImageExternFormat((AVPixelFormat)pFrame->format);
        LOGI("[Echo Frame:%s]: Size<%d,%d> Format<%s> AspectRatio<%d:%d>", \
             getPictureTypeDescript(pFrame->pict_type), pFrame->width, pFrame->height, \
             getImagePixFmtDescript(eLocalVideoPixFormat), \
             pFrame->sample_aspect_ratio.num, pFrame->sample_aspect_ratio.den);
        
        LOGI("         ===> PTS<%lld> best_effort_timestamp<%lld> DTS<%lld> codecd_no<%d> display_no<%d>", \
             pFrame->pts, pFrame->best_effort_timestamp, pFrame->pkt_dts, pFrame->coded_picture_number, pFrame->display_picture_number);
    }
}

}
