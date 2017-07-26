//
//  main.cpp
//  audioTranscode
//
//  Created by meitu on 2017/7/17.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include <iostream>
#include "InFileContext.hpp"
#include "OutFileContext.hpp"

#include <sys/time.h>
#define _DEBUG_PRINT printf

#define SUFFIX "aac"
#define DEBUG_MODEL 0

int decode_audio(const char *audioFile, AudioParam_t *param, uint8_t **data, int &dataSize)
{
    int ret;
    InFileContext infileCtx;
    int channels;
    int samplerate;
    int samplefmt;
    double duration;
    AudioDecoder *decoder;
    size_t fileSize = 0;
    uint8_t *decodedData = NULL;
    
    ret = infileCtx.open(audioFile);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Open file error!\n");
        goto TAR_OUT;
    }
    
    ret = infileCtx.setAudioOutDecodedData(param);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Set audio out decoder parameter error!\n");
        goto TAR_OUT;
    }
    duration = infileCtx.getDuration();
    decoder = infileCtx.getAudioDecoder();
    channels = decoder->getChannels();
    samplerate = decoder->getSampleRate();
    samplefmt = decoder->getSampleFormat();
    fileSize = infileCtx.getOutFileTotalDecodedDataLen();
    _DEBUG_PRINT("Origin file duration : %lf channels : %d samplerate %d samplefmt %d fileSize %zu\n", \
                 duration, channels, samplerate, samplefmt, fileSize);
    
    decodedData = (uint8_t *)malloc(fileSize);
    if (decodedData == NULL) {
        ret = -1;
        av_log(NULL, AV_LOG_ERROR, "Malloc %zu decoded data error!\n", fileSize);
        goto TAR_OUT;
    }
    
    ret = infileCtx.readAudioDecodeData(decodedData, fileSize);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Read decoded data error!\n");
        goto TAR_OUT;
    }
    
    *data = decodedData;
    dataSize = ret;
    
TAR_OUT:
    
    infileCtx.close();
    
    if (ret < 0) {
        if (decodedData) {
            free(decodedData);
            decodedData = NULL;
            *data = NULL;
            dataSize = 0;
        }
    }
    
    return ret;
}

int encode_audio(const char *audioFile, const char *audioCodec, AudioParam_t *param, uint8_t *data, int dataSize)
{
    int ret;
    OutFileContext outfileCtx;
    
    ret = outfileCtx.open(audioFile);
    if (ret < 0) {
        _DEBUG_PRINT("Open audio file %s error!\n", audioFile);
        goto TAR_OUT;
    }
    /* 设置输出控制器的输入数据的类型，让控制器正确处理输入数据 */
    ret = outfileCtx.setInAudioFormat(param);
    if (ret < 0) {
        _DEBUG_PRINT("Set in audio format error!\n");
        goto TAR_OUT;
    }
    
    /* 设置输出的编码格式信息 */
    ret = outfileCtx.setOutAudioFormat(audioCodec, param);
    if (ret < 0) {
        _DEBUG_PRINT("Set out audio format error!\n");
        goto TAR_OUT;
    }
    
    /* 写入解码数据，在内部进行编码 */
    ret = outfileCtx.writeAudioDecodeData(data, dataSize);
    if (ret < 0) {
        _DEBUG_PRINT("Write audio decoded data error!\n");
        goto TAR_OUT;
    }
    
TAR_OUT:
    
    outfileCtx.close();
    
    return ret;
}

int decode_encode_test()
{
    std::cout << "\n\n......@@@@@Transcode@@@@....\n\n";
    int ret;
    uint8_t *audioData = NULL;
    InFileContext infileCtx;
    OutFileContext outfileCtx;
    AudioParam_t param;
    int size;
    struct timeval tv1, tv2;
    std::string suffix = "mp3";
    std::string outFile = "/Users/meitu/Documents/testDir/audio/new1." + suffix;
    std::string inFile = "/Users/meitu/Documents/testDir/audio/FREE闪聊(BPM114).mp3";
    
#if DEBUG_MODEL
    const char *pcmFile = "/Users/meitu/Documents/testDir/audio/new1.pcm";
    FILE *fp;
    
    fp = fopen(pcmFile, "wb");
    if (fp == NULL) {
        _DEBUG_PRINT("Open %s error\n", pcmFile);
        return -1;
    }
#endif
    
    gettimeofday(&tv1, NULL);
    
    param.channels = 1;
    param.sampleRate = 44100;
    param.sampleFmt = -1; // 当不知道设置什么格式，输入-1，默认channels=1时采样为 16bit的short

    ret = decode_audio(inFile.c_str(), &param, &audioData, size);
    if (ret < 0) {
        _DEBUG_PRINT("Decode audio data error!\n");
        goto TAR_OUT;
    }
    
    ret = encode_audio(outFile.c_str(), suffix.c_str(), &param, audioData, size);
    if (ret < 0) {
        _DEBUG_PRINT("Encode data error!\n");
        goto TAR_OUT;
    }
    
    if (audioData) {
        free(audioData);
        audioData = NULL;
    }
    
    gettimeofday(&tv2, NULL);
    
    _DEBUG_PRINT("Use time %ld\n", (tv2.tv_sec - tv1.tv_sec) * 1000000 + tv2.tv_usec - tv1.tv_usec);
    
    return 0;
    
TAR_OUT:

    if (audioData) {
        free(audioData);
        audioData = NULL;
    }

    return ret;
}
#include <unistd.h>

int main(int argc, const char * argv[]) {
    // insert code here...

    int ret = 0;
    
    for (int i=0; i<1000; i++) {
        ret = decode_encode_test();
    }
    
//    while (1) {
//        sleep(1);
//    }
    return ret;
}
