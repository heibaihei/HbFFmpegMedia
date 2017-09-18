//
//  AudioDecoder.cpp
//  CombineVideo
//
//  Created by Adiya on 13/05/2017.
//  Copyright © 2017 meitu. All rights reserved.
//

#include "Error.h"
#include "AudioDecoder.hpp"

AudioDecoder::AudioDecoder()
{
    audioDeCtx = NULL;
    channels = 0;
    samplerate = NULL;
}

AudioDecoder::~AudioDecoder()
{

}

/*
 * 配置音频的解码器，并将解码器打开
 */
int AudioDecoder::setDecoder(AVCodecContext *ctx)
{
    int ret = -1;
    
    if (ctx == NULL) {
        av_log(NULL, AV_LOG_ERROR, "Codec context is null\n");
        return ret;
    }
    audioDeCtx = ctx;
    channels = ctx->channels;
    samplerate = ctx->sample_rate;
    channelsLayout = ctx->channel_layout;
    
    ret = avcodec_open2(audioDeCtx, audioDeCtx->codec, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Open codec error![%s]\n", makeErrorStr(ret));
        return ret;
    }
    
    return 0;
}

int AudioDecoder::getChannels()
{
    return channels;
}

int AudioDecoder::getSampleRate()
{
    return samplerate;
}

int64_t AudioDecoder::getChannelsLayout()
{
    return channelsLayout;
}
/*
 * 将读取到的数据放入解码队列中
 */

int AudioDecoder::pushPacket(const AVPacket *pkt)
{
    return avcodec_send_packet(audioDeCtx, pkt);
}

/*
 * 输出解码后的数据
 */
int AudioDecoder::popFrame(AVFrame *frame)
{
    int ret = 0;
    
    if (frame == NULL) {
        return AV_MALLOC_ERR;
    }
    
    ret = avcodec_receive_frame(audioDeCtx, frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        return ret;
    } else if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error during decoding\n");
        return AV_DECODE_ERR;
    }
    
    return ret;
}

void AudioDecoder::close()
{
    if (audioDeCtx) {
        avcodec_close(audioDeCtx);
    }
}
