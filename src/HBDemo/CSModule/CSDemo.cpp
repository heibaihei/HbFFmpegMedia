//
//  CSDemo.cpp
//  Sample
//
//  Created by zj-db0519 on 2017/11/13.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSDemo.h"
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>

#include "CSExternAPI.h"

#define SINGLE_SAMPLES  (1024)

static void *_audioReadThread(void *arg);
static void *_videoReadThread(void *arg);

static int _constructTimeline(HBMedia::CSTimeline* pTimeline);

namespace HBMedia {

class MediaListener:public HBMedia::MediaStateListener {
public:
    MediaListener() {};
    ~MediaListener() {};
    void MediaRecordProgressBegan(HBMedia::CSPlayer* recorder)
    {
        LOGD("Media record begin\n");
    }
    
    void MediaRecordProgressChanged(HBMedia::CSPlayer* recorder, int type)
    {
        LOGD("Media inner thread stat change\n");
        switch (type) {
            case MT_AUDIO_ENCODE_START:
                LOGD("Auido thread begin!\n");
                break;
            case MT_AUDIO_ENCODE_STOP:
                LOGD("Audio thread end!\n");
                break;
            case MT_VIDEO_ENCODE_START:
                LOGD("Video encode begin!\n");
                break;
            case MT_VIDEO_ENCODE_STOP:
                LOGD("video encode end!\n");
                break;
            case MT_WRITE_START:
                LOGD("write begin!\n");
                
                break;
            case MT_WRITE_STOP:
                LOGD("write begin!\n");
                break;
            default:
                break;
        }
    }
    
    /**
     *  录制结束
     *
     *  @param editer  MediaRecorder object.
     */
    void MediaRecordProgressEnded(HBMedia::CSPlayer* editer)
    {
        LOGD("Record stop\n");
    }
    
    /**
     *  取消录制消息
     *
     *  @param editer MediaRecorder object
     */
    void MediaRecordProgressCanceled(HBMedia::CSPlayer* editer)
    {
        LOGD("Record canceled\n");
    }
    
};

}
void CSModulePlayerDemo() {
    int ret = 0;
    
    /** 线程信息 */
    HBMedia::MediaListener mediaPlayerlistener;
    pthread_t audioReadThreadId;
    pthread_t videoReadThreadId;
    HBMedia::CSTimeline* pTimeline = nullptr;
    HBMedia::CSPlayer::CSPlayer *pMediaPlayer = new HBMedia::CSPlayer::CSPlayer();
    if (!pMediaPlayer) {
        LOGE("create new media player failed !");
        goto PLAYER_DEMO_EXIT_LABEL;
    }
    
    /** 初始化 timeline */
    pTimeline = new HBMedia::CSTimeline();
    if (!pTimeline) {
        LOGE("create new media timeline failed !");
        goto PLAYER_DEMO_EXIT_LABEL;
    }
    if (_constructTimeline(pTimeline) != HB_OK) {
        LOGE("Construce timeline failed !");
        delete pTimeline;
        goto PLAYER_DEMO_EXIT_LABEL;
    }
    
    pMediaPlayer->setTimeline(pTimeline);
    pMediaPlayer->setStateListener(&mediaPlayerlistener);
    ret = pMediaPlayer->prepare();
    if (ret < 0) {
        LOGE("Prepare error!\n");
        goto PLAYER_DEMO_EXIT_LABEL;
    }
    
    ret = pMediaPlayer->start();
    if (ret < 0) {
        LOGE("start error!\n");
        goto PLAYER_DEMO_EXIT_LABEL;
    }
    
    ret = pthread_create(&audioReadThreadId, NULL, _audioReadThread, pMediaPlayer);
    if (ret < 0) {
        LOGE("Create audio thread error!\n");
        goto PLAYER_DEMO_EXIT_LABEL;
    }
    
    ret = pthread_create(&videoReadThreadId, NULL, _videoReadThread, pMediaPlayer);
    if (ret < 0) {
        LOGE("Create audio thread error!\n");
        goto PLAYER_DEMO_EXIT_LABEL;
    }
    
    pthread_join(audioReadThreadId, NULL);
    pthread_join(videoReadThreadId, NULL);
    
PLAYER_DEMO_EXIT_LABEL:
    if (pMediaPlayer)
        pMediaPlayer->stop();
    SAFE_DELETE(pMediaPlayer);
    SAFE_DELETE(pTimeline);
}

