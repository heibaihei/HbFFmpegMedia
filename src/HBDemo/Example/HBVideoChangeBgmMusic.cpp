//
//  HBVideoChangeBgmMusic.cpp
//  Sample
//
//  Created by zj-db0519 on 2017/10/13.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "HBVideoChangeBgmMusic.h"

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

#define USE_H264BSF 0
#define USE_AACBSF  0
/** 参考链接： http://blog.csdn.net/king1425/article/details/72628607 */

#define CONCAT_RESOURCE_ROOT_PATH "/Users/zj-db0519/work/code/github/HbFFmpegMedia/resource/video/change_music"
int demo_video_change_bgm_music() {
    char *strVideoFile = (char *)CONCAT_RESOURCE_ROOT_PATH"/100.mp4";
    char *strMusicFile = (char *)CONCAT_RESOURCE_ROOT_PATH"/music_1.mp3";
    char *strOutputFile = (char *)CONCAT_RESOURCE_ROOT_PATH"/outputVideo.mp4";
    
    av_register_all();
    
    int HBErr = HB_OK;
    AVFormatContext *pInputVideoFmtCtx = nullptr, *pInputAudioFmtCtx = nullptr;
    if ((HBErr = avformat_open_input(&pInputVideoFmtCtx, strVideoFile, nullptr, nullptr)) < 0) {
        LOGE("Open input video media file failed, %s !", av_err2str(HBErr));
        return HB_ERROR;
    }
    if ((HBErr = avformat_find_stream_info(pInputVideoFmtCtx, NULL)) < 0) {
        LOGE("Find input video streams info failed , %s !", av_err2str(HBErr));
        return HB_ERROR;
    }
    
    if ((HBErr = avformat_open_input(&pInputAudioFmtCtx, strMusicFile, nullptr, nullptr)) < 0) {
        LOGE("Open input audio media file failed, %s !", av_err2str(HBErr));
        return HB_ERROR;
    }
    if ((HBErr = avformat_find_stream_info(pInputAudioFmtCtx, NULL)) < 0) {
        LOGE("Find input audio streams info failed , %s !", av_err2str(HBErr));
        return HB_ERROR;
    }
    
    LOGD("===========Input Information==========\n");
    av_dump_format(pInputVideoFmtCtx, 0, strVideoFile, 0);
    av_dump_format(pInputAudioFmtCtx, 0, strMusicFile, 0);
    LOGD("======================================\n");
    
    AVFormatContext *pOutputVideoFmtCtx = nullptr;
    avformat_alloc_output_context2(&pOutputVideoFmtCtx, NULL, NULL, strOutputFile);
    if (!pOutputVideoFmtCtx) {
        LOGE("Create output video fotmat failed !");
        return HB_ERROR;
    }
    int iVideoStreamIndex = INVALID_STREAM_INDEX;
    int iAudioStreamIndex = INVALID_STREAM_INDEX;
    int oVideoStreamIndex = INVALID_STREAM_INDEX;
    int oAudioStreamIndex = INVALID_STREAM_INDEX;
    AVOutputFormat *pOutputFormat = pOutputVideoFmtCtx->oformat;
    
    for (int i=0; i<pInputVideoFmtCtx->nb_streams; i++) {
        if (pInputVideoFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            iVideoStreamIndex = i;
            AVStream *pInputVideoStream = pInputVideoFmtCtx->streams[iVideoStreamIndex];
            AVCodec *pOutputVideoCodec = avcodec_find_encoder(pInputVideoStream->codecpar->codec_id);
            if (!pOutputVideoCodec) {
                LOGE("Find output video codec failed !");
                return HB_ERROR;
            }
            AVStream *pOutputVideoStream = avformat_new_stream(pOutputVideoFmtCtx, pOutputVideoCodec);
            if (!pOutputVideoStream) {
                LOGE("Find output video stream failed !");
                return HB_ERROR;
            }
            oVideoStreamIndex = pOutputVideoStream->index;
            avcodec_parameters_copy(pOutputVideoStream->codecpar, pInputVideoStream->codecpar);
            uint32_t tag = 0;
            if (av_codec_get_tag2(pOutputVideoFmtCtx->oformat->codec_tag, pOutputVideoCodec->id, &tag) == 0) {
                LOGW("could not find codec tag for codec id %d, default to 0", pOutputVideoCodec->id);
            }
            pOutputVideoStream->codecpar->codec_tag = tag;
//            if (pOutputVideoFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
//                pOutputVideoStream->codecpar->codec_tag |= CODEC_FLAG_GLOBAL_HEADER;
            break;
        }
    }
    
    for (int i=0; i<pInputAudioFmtCtx->nb_streams; i++) {
        if (pInputAudioFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            iAudioStreamIndex = i;
            AVStream *pInputAudioStream = pInputAudioFmtCtx->streams[iAudioStreamIndex];
            AVCodec *pOutputAudioCodec = avcodec_find_encoder(pInputAudioStream->codecpar->codec_id);
            if (!pOutputAudioCodec) {
                LOGE("Find output audio codec failed ![%s] ", avcodec_get_name(pInputAudioStream->codecpar->codec_id));
                return HB_ERROR;
            }
            AVStream *pOutputAudioStream = avformat_new_stream(pOutputVideoFmtCtx, pOutputAudioCodec);
            if (!pOutputAudioStream) {
                LOGE("Find output audio stream failed !");
                return HB_ERROR;
            }
            oAudioStreamIndex = pOutputAudioStream->index;
            avcodec_parameters_copy(pOutputAudioStream->codecpar, pInputAudioStream->codecpar);
            uint32_t tag = 0;
            if (av_codec_get_tag2(pOutputVideoFmtCtx->oformat->codec_tag, pOutputAudioCodec->id, &tag) == 0) {
                LOGW("could not find codec tag for codec id %d, default to 0", pOutputAudioCodec->id);
            }
            pOutputAudioStream->codecpar->codec_tag = tag;
//            if (pOutputVideoFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
//                pOutputAudioStream->codecpar->codec_tag |= CODEC_FLAG_GLOBAL_HEADER;
            break;
        }
    }
    
    LOGD("==========Output Information==========\n");
    av_dump_format(pOutputVideoFmtCtx, 0, strOutputFile, 1);
    LOGD("======================================\n");
   
    if (!(pOutputFormat->flags & AVFMT_NOFILE)) {
        if (avio_open(&pOutputVideoFmtCtx->pb, strOutputFile, AVIO_FLAG_WRITE) < 0) {
            LOGE("Open output video file failed, %s !", strOutputFile);
            return HB_ERROR;
        }
    }
    
    if (avformat_write_header(pOutputVideoFmtCtx, NULL) < 0) {
        LOGE("Write output video file header failed !");
        return HB_ERROR;
    }
    
#if USE_H264BSF
    AVBitStreamFilterContext* pH264bsfcFilter =  av_bitstream_filter_init("h264_mp4toannexb");
#endif
#if USE_AACBSF
    AVBitStreamFilterContext* pAacbsfcFilter =  av_bitstream_filter_init("aac_adtstoasc");
#endif
    
    int  iVideoFrameIndex = 0;
    int  iAudioFrameIndex = 0;
    
    AVRational stDefaultTimeBase;
    stDefaultTimeBase.num = 1;
    stDefaultTimeBase.den = AV_TIME_BASE;
    int64_t tCurrentPts = 0;
    AVPacket *pNewPacket = av_packet_alloc();
    int64_t tCurVideoPts=0, tCurAudioPts=0;
    AVStream *pCurWorkInputStream = nullptr, *pCurWorkOutputStream = nullptr;
    int64_t tFirstInputVideoPacketPts = AV_NOPTS_VALUE;
    int64_t tFirstInputVideoPacketDts = AV_NOPTS_VALUE;
    int64_t tFirstInputAudioPacketPts = AV_NOPTS_VALUE;
    
    bool bVideoReachEof = false;
    while (1) {
        int iCurWorkStreamIndex = INVALID_STREAM_INDEX;
        AVFormatContext *pCurWorkInputFmtCtx = nullptr;
        
        /** av_compare_ts是比较时间戳用的。通过该函数可以决定该写入视频还是音频 */
        if(av_compare_ts(tCurVideoPts, pInputVideoFmtCtx->streams[iVideoStreamIndex]->time_base, \
                         tCurAudioPts, pInputAudioFmtCtx->streams[iAudioStreamIndex]->time_base) <= 0)
        {
            HBErr = HB_OK;
            pCurWorkInputFmtCtx = pInputVideoFmtCtx;
            iCurWorkStreamIndex = oVideoStreamIndex;
            
            if ((HBErr = av_read_frame(pInputVideoFmtCtx, pNewPacket)) == 0) {
                pCurWorkInputStream = pInputVideoFmtCtx->streams[iVideoStreamIndex];
                pCurWorkOutputStream = pOutputVideoFmtCtx->streams[oVideoStreamIndex];
                
                do {
                    if (pNewPacket->stream_index == iVideoStreamIndex) {
                        pNewPacket->stream_index = pCurWorkOutputStream->index;
                        tCurVideoPts = pNewPacket->pts;
                        av_packet_rescale_ts(pNewPacket, pCurWorkInputStream->time_base, pCurWorkOutputStream->time_base);
                        
                        if (iVideoFrameIndex == 0) {
                            tFirstInputVideoPacketDts = pNewPacket->dts;
                            tFirstInputVideoPacketPts = pNewPacket->pts;
                        }
                        pNewPacket->pts = pNewPacket->pts - tFirstInputVideoPacketPts;
                        if (tFirstInputVideoPacketPts > 0)
                            pNewPacket->dts = pNewPacket->dts - tFirstInputVideoPacketPts;
                        else
                            pNewPacket->dts = pNewPacket->dts + tFirstInputVideoPacketPts;
                        
                        
                        iVideoFrameIndex++;
                        break; /** 得到一个目标帧就即保存 */
                    }
                } while ((HBErr = av_read_frame(pInputVideoFmtCtx, pNewPacket)) == 0);
            }
            
            if (HBErr < 0) {
                if (HBErr == AVERROR_EOF)
                    bVideoReachEof = true;
                LOGW("Input video : %s !", av_err2str(HBErr));
                break;
            }
            
        }
        else {
            pCurWorkInputFmtCtx = pInputAudioFmtCtx;
            iCurWorkStreamIndex = oAudioStreamIndex;
            if (av_read_frame(pInputAudioFmtCtx, pNewPacket) == 0) {
                pCurWorkInputStream = pInputAudioFmtCtx->streams[iAudioStreamIndex];
                pCurWorkOutputStream = pOutputVideoFmtCtx->streams[oAudioStreamIndex];
                
                do {
                    if (pNewPacket->stream_index == iAudioStreamIndex && pCurWorkInputStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                        pNewPacket->stream_index = pCurWorkOutputStream->index;
                        
                        tCurAudioPts = pNewPacket->pts;
                        av_packet_rescale_ts(pNewPacket, pCurWorkInputStream->time_base, pCurWorkOutputStream->time_base);
                        
                        if (iAudioFrameIndex == 0) {
                            tFirstInputAudioPacketPts = pNewPacket->pts;
                        }
                        
                        pNewPacket->pts = pNewPacket->pts - tFirstInputAudioPacketPts;
                        pNewPacket->dts = pNewPacket->dts - tFirstInputAudioPacketPts;
                        iAudioFrameIndex++;
                        break; /** 得到一个目标帧就即保存 */
                    }
                } while (av_read_frame(pInputVideoFmtCtx, pNewPacket) == 0);
            }
            else
                break;
        }
        
        //FIX:Bitstream Filter
#if USE_H264BSF
//        (AVBitStreamFilterContext *bsfc,
//         AVCodecContext *avctx, const char *args,
//         uint8_t **poutbuf, int *poutbuf_size,
//         const uint8_t *buf, int buf_size, int keyframe)
        
        av_bitstream_filter_filter(pH264bsfcFilter, pCurWorkInputStream->codec, \
                  NULL, &(pNewPacket->data), &(pNewPacket->size), pNewPacket->data, pNewPacket->size, 0);
#endif
#if USE_AACBSF
        av_bitstream_filter_filter(pAacbsfcFilter, pCurWorkOutputStream->codec, \
                  NULL, &(pNewPacket->data), &(pNewPacket->size), pNewPacket->data, pNewPacket->size, 0);
#endif
        
        tCurrentPts = av_rescale_q_rnd(pNewPacket->pts, pCurWorkOutputStream->time_base, stDefaultTimeBase, AV_ROUND_INF);
        
        LOGF("[Huangcl] [V:%d : A:%d ==> T:%d] - packet.%d duration:%lld, Ext:%lld, pts:%lld, dts:%lld", oVideoStreamIndex, oAudioStreamIndex, \
             pNewPacket->stream_index, (pNewPacket->stream_index == oVideoStreamIndex ? iVideoFrameIndex : iAudioFrameIndex),\
             pNewPacket->duration, tCurrentPts, pNewPacket->pts, pNewPacket->dts);

        if ((HBErr = av_interleaved_write_frame(pOutputVideoFmtCtx, pNewPacket)) < 0) {
            LOGE( "Error muxing packet, %s\n", av_err2str(HBErr));
            break;
        }
        av_packet_unref(pNewPacket);
    }
    
    av_write_trailer(pOutputVideoFmtCtx);

#if USE_H264BSF
    av_bitstream_filter_close(pH264bsfcFilter);
#endif
#if USE_AACBSF
    av_bitstream_filter_close(pAacbsfcFilter);
#endif

    avformat_close_input(&pInputAudioFmtCtx);
    avformat_close_input(&pInputVideoFmtCtx);
    
//    if (pOutputVideoFmtCtx && !(pOutputFormat->flags & AVFMT_NOFILE)) {
//        avio_close(pOutputVideoFmtCtx->pb);
//    }
    avformat_close_input(&pOutputVideoFmtCtx);
    
    return HB_OK;
}
