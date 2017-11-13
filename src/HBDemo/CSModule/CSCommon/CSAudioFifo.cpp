//
//  CSAudioFifo.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/14.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSAudioFifo.h"

/** 音频管道 */
int initialFifo(AVAudioFifo **fifo, enum AVSampleFormat fmt, int channels, int size) {
    if (fifo == NULL)
        return HB_ERROR;
    
    *fifo = av_audio_fifo_alloc(fmt, channels, size);
    if (!(*fifo)) {
        LOGE("Alloc audio fifo err!\n");
        return HB_ERROR;
    }
    
    return HB_OK;
}

int pushSamplesToFifo(AVAudioFifo *fifo,
                     uint8_t **inputSamples,
                     const int frame_size) {
    int ret = av_audio_fifo_write(fifo, (void **)inputSamples, frame_size);
    if (ret < frame_size) {
        LOGE("Audio fifo write data err![%d]\n", ret);
        return HB_ERROR;
    }
    
    return HB_OK;
}

int initialAudioFrameWidthParams(AVFrame **frame,
                    AudioParams *parm,
                    int samples) {
    if (!frame) {
        LOGE("initial ouput frame error !");
        return HB_ERROR;
    }
    
    AVFrame *pTempFrame = *frame;
    if (pTempFrame == NULL) {
        pTempFrame = av_frame_alloc();
        if (!pTempFrame) {
            LOGE("Malloc frame failed !");
            return HB_ERROR;
        }
    }
    
    pTempFrame->nb_samples     = samples;
    pTempFrame->format         = parm->sample_fmt;
    pTempFrame->sample_rate    = parm->sample_rate;
    pTempFrame->channels       = parm->channels;
    pTempFrame->channel_layout = av_get_default_channel_layout(parm->channels);
    
    int HBErr = av_frame_get_buffer(pTempFrame, 0);
    if (HBErr < 0) {
        LOGE("Get frame buffer error![%s]\n", makeErrorStr(HBErr));
        return HB_ERROR;
    }
    
    *frame = pTempFrame;
    return HB_OK;
}
