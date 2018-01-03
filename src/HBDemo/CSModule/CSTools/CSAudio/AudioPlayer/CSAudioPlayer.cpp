//
//  CSAudioPlayer.cpp
//  Sample
//
//  Created by zj-db0519 on 2018/1/3.
//  Copyright © 2018年 meitu. All rights reserved.
//

#include "CSAudioPlayer.h"
#include <SDL_audio.h>

#include "CSAudioRoundBuf.h"

namespace HBMedia {

#define ADJUST_VOLUME(s, v)   (s *= v)

static void AUDIO_AudioVolume(uint8_t * src, uint32_t len, float volume)
{
    if (volume < 0 || volume >= 1) {
        return;
    }
    else if (volume ==0)
    {
        memset(src, 0, len);
    }
    else
    {
        len /= 2;
        short* dest = (short*)src;
        while (len--)
        {
            ADJUST_VOLUME(dest[0], volume);
            dest++;
        }
        
    }
}

void CSAudioPlayer::CSAudioCallback(void *opaque, uint8_t *pOutDataBuffer, int outLength) {
    CSAudioPlayer *pAudioPlayer = (CSAudioPlayer*)opaque;
    int iWantLength = outLength;
    u_char *pTmpDataBuffer = pOutDataBuffer;
//    double audio_callback_time = av_gettime_relative() / 1000000.0;
    
    pthread_mutex_lock(&(pAudioPlayer->mAudioDataBufferMutex));
    while (true) {
        if (pAudioPlayer->mAbort)
            break;

        int iReadLength = rbuf_read(pAudioPlayer->mAudioDataRoundBuffer, (u_char*)pTmpDataBuffer, iWantLength);
        pAudioPlayer->mAudioPlayedBytes += iReadLength;
        
        if (iReadLength < iWantLength) {
            iWantLength -= iReadLength;
            pTmpDataBuffer += iReadLength;
            pthread_cond_signal(&(pAudioPlayer->mAudioDataBufferCond));
            pthread_cond_wait(&(pAudioPlayer->mAudioDataBufferCond), &(pAudioPlayer->mAudioDataBufferMutex));
        } else {
            iWantLength -= iReadLength;
            AUDIO_AudioVolume(pOutDataBuffer, outLength, pAudioPlayer->mVolume);
            break;
        }
    }
    pthread_mutex_unlock(&(pAudioPlayer->mAudioDataBufferMutex));
    pthread_cond_signal(&(pAudioPlayer->mAudioDataBufferCond));
}

CSAudioPlayer::CSAudioPlayer() {
    mAudioDataRoundBufferSize = 0;
    mAudioDataRoundBuffer = nullptr;
    mAbort = false;
    pthread_cond_init(&mAudioDataBufferCond, NULL);
    pthread_mutex_init(&mAudioDataBufferMutex, NULL);
}

CSAudioPlayer::~CSAudioPlayer() {
    
}
    
int CSAudioPlayer::prepare() {
    mAbort = false;
    if (_Open() != HB_OK) {
        LOGE("Audio player open failed !");
        return HB_ERROR;
    }
    mTmpAudioDataBufferSize = 0;
    mTmpAudioDataBuffer = nullptr;
    mVolume = 1.0f;
    mAudioPlayedBytes = 0;
    mAudioDataRoundBuffer = rbuf_create(mAudioDataRoundBufferSize << 1);
    return HB_OK;
}

int CSAudioPlayer::sendFrame(AVFrame *pFrame) {

    int iTmpAudioDataBufferSize = av_samples_get_buffer_size(NULL, av_frame_get_channels(pFrame), pFrame->nb_samples, (enum AVSampleFormat)pFrame->format, 1);
    av_fast_malloc(&mTmpAudioDataBuffer, &mTmpAudioDataBufferSize, iTmpAudioDataBufferSize);
    if (!mTmpAudioDataBuffer) {
        LOGE("Audio player fast alloc data buffer failed !");
        return HB_ERROR;
    }
    
    int iWantLength = iTmpAudioDataBufferSize;
    u_char *pTmpDataBuffer = mTmpAudioDataBuffer;
    pthread_mutex_lock(&mAudioDataBufferMutex);
    while (!mAbort) {
        int iWriteLength = rbuf_write(mAudioDataRoundBuffer, pTmpDataBuffer, iWantLength);
        if (iWriteLength < iWantLength) {
            pTmpDataBuffer += iWriteLength;
            iWantLength -= iWriteLength;
            pthread_cond_signal(&mAudioDataBufferCond);
            pthread_cond_wait(&mAudioDataBufferCond, &mAudioDataBufferMutex);
        } else {
            iWantLength -= iWriteLength;
            break;
        }
    }
    pthread_mutex_unlock(&mAudioDataBufferMutex);
    pthread_cond_signal(&mAudioDataBufferCond);
    return HB_OK;
}

int CSAudioPlayer::_Open() {
    SDL_AudioSpec wanted_spec, spec;
    const char *env;
    static const int next_nb_channels[] = {0, 0, 1, 6, 2, 6, 4, 6};
    static const int next_sample_rates[] = {0, 44100, 48000, 96000, 192000};
    int next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;
    
    env = SDL_getenv("SDL_AUDIO_CHANNELS");
    if (env) {
        mTargetAudioParams.channels = atoi(env);
        mTargetAudioParams.channel_layout = av_get_default_channel_layout(mTargetAudioParams.channels);
    }
    if (!mTargetAudioParams.channel_layout || mTargetAudioParams.channels != av_get_channel_layout_nb_channels(mTargetAudioParams.channel_layout)) {
        mTargetAudioParams.channel_layout = av_get_default_channel_layout(mTargetAudioParams.channels);
        mTargetAudioParams.channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
    }
    mTargetAudioParams.channels = av_get_channel_layout_nb_channels(mTargetAudioParams.channel_layout);
    wanted_spec.channels = mTargetAudioParams.channels;
    wanted_spec.freq = mTargetAudioParams.sample_rate;
    if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
        av_log(NULL, AV_LOG_ERROR, "Invalid sample rate or channel count!\n");
        return -1;
    }
    while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq)
        next_sample_rate_idx--;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.silence = 0;
    wanted_spec.samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
    wanted_spec.callback = CSAudioCallback;
    wanted_spec.userdata = this;
    while (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
        av_log(NULL, AV_LOG_WARNING, "SDL_OpenAudio (%d channels, %d Hz): %s\n",
               wanted_spec.channels, wanted_spec.freq, SDL_GetError());
        wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
        if (!wanted_spec.channels) {
            wanted_spec.freq = next_sample_rates[next_sample_rate_idx--];
            wanted_spec.channels = mTargetAudioParams.channels;
            if (!wanted_spec.freq) {
                av_log(NULL, AV_LOG_ERROR,
                       "No more combinations to try, audio open failed\n");
                return -1;
            }
        }
        mTargetAudioParams.channel_layout = av_get_default_channel_layout(wanted_spec.channels);
    }
    if (spec.format != AUDIO_S16SYS) {
        av_log(NULL, AV_LOG_ERROR,
               "SDL advised audio format %d is not supported!\n", spec.format);
        return -1;
    }
    if (spec.channels != wanted_spec.channels) {
        mTargetAudioParams.channel_layout = av_get_default_channel_layout(spec.channels);
        if (!mTargetAudioParams.channel_layout) {
            av_log(NULL, AV_LOG_ERROR,
                   "SDL advised channel count %d is not supported!\n", spec.channels);
            return -1;
        }
    }
    
    mTargetAudioParams.pri_sample_fmt = getAudioOuterFormat(AV_SAMPLE_FMT_S16);
    mTargetAudioParams.sample_rate = spec.freq;
    mTargetAudioParams.channel_layout = mTargetAudioParams.channel_layout;
    mTargetAudioParams.channels =  spec.channels;
//    audio_hw_params->frame_size = av_samples_get_buffer_size(NULL, audio_hw_params->channels, 1, audio_hw_params->fmt, 1);
//    audio_hw_params->bytes_per_sec = av_samples_get_buffer_size(NULL, audio_hw_params->channels, audio_hw_params->freq, audio_hw_params->fmt, 1);
//    if (audio_hw_params->bytes_per_sec <= 0 || audio_hw_params->frame_size <= 0) {
//        av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size failed\n");
//        return -1;
//    }
    mAudioDataRoundBufferSize = spec.size;
    return spec.size;
}
    
}