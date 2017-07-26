//
//  AudioEncoder.hpp
//  CombineVideo
//
//  Created by Adiya on 13/05/2017.
//  Copyright © 2017 meitu. All rights reserved.
//

#ifndef AudioEncoder_hpp
#define AudioEncoder_hpp
#include "Common.h"
#include <stdio.h>
#include "AudioParam.h"

class AudioEncoder {
public:
    AudioEncoder();
    ~AudioEncoder();
    int setEncoder(AVCodecContext *ctx);
    
    int getChannels();
    int getSampleRate();
    int getSampleFormat();
    int64_t getChannelsLayout();
    int pushDecodedData(uint8_t **decodedData, int SampleSize);
    int popEncodedData(uint8_t **encodedData, int &size);
    int pushFrame(const AVFrame *frame);
    int popPacket(AVPacket *pkt);
    void close();
private:
    AVFrame *audioFrame;
    AVAudioFifo *audioFifo;
    AudioParam_t param;
    int sampleSize;
    AVCodecContext *audioEnCtx;
    int channels;
    int samplerate;
    int sampleFormat;
    int64_t channelsLayout;
    int64_t pts;
};

#endif /* AudioEncoder_hpp */
