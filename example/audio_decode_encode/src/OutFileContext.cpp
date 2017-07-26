//
//  OutFileContext.cpp
//  audioDecode
//
//  Created by Adiya on 17/05/2017.
//  Copyright © 2017 meitu. All rights reserved.
//

#include "OutFileContext.hpp"
#include "Error.h"
#include "Utils.h"


OutFileContext::OutFileContext()
{
    ofmtCtx = NULL;
    audioEnctx = NULL;
    frame = NULL;
    channels = 1;
    sampleRate = 44100;
    sampleFmt = AV_SAMPLE_FMT_S16;
    frame = NULL;
    resampler = NULL;
    outAudiobuf = NULL;
    audioFifo = NULL;
    needResample = false;
    inParam = {1, 44100, -1, 0};
    outParam = {1, 44100, -1, 0};
}

OutFileContext::~OutFileContext()
{
    
}

int initFifo(AVAudioFifo **fifo, enum AVSampleFormat fmt, int channels, int size)
{
    if (fifo == NULL) {
        return AV_PARM_ERR;
    }
    
    *fifo = av_audio_fifo_alloc(fmt, channels, size);
    if (*fifo == NULL) {
        av_log(NULL, AV_LOG_ERROR, "Alloc audio fifo err!\n");
        return AV_MALLOC_ERR;
    }
    
    return 0;
}

int addSamplesToFifo(AVAudioFifo *fifo,
                     uint8_t **inputSamples,
                     const int frame_size)
{
    int ret;
    
    ret = av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo)+frame_size);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Audio fifo realloc err!\n");
        ret = AV_MALLOC_ERR;
        goto TAR_OUT;
    }
    
    ret = av_audio_fifo_write(fifo, (void **)inputSamples, frame_size);
    if (ret < frame_size) {
        av_log(NULL, AV_LOG_ERROR, "Audio fifo write data err![%d]\n", ret);
        goto TAR_OUT;
    }
    
TAR_OUT:
    
    return ret;
}

int OutFileContext::open(const char *filename)
{
    int ret;

    if (ofmtCtx != NULL) {
        avformat_close_input(&ofmtCtx);
        ofmtCtx = NULL;
    }
    
    ret = avformat_alloc_output_context2(&ofmtCtx, NULL, NULL, filename);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Alloc out format context error!\n");
        goto TAR_OUT;
    }
    strncpy(ofmtCtx->filename, filename, strlen(filename));
    
TAR_OUT:
    
    if (ret < 0) {
        if (ofmtCtx) {
            avformat_close_input(&ofmtCtx);
        }
        if (audioEnctx) {
            audioEnctx->close();
            delete audioEnctx;
            audioEnctx = NULL;
        }
        if (frame) {
            av_frame_free(&frame);
        }
    }
    return ret;
}


int OutFileContext::setInAudioFormat(AudioParam_t *param)
{
    if (param == NULL) {
        return AV_PARM_ERR;
    }
    
    memcpy(&inParam, param, sizeof(*param));
    if (inParam.sampleFmt == -1 && inParam.channels == 1) {
        inParam.sampleFmt = AV_SAMPLE_FMT_S16;
    } else if (inParam.sampleFmt == -1 && inParam.channels > 1) {
        inParam.sampleFmt = AV_SAMPLE_FMT_S16P;
    }
    
    return 0;
}

