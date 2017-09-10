//
//  CSTimeline.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/10.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSMediaTimeline.h"
#include "CSIStream.h"

namespace HBMedia {

CSTimeline::CSTimeline(){
    mFmtCtx = nullptr;
    mSaveFilePath = nullptr;
}

CSTimeline::~CSTimeline(){
    if (mFmtCtx) {
        avformat_free_context(mFmtCtx);
        mFmtCtx = nullptr;
    }
}

int CSTimeline::open(const char *filename) {
    int HBErr = HB_OK;
    HBErr = avformat_alloc_output_context2(&mFmtCtx, NULL, NULL, filename);
    
    return HBErr;
}

void CSTimeline::setOutputFile(char *file) {
    if (mSaveFilePath)
        av_freep(&mSaveFilePath);
    mSaveFilePath = av_strdup(file);
    LOGI("Timeline set output file:%s", mSaveFilePath);
}

}
