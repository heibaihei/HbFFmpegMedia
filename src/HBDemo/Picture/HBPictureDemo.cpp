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
#include "stb/stb_image_write.h"

/**
 * libyuv 参考：
 *     http://blog.csdn.net/fengbingchun/article/details/50323273
 *
 // 像素分量分离
 * http://blog.csdn.net/leixiaohua1020/article/details/50534150
 */

#define PIC_RESOURCE_ROOT_PATH       "/Users/zj-db0519/work/code/github/HbFFmpegMedia/resource/Picture"
#define TARGET_IMAGE_PIX_FMT         (AV_PIX_FMT_YUYV422)
#define TARGET_IMAGE_PIX_FMT_STR      "YUYV422"

/**
 *  生成PPM媒体文件，存放的是 RGB 媒体数据
 */
static void _ProcessFrame(AVFrame *pSrcFrame, AVFrame *pDstFrame);

const char *pInputFilePath = (const char *)(PIC_RESOURCE_ROOT_PATH"/100.mp4");
int HBPickPictureFromVideo()
{
    av_register_all();
 
    int iVideoStreamIndex = -1;
    int HBError = HB_OK;
    AVFormatContext* pFormatCtx = nullptr;
    if ((HBError = avformat_open_input(&pFormatCtx, pInputFilePath, NULL, NULL)) != 0) {
        LOGE("Open input media file failed !, %s", av_err2str(HBError));
        return HB_ERROR;
    }
    
    if ((HBError = avformat_find_stream_info(pFormatCtx, NULL)) < 0) {
        LOGE("Find stream info failed !, %s", av_err2str(HBError));
        return HB_ERROR;
    }
    
    if ((iVideoStreamIndex = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0)) < 0) {
        LOGE("find best video stream failed!, %s", av_err2str(iVideoStreamIndex));
        return HB_ERROR;
    }
    
    AVStream *pVideoStream = pFormatCtx->streams[iVideoStreamIndex];
    AVCodec* pCodec = avcodec_find_decoder(pVideoStream->codecpar->codec_id);
    if (pCodec == nullptr) {
        LOGE("Can't find valid decoder !");
        return HB_ERROR;
    }
    
    AVCodecContext* pCodecCtx = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(pCodecCtx, pVideoStream->codecpar);
    
    av_dump_format(pFormatCtx, 0, pInputFilePath, 0);
    if ((HBError = avcodec_open2(pCodecCtx, pCodec, NULL)) < 0) {
        LOGE("Can't open video codec, %s", av_err2str(HBError));
        return HB_ERROR;
    }
    
    AVFrame *pTargetFrame = av_frame_alloc();
    if (pTargetFrame == nullptr) {
        LOGE("allock new frame room failed !");
        return HB_ERROR;
    }
    
    HBError = av_image_alloc(pTargetFrame->data, pTargetFrame->linesize, pCodecCtx->width, pCodecCtx->height, TARGET_IMAGE_PIX_FMT, 1);
    if (HBError < 0) {
        LOGE("Video format alloc new image room failed !");
        return HB_ERROR;
    }
    
    AVFrame *pFrame = nullptr;
    AVPacket* pNewPacket = av_packet_alloc();
    while (av_read_frame(pFormatCtx, pNewPacket) == 0) {
        
        if (pNewPacket->stream_index == iVideoStreamIndex) {
            
            HBError = avcodec_send_packet(pCodecCtx, pNewPacket);
            if (HBError != 0) {
                if (HBError == AVERROR(EAGAIN))
                    continue;
                LOGE("avcodec send packet failed !");
                break;
            }
            
            pFrame = av_frame_alloc();
            if (!pFrame) {
                LOGE("Malloc new frame failed !");
                return HB_ERROR;
            }
            
            HBError = avcodec_receive_frame(pCodecCtx, pFrame);
            if (HBError != 0) {
                if (HBError != AVERROR(EAGAIN)) {
                    LOGW("Decode process end!");
                    break;
                }
                av_frame_free(&pFrame);
                continue;
            }
            else {
                _ProcessFrame(pFrame, pTargetFrame);
                break;
            }
        }
    }
    
    /** 释放 stPacket 内部占用的内存 */
    av_packet_unref(pNewPacket);

    av_frame_free(&pTargetFrame);
    av_frame_free(&pFrame);
    
    avcodec_close(pCodecCtx);
    
    avformat_close_input(&pFormatCtx);
    
    return 0;
}

static void _ProcessFrame(AVFrame *pSrcFrame, AVFrame *pDstFrame) {
    int HBError = HB_OK;
    struct SwsContext *pSwsCtx = sws_getContext(pSrcFrame->width, pSrcFrame->height, (enum AVPixelFormat)pSrcFrame->format, \
                            pSrcFrame->width, pSrcFrame->height, TARGET_IMAGE_PIX_FMT, SWS_BILINEAR, NULL, NULL, NULL);
    
    /** 得到帧，可以在这里对帧进行相应的操作 */
    HBError = sws_scale(pSwsCtx, pSrcFrame->data, pSrcFrame->linesize, 0, pSrcFrame->height, \
                        pDstFrame->data, pDstFrame->linesize);
    if (HBError <= 0) {
        LOGE("sws scale new frame failed !");
    }
    else {//stbi_write_bmp
        pDstFrame->width = pSrcFrame->width;
        pDstFrame->height = pSrcFrame->height;
        pDstFrame->format = TARGET_IMAGE_PIX_FMT;
        LOGI("sws scale new frame info: w:%d, h:%d to w:%d, h:%d ", \
             pSrcFrame->width, pSrcFrame->height, pDstFrame->width, pDstFrame->height);
        /** 文件输出 */
        // (char const *filename, int w, int h, int comp, const void  *data, int stride_in_bytes)
//        stbi_write_png(PIC_RESOURCE_ROOT_PATH"/newFrame.png", pDstFrame->width, pDstFrame->height, 1, pDstFrame->data, 1);
        
        {/** 以文件的方式输出裸数据 */
            char szFilename[512] = {0};
            sprintf(szFilename, "%s/OutputPicture_%s_%d_%d.raw", PIC_RESOURCE_ROOT_PATH, \
                    TARGET_IMAGE_PIX_FMT_STR, pDstFrame->width, pDstFrame->height);
            FILE *pFileHanle = fopen(szFilename, "wb");
            if (pFileHanle == NULL) {
                LOGE("Open output dest file %s failed !", szFilename);
                return;
            }
            fwrite(pDstFrame->data[0],  1, \
                   av_image_get_buffer_size((enum AVPixelFormat)(pDstFrame->format), pDstFrame->width, pDstFrame->height, 1),
                   pFileHanle);
            
            fclose(pFileHanle);
        }
    }
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





















