//
//  InFileContext.hpp
//  audioDecode
//
//  Created by Adiya on 17/05/2017.
//  Copyright © 2017 鹰视工作室. All rights reserved.
//

#ifndef InFileContext_hpp
#define InFileContext_hpp

#include "AudioDecoder.hpp"

#include "libavformat/avformat.h"
#include <stdio.h>

class InFileContext {
public:
    InFileContext();
    ~InFileContext();
    int open(const char *filename);
    int readPacket(AVPacket *pkt);
    int getAudioIndex();
    int getVideoIndex();
    int close();
    double getDuration();
    AVRational getTimeBase();
    AudioDecoder *getAudioDecoder();

private:
    AVFormatContext *ifmtCtx;
    AudioDecoder *audioDectx;
    
    int audioIndex;
    int videoIndex;
    double durationOfAudio;
};

#endif /* InFileContext_hpp */
