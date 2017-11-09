//
//  HBPickPicture.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/7/6.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "HBPictureDemo.h"
#include "CSPicture.h"
#include "CSCommon.h"

/**
 * libyuv 参考：
 *     http://blog.csdn.net/fengbingchun/article/details/50323273
 *
 */

#define PIC_RESOURCE_ROOT_PATH       "/Users/zj-db0519/work/code/github/HbFFmpegMedia/resource/Picture"
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
 
    int videoStream = -1;
    int HBError = HB_OK;
    AVFormatContext* pFormatCtx = nullptr;
    if ((HBError = avformat_open_input(&pFormatCtx, PIC_RESOURCE_ROOT_PATH"/100.mp4", NULL, NULL)) != 0) {
        LOGE("Open input media file failed !, %s", av_err2str(HBError));
        return HB_ERROR;
    }
    
    if ((HBError = avformat_find_stream_info(pFormatCtx, NULL)) < 0) {
        LOGE("Find stream info failed !, %s", av_err2str(HBError));
        return HB_ERROR;
    }
    
    av_dump_format(pFormatCtx, 0, SRC_MEDIA_VIDEO_FILE, 0);

    if ((videoStream = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0)) < 0) {
        LOGE("find best video stream failed!, %s", av_err2str(videoStream));
        return HB_ERROR;
    }
    
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

static void _SaveFrame(AVFrame *pFrame, int width, int height) {
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

int PictureCSpictureDemo()
{
    HBMedia::CSPicture objPicture;
    objPicture.setSrcPicDataType(MD_TYPE_RAW_BY_FILE);
    objPicture.setInputPicMediaFile((char *)("/Picture/encoder/1080_1080_JYUV420P.yuv"));
    ImageParams srcPictureParam = { CS_PIX_FMT_YUVJ420P, 1080, 1080, NULL, 1 };
    objPicture.setSrcPictureParam(&srcPictureParam);
    
    objPicture.setTrgPicDataType(MD_TYPE_COMPRESS);
    objPicture.setOutputPicMediaFile((char *)("/Picture/encoder/1080_1080_JYUV420P_HB_encoder.jpg"));
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





















