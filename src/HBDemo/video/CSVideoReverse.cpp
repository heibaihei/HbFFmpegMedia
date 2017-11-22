//
//  CSVideoReverse.cpp
//  Sample
//
//  Created by zj-db0519 on 2017/11/21.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSVideoReverse.h"

CSVideoFilter::CSVideoFilter() {
    mVideoInputFile = nullptr;
    mVideoOutputFile = nullptr;
    mInFmtCtx = nullptr;
    mInVideoStream = nullptr;
    mInVideoCodecCtx = nullptr;
    mStartTime = 0.0;
    mEndTime = 0.0;
    mEndVideoPts = 0.0;
}

CSVideoFilter::~CSVideoFilter() {
    if (mVideoInputFile)
        av_freep(&mVideoInputFile);
    if (mVideoOutputFile)
        av_freep(&mVideoOutputFile);
}

int CSVideoFilter::prepare() {
    if (_prepareInMedia() != HB_OK) {
        LOGE("Prepare input media faidled !");
        return HB_ERROR;
    }
    
    if (_collectKeyFramePts() != HB_OK) {
        LOGE("Collect input media faidled !");
        return HB_ERROR;
    }
    
    if (_doReverse() != HB_OK) {
        LOGE("do video reverse failed !");
        return HB_ERROR;
    }
    
    return HB_OK;
}

int CSVideoFilter::_prepareInMedia() {
    AVCodec *pCodec = nullptr;
    int HBError = avformat_open_input(&mInFmtCtx, mVideoInputFile, NULL, NULL);
    if (HBError != 0) {
        LOGE("Video decoder couldn't open input file. <%d> <%s>", HBError, av_err2str(HBError));
        return HB_ERROR;
    }
    
    HBError = avformat_find_stream_info(mInFmtCtx, NULL);
    if (HBError < 0) {
        LOGE("Video decoder couldn't find stream information. <%s>", av_err2str(HBError));
        return HB_ERROR;
    }
    
    mInVideoStream = nullptr;
    for (int i = 0; i<mInFmtCtx->nb_streams; i++) {
        if (mInFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            mInVideoStream = mInFmtCtx->streams[i];
            break;
        }
    }
    if (!mInVideoStream) {
        LOGE("Cant't find valid video stream !");
        return HB_ERROR;
    }
    
    pCodec = avcodec_find_decoder(mInVideoStream->codecpar->codec_id);
    mInVideoCodecCtx = avcodec_alloc_context3(pCodec);
    if (!mInVideoCodecCtx) {
        LOGE("Codec ctx <%d> not found !", mInVideoStream->codecpar->codec_id);
        return HB_ERROR;
    }
    avcodec_parameters_to_context(mInVideoCodecCtx, mInVideoStream->codecpar);
    
    HBError = avcodec_open2(mInVideoCodecCtx, pCodec, nullptr);
    if (HBError < 0) {
        LOGE("Could not open codec. <%s>", av_err2str(HBError));
        return HB_ERROR;
    }
    
    return HB_OK;
}

int CSVideoFilter::insertFrameQueue(std::vector<FrameInfo *> &frameQueue, AVFrame *pFrame, int mediaStreamIndex, enum AVMediaType mediaType) {
    
    FrameInfo *pNewFrameInfo = new FrameInfo;
    if (!pNewFrameInfo) {
        LOGE("Create new frame info failed !");
        return HB_ERROR;
    }
    
    pNewFrameInfo->pMediaFrame = av_frame_clone(pFrame);
    pNewFrameInfo->MediaType = mediaType;
    pNewFrameInfo->StreamIndex = mediaStreamIndex;
    frameQueue.insert(frameQueue.begin(), pNewFrameInfo);
    
    return HB_OK;
}
int CSVideoFilter::_partReverse(int streamIndex, int64_t StartTime, int64_t EndTime) {
    int HBError = HB_OK;
    AVPacket pNewPacket;
    AVFrame *pNewFrame = nullptr;
    AVStream *pCurStream = nullptr;
    AVRational externDefaultTimebase = {1, AV_TIME_BASE};
    int64_t reverseStartPts = av_rescale_q((int64_t)mStartTime * AV_TIME_BASE, externDefaultTimebase, mInVideoStream->time_base);
    int64_t reverseEndPts = av_rescale_q((int64_t)mEndTime * AV_TIME_BASE, externDefaultTimebase, mInVideoStream->time_base);
    std::vector<FrameInfo *> frameQueue;
    std::vector<FrameInfo *>::iterator pFrameQueueIterator;
    if (reverseEndPts != 0 && StartTime > reverseEndPts) {
        HBError = HB_ERROR;
        goto PART_REVERSE_END_LABEL;
    }
    
    HBError = av_seek_frame(mInFmtCtx, streamIndex, StartTime, AVSEEK_FLAG_BACKWARD);
    if (HBError >=0 ) {
        LOGE("seek to start pos failed, %s", av_err2str(HBError));
        HBError = HB_ERROR;
        goto PART_REVERSE_END_LABEL;
    }
    
    pNewFrame = av_frame_alloc();
    if (!pNewFrame) {
        LOGE("Alloc new frame room failed !");
        HBError = HB_ERROR;
        goto PART_REVERSE_END_LABEL;
    }
    
    av_init_packet(&pNewPacket);
    while (true) {
        HBError = av_read_frame(mInFmtCtx, &pNewPacket);
        if (HBError != 0) {
            LOGE("Read new frame from input media failed, %s", av_err2str(HBError));
            HBError = HB_ERROR;
            break;
        }
        
        pCurStream = mInFmtCtx->streams[pNewPacket.stream_index];
        if (pCurStream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO \
            && pCurStream->codecpar->codec_type != AVMEDIA_TYPE_AUDIO) {
            av_packet_unref(&pNewPacket);
            continue;
        }
        
        HBError = avcodec_send_packet(mInVideoCodecCtx, &pNewPacket);
        if (HBError != 0) {
            if (HBError == AVERROR(EAGAIN))
                LOGE("Not enought packet into codec");
            break;
        }
        pNewFrame = av_frame_alloc();
        HBError = avcodec_receive_frame(mInVideoCodecCtx, pNewFrame);
        if (HBError == 0) {
            pNewFrame->pts = av_frame_get_best_effort_timestamp(pNewFrame);
            if (pNewFrame->pts >= reverseStartPts && \
                pNewFrame->pts <= reverseEndPts) {
                if (mEndVideoPts < pNewFrame->pts)
                    mEndVideoPts = pNewFrame->pts;
                /** 将帧数据插入到帧队列中去 */
                insertFrameQueue(frameQueue, \
                        pNewFrame, streamIndex, pCurStream->codecpar->codec_type);
            }
            av_frame_unref(pNewFrame);
        }
        av_packet_unref(&pNewPacket);
    }
    
    for (pFrameQueueIterator = frameQueue.begin(); \
         pFrameQueueIterator != frameQueue.end(); pFrameQueueIterator++) {
//        enum AVMediaType = (*pFrameQueueIterator)->MediaType;
        
        /** 利用 filter 进行取帧 */
        
        
    }
    
    
PART_REVERSE_END_LABEL:
    
    return HBError;
}