static void *_audioReadThread(void *arg)
{
    int ret = HB_OK;
    FILE *pPcmFileHandle = nullptr;
    const char *pInputPcmFile = CS_MODULE_RESOURCE_ROOT_PATH"/100_audio_44100_s16le.pcm";
    bool bStartFlag = false;
    struct timeval start, end;
    uint8_t *pDecodedBuffer = nullptr;
    int64_t audioTimestamp = 0;
    HBMedia::CSPlayer::CSPlayer *pMediaPlayer = (HBMedia::CSPlayer::CSPlayer *)arg;
    
    pPcmFileHandle = fopen(pInputPcmFile, "r");
    if (!pPcmFileHandle)
        LOGE("[Test] ===> Open pcm input file %s error!\n", pInputPcmFile);
    else
        bStartFlag = true;
    
    pDecodedBuffer = (uint8_t *)malloc(SINGLE_SAMPLES);
    if (!pDecodedBuffer) {
        LOGE("[Test] ===> Malloc decode buffer failed !");
        goto AUDIO_READ_THREAD_EXIT_LABEL;
    }
    
    LOGI("[Test] ===> Create audio test thread !");
    while (bStartFlag) {
        gettimeofday(&start, NULL);
        ret = fread((void *)pDecodedBuffer, SINGLE_SAMPLES, 1, pPcmFileHandle);
        if (ret <= 0) {
            LOGE("[Test] ===> Read pcm data from input file error !");
            break;
        }
        
        ret = pMediaPlayer->writeExternData(pDecodedBuffer, SINGLE_SAMPLES, 1, audioTimestamp);
        if (ret < 0) {
            LOGE("[Test] ===> Audio Write data error![%d]\n", ret);
        }
        
        usleep(1500);
        gettimeofday(&end, NULL);
        
        audioTimestamp += 15;
    }
    LOGI("[Test] ===> Audio test thread exit !");
    
AUDIO_READ_THREAD_EXIT_LABEL:
    if (pDecodedBuffer)
        free(pDecodedBuffer);
    
    if (pPcmFileHandle)
        fclose(pPcmFileHandle);
    
    return NULL;
}

static void *_videoReadThread(void *arg)
{
    int ret = HB_OK;
    FILE *pInputYuvHandle = nullptr;
    const char *pInputYuvFile = CS_MODULE_RESOURCE_ROOT_PATH"/100_video_480_480_420p.yuv";
    long iDecodedBufferLen = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, 480, 480, 0);
    int64_t videoTimestamp = 0;
    uint8_t *pDecodedBuffer = nullptr;
    struct timeval start, end;
    bool bStartFlag = false;
    HBMedia::CSPlayer::CSPlayer *pMediaPlayer = (HBMedia::CSPlayer::CSPlayer *)arg;
    pInputYuvHandle = fopen(pInputYuvFile, "r");
    if (pInputYuvHandle == NULL)
        LOGE("[Test] ===> Open yuv input file %s error!\n", pInputYuvFile);
    else
        bStartFlag = true;
    
    pDecodedBuffer = (uint8_t *)malloc(iDecodedBufferLen);
    if (pDecodedBuffer == NULL) {
        LOGE("Malloc decoded data error!\n");
        goto VIDEO_READ_THREAD_EXIT_LABEL;
    }
    
    LOGI("[Test] ===> Create Video test thread !");
    while (bStartFlag) {
        gettimeofday(&start, NULL);
        ret = fread(pDecodedBuffer, 1, (size_t)iDecodedBufferLen, pInputYuvHandle);
        if (ret <= 0) {
            LOGE("[Test] ===> Read yuv file error!\n");
            break;
        }
        usleep(2000);
        ret = pMediaPlayer->writeExternData(pDecodedBuffer, iDecodedBufferLen, 0, videoTimestamp);
        if (ret < 0) {
            LOGE("[Test] ===> Video Write data error![%d]\n", ret);
        }
        gettimeofday(&end, NULL);
        videoTimestamp += 33.3;
    }
    LOGI("[Test] ===> Video test thread exit !");
    
