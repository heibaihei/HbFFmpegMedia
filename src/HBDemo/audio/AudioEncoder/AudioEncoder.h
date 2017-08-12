#ifndef _AUDIO_ENCODER_H_
#define _AUDIO_ENCODER_H_

#include "HBAudio.h"

int HBAudioEncoder(char *strInputFileName, char*strOutputFileName, AudioDataType dataType, AudioParams *outputAudioParams);

#endif
