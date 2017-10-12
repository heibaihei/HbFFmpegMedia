//
//  HBVideoConcat.cpp
//  Sample
//
//  Created by zj-db0519 on 2017/10/10.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "HBVideoConcat.h"
#include "CSLog.h"
#include "CSDefine.h"
#include "CSCommon.h"

#ifdef __cplusplus
extern "C" {
#endif
    
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libavutil/mem.h"
#include "libavutil/imgutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/error.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libswscale/swscale.h"
    
#ifdef __cplusplus
};
#endif

#define CONCAT_RESOURCE_ROOT_PATH "/Users/zj-db0519/work/code/github/HbFFmpegMedia/resource/video/concat"
#define MAX_SOURCE_FILE_NUM  (3)

/**
 *  参照:  MediaFilter.cpp
 *  func: initOutFileWithoutEncode
 */

static int _initialOutputMediaContext(AVFormatContext** pOFmtCtx, AVFormatContext* TemplateFmtCtx, const char* strOutputFile, int& videoStreamIndex, int& audioStreamIndex);

int demo_video_concat_with_same_codec() {
    int HBError = HB_OK;
    char *sourceFile[MAX_SOURCE_FILE_NUM]= { (char *)CONCAT_RESOURCE_ROOT_PATH"/pieces1.mp4",
        (char *)CONCAT_RESOURCE_ROOT_PATH"/pieces2.mp4",
        (char *)CONCAT_RESOURCE_ROOT_PATH"/pieces3.mp4"
    };
    char* outputFile = (char *)CONCAT_RESOURCE_ROOT_PATH"/outputFile.mp4";
    
    av_register_all();
    
    AVFormatContext* pOutputVideoFormatCtx = nullptr;
    
    AVRational defaultTimebase;
    
    /** 外部可直观查看的时间基数 */
    defaultTimebase.num = 1;
    defaultTimebase.den = AV_TIME_BASE;
    
    int64_t tCurDealTime = 0;
    bool bIsSyncBaseOnAudio = true;
    bool bIsSyncBaseOnVideo = false;
    
    bool bOutputMediaInitialed = false;
    int oVideoStreamIndex = INVALID_STREAM_INDEX;
    int oAudioStreamIndex = INVALID_STREAM_INDEX;
    
    /** 当前输出流中基于输出timebase 计算得到的音频 & 视频的 pts 时间 */
    int64_t tCurVideoPacketPts = 0;
    int64_t tCurAudioPacketPts = 0;
    
    /** 标识当前执行拼接视频片段的起始 pts 时间 */
    int64_t tLastVideoBaseFramePts = 0;
    int64_t tLastAudioBaseFramePts = 0;
    
    /** 完成一次拼接操作后，当前输出流基于 AV_TIME_BASE 计算得到的时长  */
    int64_t tTotalVideoDuration = 0;
    int64_t tTotalAudioDuration = 0;
    
    for (int i=0; i<MAX_SOURCE_FILE_NUM; i++) {

        int iVideoStreamIndex = INVALID_STREAM_INDEX;
        int iAudioStreamIndex = INVALID_STREAM_INDEX;
        AVFormatContext* pSourceVideoFormatCtx = avformat_alloc_context();
        HBError = avformat_open_input(&pSourceVideoFormatCtx, sourceFile[i], NULL, NULL);
        if (HBError != 0) {
            LOGE("Video decoder couldn't open input file. <%d> <%s>", HBError, av_err2str(HBError));
            return HB_ERROR;
        }
        
        HBError = avformat_find_stream_info(pSourceVideoFormatCtx, NULL);
        if (HBError < 0) {
            LOGE("Video decoder couldn't find stream information. <%s>", av_err2str(HBError));
            return HB_ERROR;
        }
        
        if (!bOutputMediaInitialed) {
            if (_initialOutputMediaContext(&pOutputVideoFormatCtx, pSourceVideoFormatCtx, outputFile, oVideoStreamIndex, oAudioStreamIndex) != HB_OK) {
                LOGE("Initial output media context failed !");
                return HB_ERROR;
            }
            bOutputMediaInitialed = true;
        }
        
        for (int i=0; i<pSourceVideoFormatCtx->nb_streams; i++) {
            if (pSourceVideoFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                iVideoStreamIndex = i;
            }
            else if (pSourceVideoFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                iAudioStreamIndex = i;
            }
        }
        
        int64_t tDeltaDtsWithPts = 0;
        bool bIsFirstVideoPacket = true;
        int64_t tFirstVideoPacketPts = 0;
        int64_t tFirstVideoPacketDts = 0;
        int64_t tCurSrcMediaSegmentVideoDuration = 0;
        
        
        bool bIsFitstAudioPacket = true;
        int64_t tFirstAudioPacketPts = 0;
        int64_t tCurSrcMediaSegmentAudioDuration = 0;
        
        AVPacket *pNewPacket = av_packet_alloc();
        AVStream* pCurPacketSourceStream = nullptr;
        AVStream* pCurPacketOutputStream = nullptr;
        
        while (true) {
            HBError = av_read_frame(pSourceVideoFormatCtx, pNewPacket);
            if (HBError == 0 && pNewPacket) {
                pCurPacketSourceStream = pSourceVideoFormatCtx->streams[pNewPacket->stream_index];
                tCurDealTime = av_rescale_q_rnd(pNewPacket->pts, pCurPacketSourceStream->time_base, defaultTimebase, AV_ROUND_INF);
                
                if (pNewPacket->stream_index == iAudioStreamIndex) {
                    pCurPacketOutputStream = pOutputVideoFormatCtx->streams[oAudioStreamIndex];
                    av_packet_rescale_ts(pNewPacket, pCurPacketSourceStream->time_base, pCurPacketOutputStream->time_base);
                    
                    if (bIsFitstAudioPacket) {
                        bIsFitstAudioPacket = false;
                        tFirstAudioPacketPts = pNewPacket->pts;
                    }
                    if (tCurSrcMediaSegmentAudioDuration == 0) {/** 转换输入媒体时长，转化获取输出时长 */
                        tCurSrcMediaSegmentAudioDuration = av_rescale_q(pCurPacketSourceStream->duration,\
                                            pCurPacketSourceStream->time_base, defaultTimebase);
                        LOGI("File:%s, audio duration:%lld", sourceFile[i], tCurSrcMediaSegmentAudioDuration);
                    }
                    if (bIsSyncBaseOnAudio \
                        && tCurSrcMediaSegmentAudioDuration != 0 && tCurSrcMediaSegmentAudioDuration != 0 \
                        && tCurSrcMediaSegmentVideoDuration < tCurSrcMediaSegmentAudioDuration \
                        && tCurDealTime > tCurSrcMediaSegmentVideoDuration) {
                        LOGW("[Sync with audio] Cut audio frame, drop packet:%lld", tCurDealTime);
                        av_packet_unref(pNewPacket);
                        continue;
                    }
                    
                    pNewPacket->pts = pNewPacket->pts - tFirstAudioPacketPts \
                            + av_rescale_q(tTotalAudioDuration, AV_TIME_BASE_Q, pCurPacketOutputStream->time_base);
                    pNewPacket->dts = pNewPacket->dts - tFirstAudioPacketPts \
                            + av_rescale_q(tTotalAudioDuration, AV_TIME_BASE_Q, pCurPacketOutputStream->time_base);
                    
                    tCurAudioPacketPts = pNewPacket->pts;
                }
                else if (pNewPacket->stream_index == iVideoStreamIndex) {
                    pCurPacketOutputStream = pOutputVideoFormatCtx->streams[oVideoStreamIndex];
                    av_packet_rescale_ts(pNewPacket, pCurPacketSourceStream->time_base, pCurPacketOutputStream->time_base);
                    
                    if (bIsFirstVideoPacket) {
                        bIsFirstVideoPacket = false;
                        tFirstVideoPacketPts = pNewPacket->pts;
                        tFirstVideoPacketDts = pNewPacket->dts;
                        tDeltaDtsWithPts = tFirstVideoPacketPts - tFirstVideoPacketDts;
                        if (tDeltaDtsWithPts < 0)
                            tDeltaDtsWithPts = -tDeltaDtsWithPts;
                    }
                    if (tCurSrcMediaSegmentVideoDuration == 0) {
                        tCurSrcMediaSegmentVideoDuration = av_rescale_q(pCurPacketSourceStream->duration, \
                                            pCurPacketSourceStream->time_base, defaultTimebase);
                    }
                    pNewPacket->pts = pNewPacket->pts - tFirstVideoPacketPts \
                        + av_rescale_q(tTotalVideoDuration, AV_TIME_BASE_Q, pCurPacketOutputStream->time_base);
                    
                    if (tFirstVideoPacketPts > 0) {
                        pNewPacket->dts = pNewPacket->dts - tFirstVideoPacketPts + av_rescale_q(tTotalVideoDuration, AV_TIME_BASE_Q, pCurPacketOutputStream->time_base);
                    }
                    else {
                        pNewPacket->dts = pNewPacket->dts + tFirstVideoPacketPts + av_rescale_q(tTotalVideoDuration, AV_TIME_BASE_Q, pCurPacketOutputStream->time_base);
                    }
                    
                    tCurVideoPacketPts = pNewPacket->pts;

                    if (bIsSyncBaseOnVideo \
                        && tCurSrcMediaSegmentAudioDuration != 0 && tCurSrcMediaSegmentAudioDuration != 0 \
                        && tCurSrcMediaSegmentVideoDuration > tCurSrcMediaSegmentAudioDuration \
                        && tCurDealTime > tCurSrcMediaSegmentAudioDuration) {
                        LOGW("[Sync with audio] Cut video frame, drop packet:%lld", tCurDealTime);
                        av_packet_unref(pNewPacket);
                        continue;
                    }
                }
                
                /** For debug */
                tCurDealTime = av_rescale_q_rnd(pNewPacket->pts, pCurPacketOutputStream->time_base, defaultTimebase, AV_ROUND_INF);
                LOGI("Cur deal with time:%lld", tCurDealTime);
                
                if (av_interleaved_write_frame(pOutputVideoFormatCtx, pNewPacket) < 0) {
                    LOGE("Write packet failed !");
                    return HB_ERROR;
                }
                av_packet_unref(pNewPacket);
            }
            else if (HBError == AVERROR_EOF) {
                LOGW("Current media file reach eof !");
                break;
            }
        }
        
        tLastVideoBaseFramePts = tCurVideoPacketPts + 1; /** tCurVideoPacketPts + tDeltaDtsWithPts */
        tLastAudioBaseFramePts = tCurAudioPacketPts + 1;
        
        /** 计算当前片段经过转换后，基于 AV_TIME_BASE 时间基转换后的时长 */
        int64_t tCurSegmentVideoDuration = 0;
        if (oVideoStreamIndex >= 0) {
            tCurSegmentVideoDuration = av_rescale_q_rnd(tLastVideoBaseFramePts, pOutputVideoFormatCtx->streams[oVideoStreamIndex]->time_base, AV_TIME_BASE_Q, AV_ROUND_INF) - tTotalVideoDuration;
            tTotalVideoDuration = av_rescale_q_rnd(tLastVideoBaseFramePts, pOutputVideoFormatCtx->streams[oVideoStreamIndex]->time_base, AV_TIME_BASE_Q, AV_ROUND_INF);
        }
        
        int64_t tCurSegmentAudioDuration = 0;
        if (oAudioStreamIndex >= 0) {
            tCurSegmentAudioDuration = av_rescale_q_rnd(tLastAudioBaseFramePts, pOutputVideoFormatCtx->streams[oAudioStreamIndex]->time_base, AV_TIME_BASE_Q, AV_ROUND_INF) - tTotalAudioDuration;
            tTotalAudioDuration  = av_rescale_q_rnd(tLastAudioBaseFramePts, pOutputVideoFormatCtx->streams[oAudioStreamIndex]->time_base, AV_TIME_BASE_Q, AV_ROUND_INF);
        }
        
        if (oVideoStreamIndex >=0 && oAudioStreamIndex >= 0) {
            if (tTotalAudioDuration > tTotalVideoDuration)
                tTotalVideoDuration = tTotalAudioDuration;
        }
        
        avformat_close_input(&pSourceVideoFormatCtx);
    }

LABEL_CONCAT_EXIT:
    if (pOutputVideoFormatCtx) {
        if (0 != av_write_trailer(pOutputVideoFormatCtx)) {
            LOGE("Audio write tailer failed !");
            return HB_ERROR;
        }
        avformat_close_input(&pOutputVideoFormatCtx);
        pOutputVideoFormatCtx = nullptr;
    }
    
    return HB_OK;
}