VIDEO_READ_THREAD_EXIT_LABEL:
    if (pDecodedBuffer)
        free(pDecodedBuffer);
    
    if (pInputYuvHandle)
        fclose(pInputYuvHandle);
    
    return NULL;
}

static int _constructTimeline(HBMedia::CSTimeline* pTimeline) {
    char *pOutputMediaFile = (char *)(CS_MODULE_RESOURCE_ROOT_PATH"/100_output.mp4");
    int inWidth = 480, inHeight = 480;
    int outWidth = 480, outHeight = 480;
    
    /** 以输入的左上角坐标为坐标原点 */
    int cropX = 0, cropY = 0;
    int cropWidth = outWidth, cropHeight = outHeight;
    
    /** 输出媒体参数 */
    VideoRorate ouputImageRotate = MT_Rotate0;
    int outputChannels = 2;
    int outputSampleRate = 48000;
    AUDIO_SAMPLE_FORMAT outputSampleFmt = CS_SAMPLE_FMT_FLTP;
    IMAGE_PIX_FORMAT outputImagePixFormat = CS_PIX_FMT_YUV420P;
    
    /** 输入媒体参数 */
    int inputChannels = 2;
    int inputSampleRate = 44100;
    float crf = 58.0f;
    AUDIO_SAMPLE_FORMAT inputSampleFmt = CS_SAMPLE_FMT_S16;
    IMAGE_PIX_FORMAT inputImagePixFormat = CS_PIX_FMT_YUV420P;
    
    ImageParams inputImageParam;
    inputImageParam.mWidth = inWidth;
    inputImageParam.mHeight = inHeight;
    inputImageParam.mPixFmt = inputImagePixFormat;
    inputImageParam.mAlign = 1;
    pTimeline->setSrcImageParam(&inputImageParam);
    
    AudioParams inputAudioParams;
    inputAudioParams.channels = inputChannels;
    inputAudioParams.channel_layout = av_get_default_channel_layout(inputAudioParams.channels);
    inputAudioParams.pri_sample_fmt = inputSampleFmt;
    inputAudioParams.sample_rate = inputSampleRate;
    pTimeline->setSrcAudioParam(&inputAudioParams);
    
    ImageParams outputImageParam;
    outputImageParam.mBitRate = 2000000;
    outputImageParam.mVideoCRF = crf;
    outputImageParam.mWidth = outWidth;
    outputImageParam.mHeight = outHeight;
    outputImageParam.mPixFmt = outputImagePixFormat;
    outputImageParam.mAlign = 1;
    outputImageParam.mRotate = ouputImageRotate;
    pTimeline->setDstImageParam(&outputImageParam);
    
    AudioParams outputAudioParams;
    outputAudioParams.channels = outputChannels;
    outputAudioParams.channel_layout = av_get_default_channel_layout(outputAudioParams.channels);
    outputAudioParams.pri_sample_fmt = outputSampleFmt;
    outputAudioParams.sample_rate = outputSampleRate;
    pTimeline->setTgtAudioParam(&outputAudioParams);
    
    pTimeline->setCropParam(cropX, cropY, cropWidth, cropHeight);
    
    pTimeline->setOutputFile(pOutputMediaFile);
    pTimeline->setGlobalSpeed(1.0f);
    /****************** */
    
    return HB_OK;
}



