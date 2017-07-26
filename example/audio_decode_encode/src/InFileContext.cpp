//
//  InFileContext.cpp
//  audioDecode
//
//  Created by Adiya on 17/05/2017.
//  Copyright © 2017 meitu. All rights reserved.
//

#include "InFileContext.hpp"
#include "Error.h"
#include "Utils.h"

InFileContext::InFileContext()
{
    ifmtCtx = NULL;
    audioDectx = NULL;
    frame = NULL;
    needResample = false;
    audioBuf = NULL;
    programStat = false;
    resampler = NULL;
    audioIndex = -1;
    memset(&outAudioParam, 0, sizeof(AudioParam_t));
}

InFileContext::~InFileContext()
{
    
}

static int open_codec_context(int *stream_idx, AVFormatContext *fmt_ctx, AVCodecContext **dec_ctx, enum AVMediaType type)
{
    int ret = 0;
    AVStream *st;
    AVCodec *dec = NULL;
    AVCodecContext *ctx;
    AVCodecParameters *codecParam;
    *stream_idx = -1;
    
    if ((ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not find %s stream !(%s)\n",
             av_get_media_type_string(type), makeErrorStr(ret));
        return ret;
    } else {
        *stream_idx = ret;
        st = fmt_ctx->streams[*stream_idx];
        /* find decoder for the stream */
        codecParam = st->codecpar;
        
        ctx = avcodec_alloc_context3(avcodec_find_decoder(codecParam->codec_id));
        if (ctx == NULL) {
            av_log(NULL, AV_LOG_ERROR, "Alloc context error!\n");
            goto TAR_OUT;
        }
        ret = avcodec_parameters_to_context(ctx, codecParam);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Copy parameters error\n");
            goto TAR_OUT;
        }

        dec = avcodec_find_decoder(codecParam->codec_id);
        if (dec == NULL) {
            av_log(NULL, AV_LOG_ERROR, "Failed to find %s codec(%s)\n",
                 av_get_media_type_string(type), makeErrorStr(ret));
            return ret;
        }
        if ((ret = avcodec_open2(ctx, dec, NULL)) < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to open %s codec(%s)\n",
                 av_get_media_type_string(type), makeErrorStr(ret));
            return ret;
        }
        *dec_ctx = ctx;
    }
    
TAR_OUT:
    
    if (ret < 0) {
        if (ctx) {
            avcodec_free_context(&ctx);
        }
        *dec_ctx = NULL;
    }
    
    return ret;
}

int InFileContext::open(const char *filename)
{
    int ret;
    AVCodecContext *audioCtx = NULL;
    AVStream *audioStream;
    
    av_register_all();
    avcodec_register_all();
    
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
    duration = ifmtCtx->duration;
    
    if (audioStream && audioStream->duration != AV_NOPTS_VALUE) {
        double divideFactor;
        divideFactor = (double)1 / av_q2d(audioStream->time_base);
        durationOfAudio = (double)audioStream->duration / divideFactor;
    }
    
    inAudioParam.channels = audioCtx->channels;
    inAudioParam.sampleRate = audioCtx->sample_rate;
    inAudioParam.sampleFmt = audioCtx->sample_fmt;
    inAudioParam.wantedSamples = audioCtx->frame_size;
    
    audioDectx = new AudioDecoder();
    if (audioDectx == NULL) {
        ret = AV_MALLOC_ERR;
        av_log(NULL, AV_LOG_ERROR, "New audio decodecontext error!\n");
        goto TAR_OUT;
    }
    
    audioDectx->setDecoder(audioCtx);
    
    frame = av_frame_alloc();
    if (frame == NULL) {
        ret = AV_MALLOC_ERR;
        av_log(NULL, AV_LOG_ERROR, "Alloc frame error!\n");
        goto TAR_OUT;
    }
    
    programStat = true;
    
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
        if (frame) {
            av_frame_free(&frame);
        }
    }
    return ret;
}

size_t InFileContext::getAudioSampleSize()
{
    int ret = 0;
    int fillDataSize;
    enum AVSampleFormat sampleFmt;
    int channels;
    int sampleRate;
    
    if (audioDectx) {
        sampleFmt = (enum AVSampleFormat)audioDectx->getSampleFormat();
        channels = audioDectx->getChannels();
        sampleRate = audioDectx->getSampleRate();
        fillDataSize =  av_samples_get_buffer_size(NULL, channels, sampleRate, sampleFmt, 0);
        
        ret = fillDataSize;
    } else {
        ret = AV_NOT_INIT;
    }
    
TAR_OUT:
    
    return ret;
}

int InFileContext::setAudioOutDecodedData(AudioParam_t *param)
{
    if (!checkAudioParamValid(param)) {
        av_log(NULL, AV_LOG_WARNING, "Set param valid!\n");
        return AV_PARM_ERR;
    }
    
    if (audioDectx == NULL) {
        av_log(NULL, AV_LOG_WARNING, "Cannot find audio decoder, please open audio file first\n");
        return AV_PARM_ERR;
    }
    
    memcpy(&outAudioParam, param, sizeof(AudioParam_t));
    
    if (outAudioParam.channels == 1 && outAudioParam.sampleFmt == -1) {
        outAudioParam.sampleFmt = AV_SAMPLE_FMT_S16;
    } else if (outAudioParam.channels > 1 && outAudioParam.sampleFmt == -1) {
        outAudioParam.sampleFmt = AV_SAMPLE_FMT_S16;
    }
    
    if (!isSameParam(&inAudioParam, &outAudioParam)) {
        if (resampler) {
            delete resampler;
            resampler = NULL;
        }
        resampler = new AudioResampler();
        resampler->initAudioResampler(outAudioParam.channels, outAudioParam.sampleFmt, outAudioParam.sampleRate,
                                      inAudioParam.channels, inAudioParam.sampleFmt, inAudioParam.sampleRate);
        needResample = true;
    } else {
        needResample = false;
    }
    
    return 0;
}

