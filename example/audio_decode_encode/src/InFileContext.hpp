//
//  InFileContext.hpp
//  audioDecode
//
//  Created by Adiya on 17/05/2017.
//  Copyright © 2017 meitu. All rights reserved.
//

#ifndef InFileContext_hpp
#define InFileContext_hpp

#include "AudioDecoder.hpp"
#include "AudioResampler.hpp"
#include "Common.h"
#include <stdio.h>

class InFileContext {
public:
    InFileContext();
    ~InFileContext();
    int open(const char *filename);
    int readPacket(AVPacket *pkt);
    size_t getAudioSampleSize();
    int readAudioDecodeData(uint8_t *outData, size_t size);
    
    int setAudioOutDecodedData(AudioParam_t *param);
    int setAudioOutDecodedData(int channels, int sampleRate, int sampleFmt);
    int setAudioBuf(uint8_t *data, int &size);
    int getAudioIndex();
    int getVideoIndex();
    int close();
    
    size_t getFileTotalDecodedDataLen();
    
    size_t getOutFileTotalDecodedDataLen();
    double getDuration();
    AVRational getTimeBase();
    AudioDecoder *getAudioDecoder();

private:
    AVFormatContext *ifmtCtx;
    AudioDecoder *audioDectx;
    AudioResampler *resampler;
    AudioParam_t inAudioParam;
    AudioParam_t outAudioParam;
    uint8_t *audioBuf;
    int audioBufSize;
    AVFrame *frame;
    int64_t duration;
    bool needResample;
    int audioIndex;
    int videoIndex;
    double durationOfAudio;
    bool programStat;
};

#endif /* InFileContext_hpp */
