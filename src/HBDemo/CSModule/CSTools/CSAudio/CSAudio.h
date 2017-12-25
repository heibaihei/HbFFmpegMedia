#ifndef  _HBAUDIO_H_
#define  _HBAUDIO_H_


#include "CSCommon.h"
#include "CSDefine.h"

typedef enum _AudioDataType {
    AUDIO_DATA_TYPE_OF_UNKNOWN = -1,
    AUDIO_DATA_TYPE_OF_PCM = 0,
    AUDIO_DATA_TYPE_OF_AAC = 1,
    AUDIO_DATA_TYPE_OF_MAX = 2,
} AudioDataType;

#include "CSCommon.h"

#include "AudioEncoder/AudioEncoder.h"
#include "CSPacketQueue.h"

#include "CSAudioResample.h"

#endif
