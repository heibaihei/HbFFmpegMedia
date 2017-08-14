#ifndef _HBAUDIO_DECODER_H_
#define _HBAUDIO_DECODER_H_

#include "HBAudio.h"

/**
 *  @func HBAudioDecoder 对不同类型的音频数据进行解码
 *  @desc  该函数接口将特定输入的音频文件格式，转换成特定音频格式的音频类型，输出到某个文件
 *  @param strInputFileName 输入音频文件URL
 *  @param strOutputFileName  根据 AudioDataType 类型转换后输出的音频文件
 *  @param dataType 音频类型
 *  @param outputAudioParams 输出音频参数信息
 *  @return >=0 正常； <0  表示发生异常
 */
int HBAudioDecoder(char *strInputFileName, char*strOutputFileName, AudioDataType dataType, AudioParams *outputAudioParams);

#endif
