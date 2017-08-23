#ifndef  _HBAUDIO_H_
#define  _HBAUDIO_H_


#include "HBCommon.h"
#include "HBSampleDefine.h"

typedef enum _AudioDataType {
    AUDIO_DATA_TYPE_OF_UNKNOWN = -1,
    AUDIO_DATA_TYPE_OF_PCM = 0,
    AUDIO_DATA_TYPE_OF_AAC = 1,
    AUDIO_DATA_TYPE_OF_MAX = 2,
} AudioDataType;

typedef struct _AudioParams {
    int sample_rate;
    int channels;
    int64_t channel_layout;
    AVSampleFormat sample_fmt;
    int frame_size;
    int bytes_per_sec;
    long mbitRate;
} AudioParams;

#include "HBCommon.h"

#include "AudioDecoder/AudioDecoder.h"
#include "AudioEncoder/AudioEncoder.h"
#include "HBPacketQueue.h"

#include "CSAudioResample.h"

#endif
