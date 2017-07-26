//
//  main.c
//  Sample
//
//  Created by zj-db0519 on 2017/4/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>

#include "HBLog.h"
#include "HBCommon.h"
#include "HBAudio.h"
#include "Picture/encoder/picture_encoder.h"
#include "FFmpegModule/Swscale/FFmpegSwscale.h"

#include <unistd.h>

#define PROJ_ROOT_PATH "/Users/zj-db0519/work/code/mlab_meitu/FFmpeg_git/ffmpeg_private/"

int gMtmvLogLevel = LOG_LEVEL_DEBUG;

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
//        outputAudioParams.fmt = AV_SAMPLE_FMT_S16;
//        outputAudioParams.channel_layout = AV_CH_LAYOUT_STEREO;
//        outputAudioParams.channels = av_get_channel_layout_nb_channels(outputAudioParams.channel_layout);
//        outputAudioParams.freq = 44100;
//        
//        HBAudioDecoder(strInputAudioFile, strOutputAudioFile, AUDIO_DATA_TYPE_OF_PCM, outputAudioParams);
//    }
    
    { /** Audio decode */
        char *strInputAudioFile = (char *)PROJ_ROOT_PATH"/src/HBDemo/audio/AudioEncoder/tdjm.pcm";
        char *strOutputAudioFile = (char *)PROJ_ROOT_PATH"/src/HBDemo/audio/AudioEncoder/tdjm.aac";
        AudioParams outputAudioParams;
        outputAudioParams.fmt = AV_SAMPLE_FMT_FLTP;
        outputAudioParams.channel_layout = AV_CH_LAYOUT_STEREO;
        outputAudioParams.channels = av_get_channel_layout_nb_channels(outputAudioParams.channel_layout);
        outputAudioParams.freq = 44100;
        outputAudioParams.mbitRate = 64000;
        
        HBAudioEncoder(strInputAudioFile, strOutputAudioFile, AUDIO_DATA_TYPE_OF_AAC, outputAudioParams);
    }
/** ################################# <<< */
    
    
    
    
    return 0;
}