int demo_video_concat_with_diffence_codec() {
    return HB_OK;
}

static int _initialOutputMediaContext(AVFormatContext** pOFmtCtx, AVFormatContext* TemplateFmtCtx, const char* strOutputFile, int& videoStreamIndex, int& audioStreamIndex) {
    if (!pOFmtCtx || !TemplateFmtCtx || !strOutputFile) {
        LOGE("Initial output media context failed , invalid params !");
        return HB_ERROR;
    }
    
    AVFormatContext* pOutputVideoFormatCtx = avformat_alloc_context();
    pOutputVideoFormatCtx->oformat = av_guess_format(NULL, strOutputFile, NULL);
    
    videoStreamIndex = INVALID_STREAM_INDEX;
    audioStreamIndex = INVALID_STREAM_INDEX;
    for (int i=0; i<TemplateFmtCtx->nb_streams; i++) {
        AVStream *pInStream = TemplateFmtCtx->streams[i];
        
        AVCodec* pOutputCodec = avcodec_find_encoder(pInStream->codecpar->codec_id);
        if (pOutputCodec == NULL) {
            LOGE("Can not find audio encoder! %d\n", pOutputCodec->id);
            return HB_ERROR;
        }
        
        AVStream *pOutStream = avformat_new_stream(pOutputVideoFormatCtx, pOutputCodec);
        if (!pOutStream) {
            LOGE("Create output stream failed !");
            return HB_ERROR;
        }
        
        avcodec_parameters_copy(pOutStream->codecpar, pInStream->codecpar);
        if (pOutStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            audioStreamIndex = i;
        else if (pOutStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            videoStreamIndex = i;
        
        av_dict_copy(&pOutStream->metadata, pInStream->metadata, AV_DICT_DONT_OVERWRITE);
    }
    
    if (!(pOutputVideoFormatCtx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&pOutputVideoFormatCtx->pb, strOutputFile, AVIO_FLAG_READ_WRITE) < 0) {
            LOGE("Failed to open output file: %s!\n", strOutputFile);
            return HB_ERROR;
        }
    }
    
    strcpy(pOutputVideoFormatCtx->filename, strOutputFile);
    
    AVDictionary * dict = NULL;
    av_dict_set(&dict, "movflags", "faststart", 0);
    if (avformat_write_header(pOutputVideoFormatCtx, &dict) < 0) {
        LOGE("Avformat write header failed !");
        return HB_ERROR;
    }
    av_dict_free(&dict);

    *pOFmtCtx = pOutputVideoFormatCtx;
    
    return HB_OK;
}
