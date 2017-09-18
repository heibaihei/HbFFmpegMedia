//
//  InFileContext.cpp
//  audioDecode
//
//  Created by Adiya on 17/05/2017.
//  Copyright © 2017 鹰视工作室. All rights reserved.
//

#include "InFileContext.hpp"
#include "Error.h"

InFileContext::InFileContext()
{
    ifmtCtx = NULL;
    audioDectx = NULL;
}

InFileContext::~InFileContext()
{
    
}

static int open_codec_context(int *stream_idx, AVFormatContext *fmt_ctx, AVCodecContext **dec_ctx, enum AVMediaType type)
{
    int ret;
    AVStream *st;
    AVCodec *dec = NULL;
    *stream_idx = -1;
    
    if ((ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not find %s stream !(%s)\n",
             av_get_media_type_string(type), makeErrorStr(ret));
        return ret;
    } else {
        *stream_idx = ret;
        st = fmt_ctx->streams[*stream_idx];
        /* find decoder for the stream */
        *dec_ctx = st->codec;
        dec = avcodec_find_decoder((*dec_ctx)->codec_id);
        if (!dec) {
            av_log(NULL, AV_LOG_ERROR, "Failed to find %s codec(%s)\n",
                 av_get_media_type_string(type), makeErrorStr(ret));
            return ret;
        }
        if ((ret = avcodec_open2(*dec_ctx, dec, NULL)) < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to open %s codec(%s)\n",
                 av_get_media_type_string(type), makeErrorStr(ret));
            return ret;
        }
    }
    return 0;
}

int InFileContext::open(const char *filename)
{
    int ret;
    AVCodecContext *audioCtx = NULL;
    AVStream *audioStream;
    
    if (ifmtCtx != NULL) {
        avformat_close_input(&ifmtCtx);
        ifmtCtx = NULL;
    }
    if ((ret = avformat_open_input(&ifmtCtx, filename, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error: Could not open %s (%s)\n", filename, makeErrorStr(ret));
        avformat_close_input(&ifmtCtx);
        ifmtCtx = NULL;
        return AV_FILE_ERR;
    }
    if ((ret = avformat_find_stream_info(ifmtCtx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR,"Could not find stream information (%s)\n", makeErrorStr(ret));
        avformat_close_input(&ifmtCtx);
        ifmtCtx = NULL;
        ret = AV_STREAM_ERR;
        goto TAR_OUT;

    }
    
    ret = open_codec_context(&audioIndex, ifmtCtx, &audioCtx, AVMEDIA_TYPE_AUDIO);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Open Audio codec error!\n");
        goto TAR_OUT;
    }
    
    audioStream = ifmtCtx->streams[audioIndex];
    
    if (audioStream && audioStream->duration != AV_NOPTS_VALUE) {
        double divideFactor;
        divideFactor = (double)1 / av_q2d(audioStream->time_base);
        durationOfAudio = (double)audioStream->duration / divideFactor;
    }
    
    audioDectx = new AudioDecoder();
    if (audioDectx == NULL) {
        ret = AV_MALLOC_ERR;
        av_log(NULL, AV_LOG_ERROR, "New audio decodecontext error!\n");
        goto TAR_OUT;
    }
    
    audioDectx->setDecoder(audioCtx);
    
    
TAR_OUT:
    
    if (ret < 0) {
        if (ifmtCtx) {
            avformat_close_input(&ifmtCtx);
        }
        if (audioDectx) {
            audioDectx->close();
            delete audioDectx;
            audioDectx = NULL;
        }
        if (audioCtx) {
            avcodec_close(audioCtx);
        }
    }
    return ret;
}

int InFileContext::readPacket(AVPacket *pkt)
{
    int ret;
    
    ret = av_read_frame(ifmtCtx, pkt);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Read exit reason [%s]\n", makeErrorStr(ret));
        goto TAR_OUT;
    }
    
TAR_OUT:
    
    return ret;
}

AudioDecoder *InFileContext::getAudioDecoder()
{
    return audioDectx;
}

int InFileContext::getAudioIndex()
{
    return audioIndex;
}

int InFileContext::getVideoIndex()
{
    return videoIndex;
}

double InFileContext::getDuration()
{
    return durationOfAudio;
}

int InFileContext::close()
{
    int ret = 0;
    
    if (audioDectx) {
        audioDectx->close();
        delete audioDectx;
        audioDectx = NULL;
    }
    
    if (ifmtCtx) {
        avformat_close_input(&ifmtCtx);
    }
    
    return ret;
}
