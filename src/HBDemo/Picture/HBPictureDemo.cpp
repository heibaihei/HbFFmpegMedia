//
//  HBPickPicture.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/7/6.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "HBPictureDemo.h"

#include "CSCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"

#ifdef __cplusplus
};
#endif


#define SRC_MEDIA_VIDEO_FILE         "/Users/zj-db0519/work/code/mlab_meitu/FFmpeg_git/ffmpeg_private/resource/video/100.mp4"
#define OUTPUT_MEDIA_VIDEO_FILE_DIR  "/Users/zj-db0519/work/code/mlab_meitu/FFmpeg_git/ffmpeg_private/resource/video/"


#define TARGET_PICTURE_PIX_FMT       AV_PIX_FMT_RGB24


/**
 *  生成PPM媒体文件，存放的是 RGB 媒体数据
 */
static void _SaveFrame(AVFrame *pFrame, int width, int height);

int HBPickPictureFromVideo()
{
    av_register_all();
    
    AVFormatContext* pFormatCtx = nullptr;
    if (avformat_open_input(&pFormatCtx, SRC_MEDIA_VIDEO_FILE, NULL, NULL) != 0)
        return -1;
    
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
        return -1;
    
    av_dump_format(pFormatCtx, 0, SRC_MEDIA_VIDEO_FILE, 0);
    
    int videoStream = -1;
    for (int i=0; i<pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }
    
    if (videoStream == -1)
        return -1;
    
    AVCodecContext* pCodecCtxOrig = pFormatCtx->streams[videoStream]->codec;
    AVCodec* pCodec = avcodec_find_decoder(pCodecCtxOrig->codec_id);
    if (pCodec == nullptr)
        return -1;
    
    AVCodecContext* pCodecCtx = avcodec_alloc_context3(pCodec);
    if (avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0)
        return -1;
    
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
        return -1;
    
    
    
    AVFrame *pFrame = av_frame_alloc();
    AVFrame *pFrameRGB = av_frame_alloc();
    if (pFrame == nullptr || pFrameRGB == nullptr)
        return -1;
    
    int numBytes = av_image_get_buffer_size(TARGET_PICTURE_PIX_FMT, pCodecCtx->width, pCodecCtx->height, 1);
    uint8_t *buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
    av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, TARGET_PICTURE_PIX_FMT, pCodecCtx->width, pCodecCtx->height, 1);
    struct SwsContext *pSwsCtx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, TARGET_PICTURE_PIX_FMT, SWS_BILINEAR, NULL, NULL, NULL);
    
    
    /** 读取媒体包 */
    AVPacket stPacket;
    int frameFinished;
    while (av_read_frame(pFormatCtx, &stPacket) >= 0) {
        if (stPacket.stream_index == videoStream) {
            
            /** avcodec_decode_video2() 会在解码到完整的一帧时设置 frameFinished 为真 */
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &stPacket);
            
            if (frameFinished) {
                sws_scale(pSwsCtx, pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
                
                _SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height);
                break;
            }
        }
    }
    
    /** 释放 stPacket 内部占用的内存 */
    av_packet_unref(&stPacket);
    
    av_free(buffer);
    av_frame_free(&pFrameRGB);
    av_frame_free(&pFrame);
    
    avcodec_close(pCodecCtx);
    avcodec_close(pCodecCtxOrig);
    
    avformat_close_input(&pFormatCtx);
    
    return 0;
}

static void _SaveFrame(AVFrame *pFrame, int width, int height){
    FILE *pFile = nullptr;
    char szFilename[512];
    
    sprintf(szFilename, OUTPUT_MEDIA_VIDEO_FILE_DIR"pickTargetFrame.ppm");
    pFile = fopen(szFilename, "wb");
    if (pFile == NULL)
        return;
    
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);
    
    fwrite(pFrame->data[0],  1, width*height*3, pFile);
    
    fclose(pFile);
}





















