//
//  CSAudioFifo.h
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/14.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSAudioFifo_h
#define CSAudioFifo_h

#include <stdio.h>
#include "CSUtil.h"
#include "CSDefine.h"
#include "CSLog.h"

int initialFifo(AVAudioFifo **fifo, enum AVSampleFormat fmt, int channels, int size);


int pushSamplesToFifo(AVAudioFifo *fifo,
                     uint8_t **inputSamples,
                     const int frame_size);

int initialAudioFrameWidthParams(AVFrame **frame,
                    AudioParams *parm,
                    int samples);

#endif /* CSAudioFifo_h */
