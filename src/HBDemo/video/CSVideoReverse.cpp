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
    mInAudioStream = nullptr;
    mInVideoCodecCtx = nullptr;
    mOutFilterCtx = nullptr;
    mStartTime = 0.0;
    mEndTime = 0.0;
    mEndVideoPts = 0.0;
    mFrameCount = 0;
    mOutFmtCtx = nullptr;
    mOutVideoStream = nullptr;
    mOutAudioStream = nullptr;
    mOutVideoCodecCtx = nullptr;
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

    if (_prepareOutMedia() != HB_OK) {
        LOGE("Prepare input media faidled !");
        return HB_ERROR;
    }

    if (_filterInitial() != HB_OK) {
        LOGE("Video filter prepare intial failed !");
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

int CSVideoFilter::_filterInitial() {
    int HBErr = HB_OK;
    enum AVMediaType mediaType = AVMEDIA_TYPE_UNKNOWN;
    mOutFilterCtx = (FilterCtx *) av_malloc_array(mInFmtCtx->nb_streams, sizeof(FilterCtx));
    if (!mOutFilterCtx) {
        HBErr = HB_ERROR;
        goto FILTER_INITIAL_END_LABEL;
    }
    memset(mOutFilterCtx, 0x00, (mInFmtCtx->nb_streams * sizeof(FilterCtx)));

    for (int i=0; i< mInFmtCtx->nb_streams; i++) {
        AVStream *pStream = mInFmtCtx->streams[i];
        mediaType = pStream->codecpar->codec_type;
        if (mediaType == AVMEDIA_TYPE_AUDIO) {


        }
        else if (mediaType == AVMEDIA_TYPE_VIDEO) {

        }
    }

FILTER_INITIAL_END_LABEL:

    return HB_OK;
}

int CSVideoFilter::_videoFilterInitial()
{
    return HB_OK;
}

int CSVideoFilter::_audioFilterInitial()
{
    return HB_OK;
}

int CSVideoFilter::_prepareOutMedia() {
    int HBError = avformat_alloc_output_context2(&mOutFmtCtx, NULL, NULL, mVideoOutputFile);
    if (HBError < 0) {
        LOGE("AVformat alloc output meida context failed, %s", av_err2str(HBError));
        HBError = HB_ERROR;
        goto PREPARE_OUT_MEDIA_END_LABEL;
    }



    return HB_OK;
PREPARE_OUT_MEDIA_END_LABEL:
    avformat_close_input(&mOutFmtCtx);
    return HBError;
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
        }
        else if (mInFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            mInAudioStream = mInFmtCtx->streams[i];
        }
    }
    
    if (!mInVideoStream || !mInAudioStream) {
        LOGE("Cant't find valid video and audio stream !");
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

int CSVideoFilter::pushFrameQueue(std::vector<FrameInfo *> &frameQueue, AVFrame *pFrame, int mediaStreamIndex, enum AVMediaType mediaType) {

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
    int64_t reverseStartPts = av_rescale_q((int64_t) mStartTime * AV_TIME_BASE, externDefaultTimebase, mInVideoStream->time_base);
    int64_t reverseEndPts = av_rescale_q((int64_t) mEndTime * AV_TIME_BASE, externDefaultTimebase, mInVideoStream->time_base);
    std::vector<FrameInfo *> frameQueue;
    std::vector<FrameInfo *>::iterator pFrameQueueIterator;
    if (reverseEndPts != 0 && StartTime > reverseEndPts) {
        HBError = HB_ERROR;
        goto PART_REVERSE_END_LABEL;
    }

    HBError = av_seek_frame(mInFmtCtx, streamIndex, StartTime, AVSEEK_FLAG_BACKWARD);
    if (HBError >= 0) {
        LOGE("seek to start pos failed, %s", av_err2str(HBError));
        HBError = HB_ERROR;
        goto PART_REVERSE_END_LABEL;
    }
    avcodec_flush_buffers(mInVideoCodecCtx);

    pNewFrame = av_frame_alloc();
    if (!pNewFrame) {
        LOGE("Alloc new frame room failed !");
        HBError = HB_ERROR;
        goto PART_REVERSE_END_LABEL;
    }

    pNewFrame = av_frame_alloc();
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
            LOGE("AVcode send new packet into video codec context failed, %s", av_err2str(HBError));
            av_packet_unref(&pNewPacket);
            if (HBError == AVERROR(EAGAIN))
                continue;
            break;
        }

        HBError = avcodec_receive_frame(mInVideoCodecCtx, pNewFrame);
        if (HBError == 0) {
            pNewFrame->pts = av_frame_get_best_effort_timestamp(pNewFrame);
            if (pNewFrame->pts >= reverseStartPts && \
                pNewFrame->pts <= reverseEndPts) {
                if (mEndVideoPts < pNewFrame->pts)
                    mEndVideoPts = pNewFrame->pts;
                /** 将帧数据插入到帧队列中去 */
                pushFrameQueue(frameQueue, \
                        pNewFrame, streamIndex, pCurStream->codecpar->codec_type);
            }
            av_frame_unref(pNewFrame);
        } else {
            av_frame_unref(pNewFrame);
            if (HBError != AVERROR(EAGAIN)) {
                LOGE("Receive decoded frame failed, %s", av_err2str(HBError));
                break;
            }
        }
        av_packet_unref(&pNewPacket);
    }

    /** 刷帧 */
    HBError = avcodec_send_packet(mInVideoCodecCtx, nullptr);
    if (HBError != 0) {
        LOGE("AVcode flush video codec context failed, %s", av_err2str(HBError));
    }

    while (true) {
        HBError = avcodec_receive_frame(mInVideoCodecCtx, pNewFrame);
        if (HBError == 0) {
            pNewFrame->pts = av_frame_get_best_effort_timestamp(pNewFrame);
            if (pNewFrame->pts >= reverseStartPts && \
                pNewFrame->pts <= reverseEndPts) {
                if (mEndVideoPts < pNewFrame->pts)
                    mEndVideoPts = pNewFrame->pts;
                /** 将帧数据插入到帧队列中去 */
                pushFrameQueue(frameQueue, \
                        pNewFrame, streamIndex, pCurStream->codecpar->codec_type);
            }
            av_frame_unref(pNewFrame);
        } else {
            av_frame_unref(pNewFrame);
            break;
        }
    }

    av_frame_free(&pNewFrame);
    
    if (_exportReverseMedia(frameQueue) != HB_OK) {
        LOGE("Export reverse media failed !");
        HBError = HB_ERROR;
    }

