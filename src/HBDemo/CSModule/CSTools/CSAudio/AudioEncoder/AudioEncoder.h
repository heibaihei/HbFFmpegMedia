#ifndef _AUDIO_ENCODER_H_
#define _AUDIO_ENCODER_H_

#include "CSAudio.h"

/**
 *  将输入的原始音频 PCM 文件输出编码成 AAC 格式输出
 */
int HBAudioEncoder(char *strInputFileName, char*strOutputFileName, AudioDataType dataType, AudioParams *outputAudioParams);

#endif
