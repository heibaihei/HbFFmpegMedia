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
#include "CSVideoDemo.h"
#include "HBExample.h"
#include "HBVideoConcat.h"
#include "HBVideoChangeBgmMusic.h"
#include "FFmpegModule/Swscale/FFmpegSwscale.h"
#include "MTVideoTransfer.h"
#include "HBPictureDemo.h"
#include "CSDemo.h"
#include "libyuv.h"
#include <unistd.h>

#define PROJ_ROOT_PATH "/Users/zj-db0519/work/code/github/HbFFmpegMedia/resource"

int gMtmvLogLevel = LOG_LEVEL_DEBUG;

int main(int argc, const char * argv[]) {
    {/** media demo */
        {/** Image */
//            HBPickImageFrameFromVideo();
        }
        {/** Video */
            {/** 视频解码 */
                CSVideoDemo_VideoDecoder();
            }
//            {/** 测试 mp4 文件转 gif 文件测试 demo */
//                FormatConvert::VideoFormatTranser *pVideoConverter = new FormatConvert::VideoFormatTranser();
//                pVideoConverter->setInputVideoMediaFile((char *)(PROJ_ROOT_PATH"/video/gif/100.mp4"));
//                pVideoConverter->setOutputVideoMediaFile((char *)(PROJ_ROOT_PATH"/video/gif/100.gif"));
//                pVideoConverter->prepare();
//                pVideoConverter->doConvert();
//            }
        }
    }
    {/** Encapsulation */
        {/** CSTimeline */
//            CSModulePlayerDemo();
        }
    }

    
/** ################################# >>> Audio */
//    { /** Audio decode */
//        char *strInputAudioFile = (char *)"/Users/zj-db0519/work/code/github/HbFFmpegMedia/src/HBDemo/audio/AudioDecoder/100.mp4";
//        char *strOutputAudioFile = (char *)"/Users/zj-db0519/Desktop/material/folder/video/100_s16_2_output.pcm";
//        AudioParams outputAudioParams;
//        outputAudioParams.sample_fmt = AV_SAMPLE_FMT_S16;
//        outputAudioParams.channels = 2;
//        outputAudioParams.channel_layout = av_get_default_channel_layout(outputAudioParams.channels);
//        outputAudioParams.sample_rate = 44100;
//        outputAudioParams.mbitRate = 64000;
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
    
//    {
//        int iArgcNum = 3;
//        char *tmpArgv[] = {nullptr, \
//            (char *)"/Users/zj-db0519/work/code/github/HbFFmpegMedia/resource/video/concat/100.mp4", \
//            (char *)"/Users/zj-db0519/work/code/github/HbFFmpegMedia/resource/video/concat/100_transcode_main.flv"
//        };
//        demo_transcod_main(iArgcNum, (char **)tmpArgv);
//    }
    
//    {   /** 解码 并且 编码 */
//        int iArgcNum = 3;
//        char *tmpArgv[] = {"demo_decode_encode", (char *)"mpg"};
//        demo_decode_encode(iArgcNum, (char **)tmpArgv);
//    }
    
    return 0;
}






