PART_REVERSE_END_LABEL:
    return HBError;
}

int CSVideoFilter::_exportReverseMedia(std::vector<FrameInfo *>& frameQueue) {

    std::vector<FrameInfo *>::iterator frameInfoIterator = frameQueue.begin();
    AVRational externDefaultTimebase = {1, AV_TIME_BASE};
    int64_t reverseStartPts = av_rescale_q((int64_t) mStartTime * AV_TIME_BASE, externDefaultTimebase, mInVideoStream->time_base);
    int64_t reverseEndPts = av_rescale_q((int64_t) mEndTime * AV_TIME_BASE, externDefaultTimebase, mInVideoStream->time_base);
    int64_t duration = reverseEndPts;
    if (reverseEndPts > mEndVideoPts)
        duration = mEndVideoPts;

    AVFrame *pNewFrame = av_frame_alloc();
    if (!pNewFrame) {
        LOGE("Allock new empty frame room failed !");
        return HB_ERROR;
    }

    enum AVMediaType eCurFrmaeMediaType;
    int  curStreamIndex = -1;
    for (; frameInfoIterator != frameQueue.end(); frameInfoIterator++) {
        eCurFrmaeMediaType = (*frameInfoIterator)->MediaType;
        if (eCurFrmaeMediaType == AVMEDIA_TYPE_VIDEO) {
            (*frameInfoIterator)->pMediaFrame->pts = mEndVideoPts - (*frameInfoIterator)->pMediaFrame->pts;
        } else if (eCurFrmaeMediaType == AVMEDIA_TYPE_AUDIO) {

            (*frameInfoIterator)->pMediaFrame->pts = mFrameCount * mInAudioStream->time_base.den / mInAudioStream->time_base.num * av_q2d(mInAudioStream->r_frame_rate);
            mFrameCount++;
        }

        curStreamIndex = (*frameInfoIterator)->StreamIndex;


    }



    return HB_OK;
}

int CSVideoFilter::_doReverse() {
    if (mKeyFramePtsVector.begin() == mKeyFramePtsVector.end()) {
        LOGE("get invalid key frame pts info !");
        return HB_ERROR;
    }

    KeyFramePts* pLastKeyFramePtsNode = nullptr;
    std::vector <KeyFramePts *>::iterator KeyFrameIterator = mKeyFramePtsVector.begin();
    for (KeyFrameIterator = mKeyFramePtsVector.begin(); KeyFrameIterator != mKeyFramePtsVector.end(); KeyFrameIterator++) {

        KeyFramePts* pKeyFramePtsNode = *KeyFrameIterator;
        if (_partReverse(mInVideoStream->index, pKeyFramePtsNode->videoPts, (pLastKeyFramePtsNode ? pLastKeyFramePtsNode->videoPts : 0)) != HB_OK) {
            LOGE("do param reverse failed !");
            break;
        }

        pLastKeyFramePtsNode = pKeyFramePtsNode;
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

    enum AVMediaType curMediaType = AVMEDIA_TYPE_UNKNOWN;
    AVPacket tmpPacket;
    while (true) {
        av_init_packet(&tmpPacket);
        HBError = av_read_frame(mInFmtCtx, &tmpPacket);
        if (HBError != 0) {
            LOGE("Can't read frame from intput fmtctx, %s", av_err2str(HBError));
            break;
        }

        curMediaType = mInFmtCtx->streams[tmpPacket.stream_index]->codecpar->codec_type;
        if (curMediaType == AVMEDIA_TYPE_VIDEO) {
            if (tmpPacket.flags & AV_PKT_FLAG_KEY) {
                pKeyFramePTS = new KeyFramePts;
                if (!pKeyFramePTS) {
                    HBError = HB_ERROR;
                    goto COLLECT_KEY_FRAME_PTS_END_LABEL;
                }

                pKeyFramePTS->videoPts = tmpPacket.pts;
                pKeyFramePTS->audioPts = curAudioPts;
                
                /** 后解析的帧往前插 */
                mKeyFramePtsVector.insert(mKeyFramePtsVector.begin(), pKeyFramePTS);
                if (0 != startTime && tmpPacket.pts > endTime) {
                    HBError = HB_ERROR;
                    goto COLLECT_KEY_FRAME_PTS_END_LABEL;
                }
            }
        }
        else if (curMediaType == AVMEDIA_TYPE_AUDIO) {
            curAudioPts = tmpPacket.pts;
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
