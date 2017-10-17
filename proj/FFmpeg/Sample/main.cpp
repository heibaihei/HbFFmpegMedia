//
//  main.c
//  Sample
//
//  Created by zj-db0519 on 2017/4/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include "CSFiFoQueue.h"
#include "CSLog.h"
#include "CSCommon.h"
#include "HBAudio.h"
#include "CSVideo.h"
#include "HBExample.h"
#include "HBVideoConcat.h"
#include "HBVideoChangeBgmMusic.h"
#include "FFmpegModule/Swscale/FFmpegSwscale.h"

#include "Picture/CSPicture.h"

#include <unistd.h>

#define PROJ_ROOT_PATH "/Users/zj-db0519/work/code/github/HbFFmpegMedia/resource"

int gMtmvLogLevel = LOG_LEVEL_DEBUG;

int PictureCSpictureDemo();
int main(int argc, const char * argv[]) {
    
//    HBFFmpegSwscale();
//    HBPictureEncoder(argc, argv);
    
/** Examples */
//    examples_filtering_video(argc, argv);

//    HBPickPictureFromVideo();
    
/** ################################# >>> Audio */
//    { /** Audio decode */
//        char *strInputAudioFile = (char *)PROJ_ROOT_PATH"/src/HBDemo/audio/AudioDecoder/skycity1.mp3";
//        char *strOutputAudioFile = (char *)PROJ_ROOT_PATH"/src/HBDemo/audio/AudioDecoder/skycity1_output.pcm";
//        AudioParams outputAudioParams;
//        outputAudioParams.fmt = AV_SAMPLE_FMT_FLTP;
//        outputAudioParams.channel_layout = AV_CH_LAYOUT_MONO;
//        outputAudioParams.channels = av_get_channel_layout_nb_channels(outputAudioParams.channel_layout);
//        outputAudioParams.freq = 44100;
//        outputAudioParams.freq = 64000;
//        
//        HBAudioDecoder(strInputAudioFile, strOutputAudioFile, AUDIO_DATA_TYPE_OF_PCM, &outputAudioParams);
//    }
    
//    { /** Audio decode */
//        char *strInputAudioFile = (char *)PROJ_ROOT_PATH"/src/HBDemo/audio/AudioEncoder/skycity1_output.pcm";
//        char *strOutputAudioFile = (char *)PROJ_ROOT_PATH"/src/HBDemo/audio/AudioEncoder/skycity1.aac";
//        AudioParams outputAudioParams;
//        outputAudioParams.sample_fmt = AV_SAMPLE_FMT_FLTP;//
//        outputAudioParams.channels = 1;
//        outputAudioParams.channel_layout = av_get_default_channel_layout(outputAudioParams.channels);
//        outputAudioParams.sample_rate = 44100;
//        outputAudioParams.mbitRate = 64000;
//        
//        HBAudioEncoder(strInputAudioFile, strOutputAudioFile, AUDIO_DATA_TYPE_OF_AAC, &outputAudioParams);
//    }
/** ################################# <<< */
    
//    PictureCSpictureDemo();
    
//    {
//    /**
//     *  qt-faststart
//     */
//        int tmpArgc = 3;
//        const char *tmpArgv[] = {argv[1], \
//            (char *)PROJ_ROOT_PATH"/video/100.mp4", \
//            (char *)PROJ_ROOT_PATH"/video/100WithFaststart.mp4"};
//        TVideoFasterTranfor(tmpArgc, tmpArgv);
//    }
//    {   /** 媒体拼接接口 */
//        demo_video_concat_with_same_codec();
//        
//    }
//    {   /** 音乐和视频拼接 */
//        demo_video_change_bgm_music();
//    }
//    {
//        /**
//         *  Example
//         */
//        int tmpArgc = 2;
//        const char *tmpArgv[] = {argv[1], \
//            (char *)PROJ_ROOT_PATH"/video/100.mp4"};
//        demo_avio_reading(tmpArgc, tmpArgv);
//    }
//    {/** Example */
//        int iArgcNum = 2;
//        char *tmpArgv[] = {nullptr, \
//                        (char *)"/Users/zj-db0519/work/code/github/HbFFmpegMedia/resource/video/concat/100.mp4"};
//        examples_filtering_video(iArgcNum, (char **)tmpArgv);
//    }
//    {
//        int iArgcNum = 2;
//        char *tmpArgv[] = {nullptr, \
//            (char *)"/Users/zj-db0519/work/code/github/HbFFmpegMedia/resource/video/concat/100.mp4"};
//        demo_avio_reading(iArgcNum, (const char **)tmpArgv);
//    }
    
    {
        int iArgcNum = 3;
        char *tmpArgv[] = {nullptr, \
            (char *)"/Users/zj-db0519/work/code/github/HbFFmpegMedia/resource/video/concat/100.mp4", \
            (char *)"/Users/zj-db0519/work/code/github/HbFFmpegMedia/resource/video/concat/100_transcode_main.flv"
        };
        demo_transcod_main(iArgcNum, (char **)tmpArgv);
    }
    
    return 0;
}

int PictureCSpictureDemo()
{
    HBMedia::CSPicture objPicture;
    objPicture.setSrcPicDataType(MD_TYPE_RAW_BY_FILE);
    objPicture.setInputPicMediaFile((char *)(PROJ_ROOT_PATH"/Picture/encoder/1080_1080_JYUV420P.yuv"));
    ImageParams srcPictureParam = { CS_PIX_FMT_YUVJ420P, 1080, 1080, NULL, 1 };
    objPicture.setSrcPictureParam(&srcPictureParam);
    
    objPicture.setTrgPicDataType(MD_TYPE_COMPRESS);
    objPicture.setOutputPicMediaFile((char *)(PROJ_ROOT_PATH"/Picture/encoder/1080_1080_JYUV420P_HB_encoder.jpg"));
    ImageParams targetPictureParam = { CS_PIX_FMT_YUVJ420P, 1080, 1080, (char *)"mjpeg", 1 };
    objPicture.setTrgPictureParam(&targetPictureParam);
    
    objPicture.prepare();
    
    int      HbErr = HB_OK;
    while (HbErr == HB_OK) {
        uint8_t *pictureData = NULL;
        int      pictureDataSizes = 0;
        
        HbErr = objPicture.receiveImageData(&pictureData, &pictureDataSizes);
        switch (HbErr) {
            case HB_OK:
                break;
            case HB_EOF:
                LOGW("Picture reach raw data EOF !");
                goto ENCODE_LOOP_END_LABEL;
            default:
                LOGE("Picture get raw data failed <%d> !", HbErr);
                goto ENCODE_LOOP_END_LABEL;
        }
        
        HbErr = objPicture.sendImageData(&pictureData, &pictureDataSizes);
        if (HbErr != HB_OK) {
            LOGE("Picture encode exit !");
            break;
        }
    }
    
ENCODE_LOOP_END_LABEL:
    objPicture.dispose();
    
    return 0;
}





























