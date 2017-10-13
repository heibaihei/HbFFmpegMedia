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
static int _initialInputMediaContext(AVFormatContext** pInFmtCtx, const char* strInputFile, int& videoStreamIndex, int& audioStreamIndex);

int demo_video_concat_with_same_codec() {
    int HBError = HB_OK;
    char *sourceFile[MAX_SOURCE_FILE_NUM]= { (char *)CONCAT_RESOURCE_ROOT_PATH"/pieces1.mp4",
        (char *)CONCAT_RESOURCE_ROOT_PATH"/pieces2.mp4",
        (char *)CONCAT_RESOURCE_ROOT_PATH"/pieces3.mp4"
    };
    char* outputFile = (char *)CONCAT_RESOURCE_ROOT_PATH"/outputFile.mp4";
    
    av_register_all();
    
    AVFormatContext* pOutputVideoFormatCtx = nullptr;
    
    /** 外部可直观查看的时间基数 */
    AVRational defaultTimebase;
    defaultTimebase.num = 1;
    defaultTimebase.den = AV_TIME_BASE;
    
    bool bIsSyncBaseOnAudio = true;
    bool bIsSyncBaseOnVideo = false;
    bool bOutputMediaInitialed = false;
    
    /** 输出媒体文件中对应的 音&视频 流索引 */
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
        
        AVFormatContext* pSourceVideoFormatCtx = nullptr;
        if (_initialInputMediaContext(&pSourceVideoFormatCtx, sourceFile[i], iVideoStreamIndex, iAudioStreamIndex) != HB_OK) {
            LOGE("Initial input media context failed, %s", sourceFile[i]);
            return HB_ERROR;
        }

        if (!bOutputMediaInitialed) {
            if (_initialOutputMediaContext(&pOutputVideoFormatCtx, pSourceVideoFormatCtx, outputFile, oVideoStreamIndex, oAudioStreamIndex) != HB_OK) {
                LOGE("Initial output media context failed, %s !", outputFile);
                return HB_ERROR;
            }
            bOutputMediaInitialed = true;
        }
        
        int64_t tDeltaDtsWithPts = 0;
        bool bIsFirstVideoPacket = true;
        int64_t tCurSourceSegmentFirstVideoPacketPts = 0;
        int64_t tCurSourceSegmentFirstVideoPacketDts = 0;
        int64_t tCurSourceMediaSegmentVideoDuration = 0;
        
        bool bIsCurSegmentFitstAudioPacket = true;
        int64_t tCurSegmentFirstAudioPacketPts = 0;
        /** 当前正在拼接的源片段，转换成外部可读性的时间基后得到的时长 */
        int64_t tCurSourceMediaSegmentAudioDuration = 0;
        
        AVPacket *pNewPacket = av_packet_alloc();
        while (true) {
            HBError = av_read_frame(pSourceVideoFormatCtx, pNewPacket);
            if (HBError == 0 && pNewPacket) {
                /** pCurPacketSourceStream & pCurPacketOutputStream 指向当前数据包对应的输入和输出媒体流 */
                AVStream* pCurPacketOutputStream = nullptr;
                AVStream* pCurPacketSourceStream = pSourceVideoFormatCtx->streams[pNewPacket->stream_index];
                
                /**
                 * 转换当前数据包 pts 转换成外部阅读的时间基数 defaultTimebase
                 * tCurDealTime 标识当前正在处理的包的显示时间
                 */
                int64_t tCurDealTime = av_rescale_q_rnd(pNewPacket->pts, pCurPacketSourceStream->time_base, defaultTimebase, AV_ROUND_INF);
                
                if (pNewPacket->stream_index == iAudioStreamIndex) {
                    pCurPacketOutputStream = pOutputVideoFormatCtx->streams[oAudioStreamIndex];
                    
                    /** 将 数据包的 pts 时间基由输入流的时间基 转换成对应输出流的时间基 */
                    av_packet_rescale_ts(pNewPacket, pCurPacketSourceStream->time_base, pCurPacketOutputStream->time_base);
                    
                    LOGD("Audio packet duration:%lld", pNewPacket->duration);
                    
                    if (bIsCurSegmentFitstAudioPacket) {
                        bIsCurSegmentFitstAudioPacket = false;
                        tCurSegmentFirstAudioPacketPts = pNewPacket->pts;
                    }
                    if (tCurSourceMediaSegmentAudioDuration == 0) {/** 转换输入媒体时长，转化获取输出时长 */
                        tCurSourceMediaSegmentAudioDuration = av_rescale_q(pCurPacketSourceStream->duration,\
                                            pCurPacketSourceStream->time_base, defaultTimebase);
                        LOGI("File:%s, audio duration:%lld", sourceFile[i], tCurSourceMediaSegmentAudioDuration);
                    }
                    if (bIsSyncBaseOnAudio \
                        && tCurSourceMediaSegmentAudioDuration != 0 && tCurSourceMediaSegmentVideoDuration != 0 \
                        && tCurSourceMediaSegmentVideoDuration < tCurSourceMediaSegmentAudioDuration \
                        && tCurDealTime > tCurSourceMediaSegmentVideoDuration) {
                        /** 如果以音频时间为准，但是当前包已经超过了视频时长，则将当前音频包丢弃 */
                        LOGW("[Sync with audio] Cut audio frame, drop packet:%lld", tCurDealTime);
                        av_packet_unref(pNewPacket);
                        continue;
                    }
                    
                    pNewPacket->pts = pNewPacket->pts - tCurSegmentFirstAudioPacketPts \
                            + av_rescale_q(tTotalAudioDuration, AV_TIME_BASE_Q, pCurPacketOutputStream->time_base);
                    pNewPacket->dts = pNewPacket->dts - tCurSegmentFirstAudioPacketPts \
                            + av_rescale_q(tTotalAudioDuration, AV_TIME_BASE_Q, pCurPacketOutputStream->time_base);
                    
                    tCurAudioPacketPts = pNewPacket->pts;
                }
                else if (pNewPacket->stream_index == iVideoStreamIndex) {
                    pCurPacketOutputStream = pOutputVideoFormatCtx->streams[oVideoStreamIndex];
                    av_packet_rescale_ts(pNewPacket, pCurPacketSourceStream->time_base, pCurPacketOutputStream->time_base);
                    
                    LOGD("Video packet duration:%lld", pNewPacket->duration);
                    
                    if (bIsFirstVideoPacket) {
                        bIsFirstVideoPacket = false;
                        tCurSourceSegmentFirstVideoPacketPts = pNewPacket->pts;
                        tCurSourceSegmentFirstVideoPacketDts = pNewPacket->dts;
                        tDeltaDtsWithPts = tCurSourceSegmentFirstVideoPacketPts - tCurSourceSegmentFirstVideoPacketDts;
                        if (tDeltaDtsWithPts < 0)
                            tDeltaDtsWithPts = -tDeltaDtsWithPts;
                    }
                    if (tCurSourceMediaSegmentVideoDuration == 0) {/** 转换输入媒体时长，转化获取输出时长 */
                        tCurSourceMediaSegmentVideoDuration = av_rescale_q(pCurPacketSourceStream->duration, \
                                            pCurPacketSourceStream->time_base, defaultTimebase);
                        LOGI("File:%s, video duration:%lld", sourceFile[i], tCurSourceMediaSegmentVideoDuration);
                    }
                    pNewPacket->pts = pNewPacket->pts - tCurSourceSegmentFirstVideoPacketPts \
                        + av_rescale_q(tTotalVideoDuration, AV_TIME_BASE_Q, pCurPacketOutputStream->time_base);
                    
                    if (tCurSourceSegmentFirstVideoPacketPts > 0) {
                        pNewPacket->dts = pNewPacket->dts - tCurSourceSegmentFirstVideoPacketPts + av_rescale_q(tTotalVideoDuration, AV_TIME_BASE_Q, pCurPacketOutputStream->time_base);
                    }
                    else {
                        pNewPacket->dts = pNewPacket->dts + tCurSourceSegmentFirstVideoPacketPts + av_rescale_q(tTotalVideoDuration, AV_TIME_BASE_Q, pCurPacketOutputStream->time_base);
                    }
                    
                    tCurVideoPacketPts = pNewPacket->pts;

                    if (bIsSyncBaseOnVideo \
                        && tCurSourceMediaSegmentAudioDuration != 0 && tCurSourceMediaSegmentVideoDuration != 0 \
                        && tCurSourceMediaSegmentVideoDuration > tCurSourceMediaSegmentAudioDuration \
                        && tCurDealTime >= tCurSourceMediaSegmentAudioDuration) {
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
        av_packet_free(&pNewPacket);
        
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
    
    AVFormatContext* pTargetVideoFormatCtx = avformat_alloc_context();
    pTargetVideoFormatCtx->oformat = av_guess_format(NULL, strOutputFile, NULL);
    
    videoStreamIndex = INVALID_STREAM_INDEX;
    audioStreamIndex = INVALID_STREAM_INDEX;
    for (int i=0; i<TemplateFmtCtx->nb_streams; i++) {
        AVStream *pInStream = TemplateFmtCtx->streams[i];
        
        AVCodec* pOutputCodec = avcodec_find_encoder(pInStream->codecpar->codec_id);
        if (pOutputCodec == NULL) {
            LOGE("Can not find audio encoder! %d\n", pOutputCodec->id);
            return HB_ERROR;
        }
        
        AVStream *pOutStream = avformat_new_stream(pTargetVideoFormatCtx, pOutputCodec);
        if (!pOutStream) {
            LOGE("Create output stream failed !");
            return HB_ERROR;
        }
        
        avcodec_parameters_copy(pOutStream->codecpar, pInStream->codecpar);
        if (pOutStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            audioStreamIndex = i;
        else if (pOutStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            videoStreamIndex = i;

        uint32_t tag = 0;
        if (av_codec_get_tag2(pTargetVideoFormatCtx->oformat->codec_tag, pOutputCodec->id, &tag) == 0) {
            LOGW("could not find codec tag for codec id %d, default to 0", pOutputCodec->id);
        }
        pOutStream->codecpar->codec_tag = tag;
        /* 待确认无法设置原因
        if (pTargetVideoFormatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
            pOutStream->codecpar->codec_tag  |= AV_CODEC_FLAG_GLOBAL_HEADER;
        } */
        av_dict_copy(&pOutStream->metadata, pInStream->metadata, AV_DICT_DONT_OVERWRITE);
    }
    
    if (!(pTargetVideoFormatCtx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&pTargetVideoFormatCtx->pb, strOutputFile, AVIO_FLAG_READ_WRITE) < 0) {
            LOGE("Failed to open output file: %s!\n", strOutputFile);
            return HB_ERROR;
        }
    }
    
    strcpy(pTargetVideoFormatCtx->filename, strOutputFile);
    
    AVDictionary * dict = NULL;
    av_dict_set(&dict, "movflags", "faststart", 0);
    int HBError = avformat_write_header(pTargetVideoFormatCtx, &dict);
    if (HBError < 0) {
        LOGE("Avformat write header failed, %s !", av_err2str(HBError));
        return HB_ERROR;
    }
    av_dict_free(&dict);

    *pOFmtCtx = pTargetVideoFormatCtx;
    
    return HB_OK;
}

static int _initialInputMediaContext(AVFormatContext** pInFmtCtx, const char* strInputFile, int& videoStreamIndex, int& audioStreamIndex) {
    if (!pInFmtCtx || !strInputFile) {
        LOGE("Initial input media context failed, invalid params !");
        return HB_ERROR;
    }
    
    int HBError = HB_OK;
    videoStreamIndex = INVALID_STREAM_INDEX;
    audioStreamIndex = INVALID_STREAM_INDEX;
    
    AVFormatContext* pTargetVideoFormatCtx = nullptr;
    HBError = avformat_open_input(&pTargetVideoFormatCtx, strInputFile, NULL, NULL);
    if (HBError != 0) {
        LOGE("Video decoder couldn't open input file. <%d> <%s>", HBError, av_err2str(HBError));
        return HB_ERROR;
    }
    
    HBError = avformat_find_stream_info(pTargetVideoFormatCtx, NULL);
    if (HBError < 0) {
        LOGE("Video decoder couldn't find stream information. <%s>", av_err2str(HBError));
        return HB_ERROR;
    }
    
    for (int i=0; i<pTargetVideoFormatCtx->nb_streams; i++) {
        if (pTargetVideoFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            videoStreamIndex = i;
        else if (pTargetVideoFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            audioStreamIndex = i;
    }
    
    *pInFmtCtx = pTargetVideoFormatCtx;
    return HB_OK;
}