int InFileContext::readAudioDecodeData(uint8_t *outData, size_t size)
{
    int ret = 0;
    AVPacket *pkt = NULL;
    int outSize = 0;
    int outSamples = 0;
    int convertSamples;
    int totalReadLen = 0;
    uint8_t *pData;
    uint8_t *pHalfData;
    uint8_t *baseAddr;
    int bytePerSample;
    long delta;
    int outchannels;
    int outFormat;
    int64_t dealTime = 0;
    
    if (audioDectx == NULL || audioIndex < 0) {
        return AV_NOT_INIT;
    }
    
    if (outData == NULL || size <= 0) {
        av_log(NULL, AV_LOG_ERROR, "In data invalid\n");
        return AV_PARM_ERR;
    }
    
    pData = outData;
    pHalfData = pData + size / 2;
    baseAddr = pHalfData;
    
    pkt = av_packet_alloc();
    if (pkt == NULL) {
        ret = AV_MALLOC_ERR;
        goto TAR_OUT;
    }

    av_init_packet(pkt);

    bytePerSample = av_get_bytes_per_sample((AVSampleFormat) outAudioParam.sampleFmt);
    outchannels = outAudioParam.channels;
    outFormat = outAudioParam.sampleFmt;
    
    while (programStat) {
        ret = av_read_frame(ifmtCtx, pkt);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Read exit reason [%s]\n", makeErrorStr(ret));
            goto TAR_OUT;
        }
        
        if (pkt->stream_index != audioIndex) {
            av_packet_unref(pkt);
            av_log(NULL, AV_LOG_INFO,  " stream index %d\n", pkt->stream_index);
            continue;
        }
        
        dealTime = av_rescale_q(pkt->pts, ifmtCtx->streams[pkt->stream_index]->time_base, AV_TIME_BASE_Q);

        ret = audioDectx->pushPacket(pkt);
        if (ret < 0 && ret != AVERROR(EAGAIN)) {
            av_log(NULL, AV_LOG_ERROR, "Send a packet to decodec error![%s]\n", makeErrorStr(ret));
            goto TAR_OUT;
        }
        av_packet_unref(pkt);
        ret = audioDectx->popFrame(frame);
        if (ret < 0 && ret != AVERROR(EAGAIN)) {
            av_frame_unref(frame);
            av_log(NULL, AV_LOG_ERROR, "Decode frame error![%s]\n", makeErrorStr(ret));
            goto TAR_OUT;
        } else if (ret == AVERROR(EAGAIN)) {
            av_frame_unref(frame);
            continue;
        }
        if (size >= frame->linesize[0]) {
            outSize = av_samples_get_buffer_size(NULL, outchannels, frame->nb_samples, (AVSampleFormat)outFormat, 0);
            if (needResample) {
                outSamples = outSize / (bytePerSample * outchannels);
                convertSamples = resampler->audioConvert(&audioBuf, outSize, (const uint8_t **)frame->data, frame->nb_samples);
                if (convertSamples < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Audio convert error!\n");
                    goto TAR_OUT;
                }
                outSize = convertSamples * outAudioParam.channels * bytePerSample;
            } else {
                audioBuf = frame->data[0];
            }

            if (av_sample_fmt_is_planar((AVSampleFormat) outAudioParam.sampleFmt)) {
                memcpy(pData, frame->data[0], frame->linesize[0]);
                pData += frame->linesize[0];
                memcpy(pHalfData, frame->data[1], frame->linesize[0]);
                pHalfData += frame->linesize[0];
            } else {
                memcpy(pData, audioBuf, outSize);
                pData += outSize;
                pHalfData = pData;
            }
            totalReadLen += outSize;
        } else {
            ret = AVERROR(ENOMEM);
            av_frame_unref(frame);
            goto TAR_OUT;
        }
        
        av_frame_unref(frame);
        if (totalReadLen > size - outSize) {
            av_log(NULL, AV_LOG_DEBUG, "Mem is full read data %d\n", totalReadLen);
            break;
        }
    }

TAR_OUT:
    
    delta = baseAddr - pData;
    if (ret > 0 && delta > 0) {
        memmove(pData, baseAddr, totalReadLen/2);
    }

    if (pkt) {
        av_packet_free(&pkt);
    }
    
    return totalReadLen <= 0 ? ret : totalReadLen;
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

size_t InFileContext::getFileTotalDecodedDataLen()
{
    return durationOfAudio * inAudioParam.sampleRate * inAudioParam.channels * av_get_bytes_per_sample((AVSampleFormat)inAudioParam.sampleFmt);;
}

size_t InFileContext::getOutFileTotalDecodedDataLen()
{
    return durationOfAudio * outAudioParam.sampleRate * outAudioParam.channels * av_get_bytes_per_sample((AVSampleFormat)outAudioParam.sampleFmt);;
}

int InFileContext::close()
{
    int ret = 0;
    
    if (audioDectx) {
        audioDectx->close();
        delete audioDectx;
        audioDectx = NULL;
    }
    
    if (frame) {
        av_frame_free(&frame);
    }
    
    if (ifmtCtx) {
        avformat_close_input(&ifmtCtx);
    }
    
    if (resampler) {
        delete resampler;
        resampler = NULL;
    }
    
    if (audioBuf) {
        av_freep(&audioBuf);
    }
    return ret;
}
