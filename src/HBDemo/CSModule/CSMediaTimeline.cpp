//
//  CSTimeline.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/10.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSMediaTimeline.h"
#include "CSStreamFactory.h"
#include "CSIStream.h"
#include "CSVStream.h"
#include "CSAStream.h"
#include "CSWorkContext.h"

namespace HBMedia {

CSTimeline::CSTimeline(){
    mGlobalSpeed = 1.0f;
    mFmtCtx = nullptr;
    mSaveFilePath = nullptr;
    audioParamInit(&mSrcAudioParams);
    audioParamInit(&mTgtAudioParams);
    imageParamInit(&mSrcImageParams);
    imageParamInit(&mTgtImageParams);
    cropParamInit(&mCropParms);
}

CSTimeline::~CSTimeline(){
    if (mFmtCtx) {
        avformat_close_input(&mFmtCtx);
        mFmtCtx = nullptr;
    }
}

int CSTimeline::prepare() {
    if (initial() != HB_OK) {
        LOGE("Timeline prepare call initial failed !");
        return HB_ERROR;
    }
    
    if (_prepareOutMedia() != HB_OK) {
        LOGE("Timeline prepare output media initial failed !");
        return HB_ERROR;
    }
    
    /** 创建线程环境 */
    mWorkCtx = new CSWorkContext();
    if (!mWorkCtx) {
        LOGE("Timeline create work context failed !");
        return HB_ERROR;
    }
    
    mWorkCtx->prepare(mFmtCtx);
    
    return HB_OK;
}

int CSTimeline::start(void) {
    CSIStream *pBaseStream = nullptr;
    for (std::vector<CSIStream *>::iterator pNode = mStreamsList.begin(); pNode != mStreamsList.end(); pNode++) {
        pBaseStream = (CSIStream *)(*pNode);
        if (HB_OK != mWorkCtx->pushStream(pBaseStream)) {
            LOGE("Timeline push stream to work context failed !");
            return HB_ERROR;
        }
    }
    
    if (HB_OK != writeHeader()) {
        LOGE("Timeline write header failed !");
        return HB_ERROR;
    }
    
    if (HB_OK != mWorkCtx->start()) {
        LOGE("Timeline start work context failed !");
        return HB_ERROR;
    }
    
    return HB_OK;
}

int CSTimeline::stop(void) {
    CSIStream* pBaseStream = nullptr;
    for (std::vector<CSIStream *>::iterator pNode = mStreamsList.begin(); pNode != mStreamsList.end(); pNode++) {
        pBaseStream = (CSIStream *)(*pNode);
        pBaseStream->stop();
    }
    
    writeTailer();
    return HB_OK;
}

int  CSTimeline::release(void) {
    CSIStream* pBaseStream = nullptr;
    for (std::vector<CSIStream *>::iterator pNode = mStreamsList.begin(); pNode != mStreamsList.end(); pNode++) {
        pBaseStream = (CSIStream *)(*pNode);
        pBaseStream->release();
        SAFE_DELETE(pBaseStream);
    }
    mStreamsList.clear();
    std::vector<CSIStream*>().swap(mStreamsList);
    return HB_OK;
}
    
int CSTimeline::_openOutputFile(const char *filename) {
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

int CSTimeline::initial() {
    av_register_all();
    av_log_set_flags(AV_LOG_DEBUG);
    
    return HB_OK;
}

int CSTimeline::sendRawData(uint8_t* pData, long DataSize, int StreamIdex, int64_t TimeStamp) {
    if (!pData || DataSize<=0 || StreamIdex < mStreamsList.size() || TimeStamp <0) {
        LOGE("Timeline send raw data failed !<Data:%p><Size:%ld><Stream:%d><:TimeStamp%lld>", pData, DataSize, StreamIdex, TimeStamp);
        return HB_ERROR;
    }
    
    CSIStream* pStream = mStreamsList[StreamIdex];
    if (!pStream || pStream->sendRawData(pData, DataSize, TimeStamp) != HB_OK) {
        LOGE("Timeline send raw data to stream failed !");
        return HB_ERROR;
    }
    return HB_OK;
}
    
int CSTimeline::_addStream(CSIStream* pNewStream) {
    if (!pNewStream || !mFmtCtx) {
        LOGE("Timeline add stream failed, args is invalid ! <Fmt:%p> <Stream:%p>", mFmtCtx, pNewStream);
        return HB_ERROR;
    }
    
    if (pNewStream->bindOpaque(mFmtCtx) != HB_OK) {
        LOGE("Timeline bind file handle failed !");
        return HB_ERROR;
    }
    
    mStreamsList.push_back(pNewStream);
    pNewStream->EchoStreamInfo();
    return HB_OK;
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

void CSTimeline::setCropParam(int posX, int posY, int cropWidth, int cropHeight) {
    mCropParms.posX = posX;
    mCropParms.posY = posY;
    cropWidth = (cropWidth + 1) >> 1;
    cropWidth = cropWidth << 1;
    cropHeight = (cropHeight + 1) >> 1;
    cropHeight = cropHeight << 1;
    mCropParms.cropWidth = cropWidth;
    mCropParms.cropHeight = cropHeight;
}

void CSTimeline::setGlobalSpeed(float speed){
    mGlobalSpeed = speed;
}

int CSTimeline::_prepareOutMedia() {
    if (HB_OK != _openOutputFile(mSaveFilePath)) {
        LOGE("Timeline open file failed !");
        return HB_ERROR;
    }
    
    /** TODO:此处可以改成，根据外部配置，设置添加媒体流的方式实现 */
    CSIStream* pBaseStream = CSStreamFactory::CreateMediaStream(CS_STREAM_TYPE_VIDEO);
    if (pBaseStream) {
        CSVStream* pVideoStream = (CSVStream *)pBaseStream;
        pVideoStream->setVideoParam(&mTgtImageParams);
        if (HB_OK != pVideoStream->setEncoder("libx264")) {
            LOGE("Timeline set video stream encoder failed !");
            return HB_ERROR;
        }
        if (HB_OK != _addStream(pVideoStream)) {
            LOGE("Timeline add video stream failed !");
            return HB_ERROR;
        }
    }
    
    pBaseStream = CSStreamFactory::CreateMediaStream(CS_STREAM_TYPE_AUDIO);
    if (pBaseStream) {
        CSAStream* pAudioStream = (CSAStream*)pBaseStream;
        pAudioStream->setAudioParam(&mTgtAudioParams);
        if (HB_OK != pAudioStream->setEncoder("libfdk_aac")) {
            LOGE("Timeline set audio stream encoder failed !");
            return HB_ERROR;
        }
        if (HB_OK != _addStream(pAudioStream)) {
            LOGE("Timeline add audio stream failed !");
            return HB_ERROR;
        }
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
