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
    /*
     * @func 设置内部解码器
     * @arg audioDeCtx 解码控制上下文
     * @return 0为成功
     */
    int setDecoder(AVCodecContext *audioDeCtx);
    /*
     * @func 获取音频通道数
     * @arg void
     * @return 通道数
     */
    int getChannels();
    int getSampleRate();
    int getSampleFormat();
    int64_t getChannelsLayout();
    int pushPacket(const AVPacket *pkt);
    int popFrame(AVFrame *frame);
    void close();
private:
    AVCodecContext *audioDeCtx;
    int channels;
    int samplerate;
    int sampleFormat;
    int64_t channelsLayout;
};

#endif /* AudioEncoder_hpp */
