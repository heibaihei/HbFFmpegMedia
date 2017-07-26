#ifndef  _HBAUDIO_H_
#define  _HBAUDIO_H_

#ifdef __cplusplus
extern "C" {
#endif
    
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libavutil/samplefmt.h"
#include "libavutil/error.h"
    
#ifdef __cplusplus
};
#endif

#include "HBSampleDefine.h"

#ifndef makeErrorStr
static char errorStr[AV_ERROR_MAX_STRING_SIZE];
#define makeErrorStr(errorCode) av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, errorCode)
#endif

typedef enum _AudioDataType {
    AUDIO_DATA_TYPE_OF_UNKNOWN = -1,
    AUDIO_DATA_TYPE_OF_PCM = 0,
    AUDIO_DATA_TYPE_OF_AAC = 1,
    AUDIO_DATA_TYPE_OF_MAX = 2,
} AudioDataType;

typedef struct _AudioParams {
    int freq;
    int channels;
    int64_t channel_layout;
    AVSampleFormat fmt;
    int frame_size;
    int bytes_per_sec;
    long mbitRate;
} AudioParams;


#include "HBCommon.h"

#include "AudioDecoder/HBAudioDecoder.h"
#include "AudioEncoder/AudioEncoder.h"

#endif