int OutFileContext::setOutAudioFormat(const char *format, AudioParam_t *param)
{
    int ret;
    AVCodec *audioCodec;
    AVStream *audioStream;
    audioCtx = NULL;
    
    if (format == NULL || strlen(format) <= 0 || param == NULL) {
        return AV_PARM_ERR;
    }
    
    if (param->sampleRate <= 0) {
        av_log(NULL, AV_LOG_ERROR, "Audio param is invalid");
        return AV_PARM_ERR;
    }

    memcpy(&outParam, param, sizeof(*param));
    
    if (strncmp(format, "aac", strlen("aac")) == 0) {
        audioCodec = avcodec_find_encoder_by_name("libfdk_aac");
        if (audioCodec == NULL) {
            ret = AV_NOT_FOUND;
            goto TAR_OUT;
        }
    } else if (strncmp(format, "mp3", strlen("mp3")) == 0) {
        audioCodec = avcodec_find_encoder_by_name("libmp3lame");
        if (audioCodec == NULL) {
            ret = AV_NOT_FOUND;
            goto TAR_OUT;
        }
    } else {
        av_log(NULL, AV_LOG_ERROR, "Not support this type media, please set it 'mp3' or 'aac'");
        ret = AV_NOT_SUPPORT;
        goto TAR_OUT;
    }
    
    if (param->channels == 1 && param->sampleFmt <= 0) {
        outParam.sampleFmt = AV_SAMPLE_FMT_S16;
    } else if (param->channels >= 1 && param->sampleFmt <= 0) {
        outParam.sampleFmt = audioCodec->sample_fmts[0];
    }
    
    audioStream = avformat_new_stream(ofmtCtx, NULL);
    if (audioStream == NULL) {
        av_log(NULL, AV_LOG_ERROR, "New audio stream error!\n");
        ret = AV_MALLOC_ERR;
        goto TAR_OUT;
    }
    
    audioCtx = avcodec_alloc_context3(audioCodec);
    if (audioCtx == NULL) {
        ret = AV_MALLOC_ERR;
        goto TAR_OUT;
    }
    if (ofmtCtx->oformat->flags & AVFMT_GLOBALHEADER) {
        audioCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
    audioCtx->codec = audioCodec;
    audioCtx->channels = outParam.channels;
    audioCtx->channel_layout = av_get_default_channel_layout(outParam.channels);
    
    audioCtx->sample_fmt = (enum AVSampleFormat)outParam.sampleFmt;
    audioCtx->sample_rate = outParam.sampleRate;
    audioCtx->codec_id = audioCodec->id;
    audioCtx->codec_type = AVMEDIA_TYPE_AUDIO;
    audioCtx->codec_tag = 0;
    audioCtx->bit_rate = 64000;
    audioStream->time_base.den = audioCtx->sample_rate;
    audioStream->time_base.num = 1;
    
    ret = avcodec_parameters_from_context(audioStream->codecpar, audioCtx);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Copy parameters from context error!\n");
        goto TAR_OUT;
    }
    audioEnctx = new AudioEncoder();
    if (audioEnctx == NULL) {
        ret = AV_MALLOC_ERR;
        goto TAR_OUT;
    }
    ret = audioEnctx->setEncoder(audioCtx);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Set encoder error!\n");
        goto TAR_OUT;
    }
    
    sampleSize = audioCtx->frame_size;
    av_log(NULL, AV_LOG_INFO, "sample %d channel layout %llu sample rate %d channels %d\n", sampleSize, audioCtx->channel_layout, audioCtx->sample_rate, audioCtx->channels);
//    if (frame == NULL) {
//        frame = av_frame_alloc();
//        if (frame == NULL) {
//            av_log(NULL, AV_LOG_ERROR, "Alloc frame error!\n");
//            ret = AV_MALLOC_ERR;
//            goto TAR_OUT;
//        }
//    }
    
//    ret = av_samples_alloc(frame->data, frame->linesize, outParam.channels, sampleSize, (enum AVSampleFormat)outParam.sampleFmt, 0);

    if (!(ofmtCtx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmtCtx->pb, ofmtCtx->filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Could not open output file'%s' reason [%s]", ofmtCtx->filename, makeErrorStr(ret));
            goto TAR_OUT;
        }
    }
    
    ret = avformat_write_header(ofmtCtx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        goto TAR_OUT;
    }
    
    if (!isSameParam(&inParam, &outParam)) {
        if (resampler) {
            delete resampler;
        }
        resampler = new AudioResampler();
        ret = resampler->initAudioResampler(outParam.channels, outParam.sampleFmt, \
                                            outParam.sampleRate, inParam.channels, \
                                            inParam.sampleFmt, inParam.sampleRate);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Init audio resampler error!\n");
            goto TAR_OUT;
        }
    }

    ret = initFifo(&audioFifo, (enum AVSampleFormat)inParam.sampleFmt, inParam.channels, 1);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Init audio fifo error\n");
        goto TAR_OUT;
    }
    
    av_log(NULL, AV_LOG_ERROR, "audioFifo %p size %d\n", audioFifo, av_audio_fifo_size(audioFifo));
    
TAR_OUT:
    if (ret < 0) {
        if (audioEnctx) {
            audioEnctx->close();
            delete audioEnctx;
            audioEnctx = NULL;
        }
        if (resampler) {
            delete resampler;
            resampler = NULL;
        }
        avformat_close_input(&ofmtCtx);
    }
    
    return ret;
}

int writeFifoDataToFile()
{
    int ret = 0;
    
    return ret;
}

