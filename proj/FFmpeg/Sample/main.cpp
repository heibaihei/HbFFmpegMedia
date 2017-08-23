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
    
    PictureCSpictureDemo();
    
    return 0;
}

int PictureCSpictureDemo()
{
    HBMedia::CSPicture objPicture;
    objPicture.setSrcPicDataType(HBMedia::PIC_D_TYPE_RAW_YUV);
    objPicture.setInputPicMediaFile((char *)(PROJ_ROOT_PATH"/Picture/encoder/1080_1080_JYUV420P.yuv"));
    PictureParams srcPictureParam = { AV_PIX_FMT_YUVJ420P, 1080, 1080, 0 };
    objPicture.setSrcPictureParam(&srcPictureParam);
    
    objPicture.setTargetPicDataType(HBMedia::PIC_D_TYPE_COMPRESS);
    objPicture.setOutputPicMediaFile((char *)(PROJ_ROOT_PATH"/Picture/encoder/1080_1080_JYUV420P_HB_encoder.jpg"));
    PictureParams targetPictureParam = { AV_PIX_FMT_YUVJ420P, 1080, 1080, 0 };
    objPicture.setTargetPictureParam(&targetPictureParam);
    
    objPicture.picBaseInitial();
    objPicture.picEncoderInitial();
    objPicture.picEncoderOpen();
    
    uint8_t* pictureData = NULL;
    int      pictureDataSizes = 0;
    int HbErr = HB_OK;
    while (HbErr == HB_OK) {
        HbErr = objPicture.getPicRawData(&pictureData, &pictureDataSizes);
        if (HbErr != HB_OK) {
            LOGW("Can't get picture data !");
            break;
        }
        
        HbErr = objPicture.pictureEncode(pictureData, pictureDataSizes);
        if (HbErr != HB_OK) {
            LOGW("Picture encode exit !");
            break;
        }
    }
    
    objPicture.pictureFlushEncode();
    objPicture.picEncoderClose();
    objPicture.picEncoderRelease();
    
    return 0;
}





























