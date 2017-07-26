//
//  AudioParam.h
//  audioTranscode
//
//  Created by meitu on 2017/7/19.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef AudioParam_h
#define AudioParam_h

/*
 * 音频参数结构体
 */
typedef struct AudioParam_t {
    int channels;
    int sampleRate;
    int sampleFmt;
    int wantedSamples;
} AudioParam_t;

static bool isSameParam(AudioParam_t *param1, AudioParam_t *param2)
{
    return (param1->channels == param2->channels &&
            param1->sampleFmt == param2->sampleFmt &&
            param1->sampleRate == param2->sampleRate);
}

#endif /* AudioParam_h */
