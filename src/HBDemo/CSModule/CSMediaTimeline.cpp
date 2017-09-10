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
        avformat_close_input(&mFmtCtx);
        mFmtCtx = nullptr;
    }
}

int CSTimeline::open(const char *filename) {
    int HBErr = HB_OK;
    HBErr = avformat_alloc_output_context2(&mFmtCtx, NULL, NULL, filename);
    if (HBErr < 0) {
        LOGE("Timeline alloc ouput avformat context failed ! <%s", makeErrorStr(HBErr));
        return HB_ERROR;
    }
    HBErr = avio_open(&mFmtCtx->pb, mSaveFilePath, AVIO_FLAG_WRITE);
    if (HBErr < 0) {
        LOGE("Timeline avio open failed !<%s>", makeErrorStr(HBErr));
        HBErr = HB_ERROR;
        goto TIMELINE_OPEN_END_LABEL;
    }
    
    strncpy(mFmtCtx->filename, mSaveFilePath, strlen(mSaveFilePath));
    return HB_OK;
TIMELINE_OPEN_END_LABEL:
    if (mFmtCtx) {
        avformat_close_input(&mFmtCtx);
        mFmtCtx = nullptr;
    }
    return HBErr;
}

int CSTimeline::writeHeader() {
    int HBErr = avformat_write_header(mFmtCtx, NULL);
    if (HBErr != 0) {
        LOGE("Timeline write header failed!<%s>", makeErrorStr(HBErr));
        return HB_ERROR;
    }
    return HB_OK;
}

int CSTimeline::writeTailer() {
    int HBErr = av_write_trailer(mFmtCtx);
    if (HBErr != 0) {
        LOGE("Timeline write trailer failed !<%s>", makeErrorStr(HBErr));
        return HB_ERROR;
    }
    return HB_OK;
}

void CSTimeline::setOutputFile(char *file) {
    if (mSaveFilePath)
        av_freep(&mSaveFilePath);
    mSaveFilePath = av_strdup(file);
    LOGI("Timeline set output file:%s", mSaveFilePath);
}

}
