//
//  AudioEncoder.hpp
//  CombineVideo
//
//  Created by Adiya on 13/05/2017.
//  Copyright © 2017 meitu. All rights reserved.
//

#ifndef AudioDecoder_hpp
#define AudioDecoder_hpp
#include "Common.h"
#include <stdio.h>

class AudioDecoder {
public:
    AudioDecoder();
    ~AudioDecoder();
    int setDecoder(AVCodecContext *audioDeCtx);
    int getChannels();
    int getSampleRate();
    int64_t getChannelsLayout();
    int pushPacket(const AVPacket *pkt);
    int popFrame(AVFrame *frame);
    void close();
private:
    AVCodecContext *audioDeCtx;
    int channels;
    int samplerate;
    int64_t channelsLayout;
};

#endif /* AudioEncoder_hpp */
