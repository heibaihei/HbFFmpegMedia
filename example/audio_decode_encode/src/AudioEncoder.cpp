//
//  AudioEncoder.cpp
//  CombineVideo
//
//  Created by Adiya on 13/05/2017.
//  Copyright © 2017 meitu. All rights reserved.
//

#include "Error.h"
#include "AudioEncoder.hpp"
#include "Utils.h"

AudioEncoder::AudioEncoder()
{
    audioEnCtx = NULL;
    channels = 0;
    samplerate = NULL;
    pts = 0;
    audioFrame = NULL;
    audioFifo = NULL;
    sampleSize = 0;
}

AudioEncoder::~AudioEncoder()
{

}

uint8_t **initConvertSample(AVCodecContext *encodecCtx, int frame_size)
{
    uint8_t **inputFrame = NULL;
    int ret;
    
    inputFrame = (uint8_t **)av_calloc(encodecCtx->channels, sizeof(**inputFrame));
    if (inputFrame == NULL) {
        goto TAR_OUT;
    }
    
    ret = av_samples_alloc(inputFrame, NULL, encodecCtx->channels, frame_size,
                           encodecCtx->sample_fmt, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Alloc sample err!\n");
        av_freep(&(inputFrame)[0]);
        free(inputFrame);
        inputFrame = NULL;
    }
    
TAR_OUT:
    
    return inputFrame;
}

int AudioEncoder::pushDecodedData(uint8_t **decodedData, int samples)
{
    int ret = 0;
    
TAR_OUT:
    
    return ret;
}

int AudioEncoder::popEncodedData(uint8_t **outData, int &samples)
{
    int ret = 0;
    int readSize = 0;
    int fifoSize;
    
    fifoSize = av_audio_fifo_size(audioFifo);
    if (fifoSize > sampleSize) {
        readSize = FFMIN(fifoSize, sampleSize);
        ret = initOutputFrame(&audioFrame, &param, readSize);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Init out frame error!\n");
            goto TAR_OUT;
        }
        ret = av_audio_fifo_read(audioFifo, (void**)audioFrame->data, sampleSize);
        if (ret < sampleSize) {
            av_log(NULL, AV_LOG_ERROR, "Read audio fifo err! read size[%d]\n", ret);
            ret = AV_FIFO_ERR;
            goto TAR_OUT;
        }
        samples = sampleSize;
    } else {
        ret = AV_NOT_ENOUGH;
    }
    
TAR_OUT:
    
    return ret;
}

int AudioEncoder::pushFrame(const AVFrame *frame)
{
    int ret = 0;
    
    if (audioEnCtx->frame_number == 0 && frame == NULL) {
        return AV_NOT_INIT;
    }

    ret = avcodec_send_frame(audioEnCtx, frame);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Send frame to encodec error!\n");
        goto TAR_OUT;
    }
    
TAR_OUT:
    
    return ret;
}

int AudioEncoder::popPacket(AVPacket *pkt)
{
    int ret = 0;
    
    ret = avcodec_receive_packet(audioEnCtx, pkt);
    if (ret < 0 && ret != AVERROR(EAGAIN)) {
        av_log(NULL, AV_LOG_ERROR, "Could not encode ![reason %s][%d]\n", makeErrorStr(ret), ret);
        goto TAR_OUT;
    }
    
TAR_OUT:
    
    return ret;
}

/*
 * 配置音频的编码器，并将编码器打开
 */
int AudioEncoder::setEncoder(AVCodecContext *ctx)
{
    int ret = -1;
    
    if (ctx == NULL) {
        av_log(NULL, AV_LOG_ERROR, "Codec context is null\n");
        return ret;
    }
    audioEnCtx = ctx;
    param.channels = ctx->channels;
    param.sampleFmt = ctx->sample_fmt;
    param.wantedSamples = ctx->frame_size;
    param.sampleRate = ctx->sample_rate;
    
    channels = ctx->channels;
    samplerate = ctx->sample_rate;
    channelsLayout = ctx->channel_layout;
    sampleFormat = ctx->sample_fmt;
    
    if (audioEnCtx->codec == NULL) {
        return AV_PARM_ERR;
    }
    
    if (audioEnCtx->codec == NULL) {
        av_log(NULL, AV_LOG_ERROR, "Cannot found codec");
        return AV_PARM_ERR;
    }
    ret = avcodec_open2(audioEnCtx, audioEnCtx->codec, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Open codec error![%s][%d]\n", makeErrorStr(ret), ret);
        return ret;
    }
    
    audioFrame = av_frame_alloc();
    if (audioFrame == NULL) {
        ret = AV_MALLOC_ERR;
        goto TAR_OUT;
    }
    
    sampleSize = audioEnCtx->frame_size;
    
TAR_OUT:
    
    if (ret < 0) {
        if (audioFifo) {
            av_audio_fifo_free(audioFifo);
        }
        if (audioEnCtx) {
            avcodec_close(audioEnCtx);
            avcodec_free_context(&audioEnCtx);
            audioEnCtx = NULL;
        }
        if (audioFrame) {
            av_frame_free(&audioFrame);
        }
    }
    return ret;
}

int AudioEncoder::getChannels()
{
    return channels;
}

int AudioEncoder::getSampleRate()
{
    return samplerate;
}

int64_t AudioEncoder::getChannelsLayout()
{
    return channelsLayout;
}

int AudioEncoder::getSampleFormat()
{
    return sampleFormat;
}

void AudioEncoder::close()
{
    if (audioEnCtx) {
        avcodec_close(audioEnCtx);
        avcodec_free_context(&audioEnCtx);
        audioEnCtx = NULL;
    }
    
    if (audioFrame) {
        av_frame_free(&audioFrame);
    }
    
    if (audioFifo) {
        av_audio_fifo_free(audioFifo);
    }
}