int CSVideoFilter::_doReverse() {
    if (mKeyFramePtsVector.begin() == mKeyFramePtsVector.end()) {
        LOGE("get invalid key frame pts info !");
        return HB_ERROR;
    }
    
    std::vector <KeyFramePts *>::iterator KeyFrameIterator = mKeyFramePtsVector.begin();
    for (; KeyFrameIterator != mKeyFramePtsVector.end(); KeyFrameIterator++) {
        KeyFramePts* pKeyFramePtsNode = *KeyFrameIterator;
        _partReverse(mInVideoStream->index, pKeyFramePtsNode->videoPts, 0);
    }
    
    return HB_OK;
}

int CSVideoFilter::_collectKeyFramePts() {
    int HBError = HB_OK;
    int64_t curAudioPts = 0;
    KeyFramePts *pKeyFramePTS;
    AVRational externDefaultTimebase = {1, AV_TIME_BASE};
    int64_t startTime = av_rescale_q((int64_t)mStartTime * AV_TIME_BASE, \
                                externDefaultTimebase, mInVideoStream->time_base);
    int64_t endTime = av_rescale_q((int64_t)mEndTime * AV_TIME_BASE, \
                                externDefaultTimebase, mInVideoStream->time_base);
    
    av_seek_frame(mInFmtCtx, mInVideoStream->index, startTime, AVSEEK_FLAG_BACKWARD);

    AVPacket tmpPacket;
    while (true) {
        av_init_packet(&tmpPacket);
        HBError = av_read_frame(mInFmtCtx, &tmpPacket);
        if (HBError != 0) {
            LOGE("read frame failed, %s", av_err2str(HBError));
            break;
        }
        
        if (mInFmtCtx->streams[tmpPacket.stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (tmpPacket.flags & AV_PKT_FLAG_KEY) {
                pKeyFramePTS = new KeyFramePts;
                if (!pKeyFramePTS) {
                    HBError = HB_ERROR;
                    goto COLLECT_KEY_FRAME_PTS_END_LABEL;
                }
                
                pKeyFramePTS->audioPts = curAudioPts;
                pKeyFramePTS->videoPts = tmpPacket.pts;
                
                /** 后解析的帧往前插 */
                mKeyFramePtsVector.insert(mKeyFramePtsVector.begin(), pKeyFramePTS);
                if (0 != startTime && tmpPacket.pts > endTime) {
                    HBError = HB_ERROR;
                    goto COLLECT_KEY_FRAME_PTS_END_LABEL;
                }
            }
        }
        else if (mInFmtCtx->streams[tmpPacket.stream_index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            curAudioPts  =tmpPacket.pts;
        }
        av_packet_unref(&tmpPacket);
    }
    
COLLECT_KEY_FRAME_PTS_END_LABEL:
    av_packet_unref(&tmpPacket);
    
    return HBError;
}

void CSVideoFilter::setInputVideoMediaFile(char *file) {
    if (mVideoInputFile)
        av_freep(&mVideoInputFile);
    mVideoInputFile = av_strdup(file);
}
char *CSVideoFilter::getInputVideoMediaFile() {
    return mVideoInputFile;
}
void CSVideoFilter::setOutputVideoMediaFile(char *file) {
    if (mVideoOutputFile)
        av_freep(&mVideoOutputFile);
    mVideoOutputFile = av_strdup(file);
}

char *CSVideoFilter::getOutputVideoMediaFile() {
    return mVideoOutputFile;
}