int OutFileContext::writeAudioDecodeData(uint8_t *inData, int size)
{
    AVPacket *pkt = NULL;
    int ret = 0;
    uint8_t *inputData[8] = {NULL};
    int lineSize[2] = {0, 0};
    int bytePerSample;
    int nbSamples;
    int wantedSamples = 0;
    uint8_t *inBuf = NULL;
    int fifoSize;
    
    if (audioEnctx == NULL) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find audio encoder\n");
        return AV_STAT_ERR;
    }
    
    bytePerSample = av_get_bytes_per_sample((enum AVSampleFormat)outParam.sampleFmt);     /*计算每个sample的大小*/
    nbSamples = size / (bytePerSample * outParam.channels);                               /*统计需要写入samples的个数*/
    
    inBuf = inData;
    
    ret = av_samples_fill_arrays(inputData, lineSize, inBuf, inParam.channels, nbSamples, (enum AVSampleFormat)inParam.sampleFmt, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Fill audio samples to array error!\n");
        goto TAR_OUT;
    }
    
    ret = addSamplesToFifo(audioFifo, inputData, nbSamples);        /*数据写入到fifo中*/
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Add sample to fifo error!\n");
        goto TAR_OUT;
    }
    
    pkt = av_packet_alloc();
    if (pkt == NULL) {
        av_log(NULL, AV_LOG_ERROR, "Alloc packet error!\n");
        ret = AV_MALLOC_ERR;
    }
    
    av_init_packet(pkt);
    av_frame_unref(frame);
    
    if (needResample) {
        wantedSamples = outParam.sampleRate / inParam.sampleRate * sampleSize + 256;
    } else {
        wantedSamples = sampleSize;
    }

    /* 读取fifo中数据到文件 */
    while ((fifoSize = av_audio_fifo_size(audioFifo)) > 0) {
        wantedSamples = FFMIN(wantedSamples, fifoSize);
        ret = initOutputFrame(&frame, &outParam, wantedSamples);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "[%d]Init out frame error!\n", __LINE__);
            goto TAR_OUT;
        }
        
        ret = av_audio_fifo_read(audioFifo, (void **)frame->data, wantedSamples);
        if (ret < wantedSamples) {
            av_log(NULL, AV_LOG_ERROR, "Read audio fifo error!\n");
            break;
        }
        
        if (needResample) {
            resampler->audioConvert(&outAudiobuf, sampleSize, (const uint8_t **)inBuf, wantedSamples);
        }
        
        ret = audioEnctx->pushFrame(frame);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Send frame to encoder error!\n");
            goto TAR_OUT;
        }
        av_frame_unref(frame);
        ret = audioEnctx->popPacket(pkt);
        if (ret < 0 && ret != AVERROR(EAGAIN)) {
            goto TAR_OUT;
        }
        
        ret = av_interleaved_write_frame(ofmtCtx, pkt);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Interleaved frame error!\n");
            goto TAR_OUT;
        }
        av_packet_unref(pkt);
    }

TAR_OUT:
    
    if (frame) {
        av_frame_unref(frame);
    }
    
    if (pkt) {
        av_packet_free(&pkt);
    }
    
    return ret;
}

void OutFileContext::flushEncoders(void)
{
    int ret = 0;
    AVPacket pkt = {0};
    
    if (audioEnctx == NULL) {
        return ;
    }
    
    ret = audioEnctx->pushFrame(NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Send frame to encoder error!\n");
        goto TAR_OUT;
    }
    
    while (1) {
        pkt.data = NULL;
        pkt.size = 0;
        av_init_packet(&pkt);
        ret = audioEnctx->popPacket(&pkt);
        if (ret < 0 && ret != AVERROR(EAGAIN)) {
            break;
        }
        
        ret = av_interleaved_write_frame(ofmtCtx, &pkt);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Interleaved frame error!\n");
            break;
        }
    }
TAR_OUT:
    
    return;
}

int OutFileContext::close()
{
    int ret;
    
    flushEncoders();
    
    if (audioEnctx) {
        audioEnctx->close();
        delete audioEnctx;
        audioEnctx = NULL;
    }
    if (audioFifo) {
        av_audio_fifo_free(audioFifo);
        audioFifo = NULL;
    }
    
    if (resampler) {
        delete resampler;
        resampler = NULL;
    }
    
    if (ofmtCtx) {
        ret = av_write_trailer(ofmtCtx);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Write file trailer error\n");
        }
        avformat_close_input(&ofmtCtx);
    }
    
    if (frame) {
        av_frame_free(&frame);
    }
    
    return 0;
}
