//
//  CSVideo.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/1.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSVideoDemo.h"
#include "MTVideoTransfer.h"
#include "CSModule.h"

/**
 *  视频裁剪： https://zhuanlan.zhihu.com/p/28008666
 *
 */

int CSVideoDemo_VideoTransfor() {
    
    {   /** 测试 mp4 文件转 gif 文件测试 demo */
        FormatConvert::VideoFormatTranser *pVideoConverter = new FormatConvert::VideoFormatTranser();
        pVideoConverter->setInputVideoMediaFile((char *)(VIDEO_RESOURCE_ROOT_PATH"/gif/100.mp4"));
        pVideoConverter->setOutputVideoMediaFile((char *)(VIDEO_RESOURCE_ROOT_PATH"/gif/100.gif"));
        pVideoConverter->setSyncMode(true);
        pVideoConverter->prepare();
        pVideoConverter->doConvert();
    }
    return HB_OK;
}

int CSVideoDemo_VideoPlayer() {
    HBMedia::CSVideoDecoder* pVideoDecoder = nullptr;
    {   /**
         *  开启视频解码器
         */
        ImageParams inImageParamObj;
        inImageParamObj.mWidth = 480;
        inImageParamObj.mHeight = 480;
        inImageParamObj.mPixFmt = CS_PIX_FMT_RGBA;
        inImageParamObj.mAlign = 1;
        
        pVideoDecoder = new HBMedia::CSVideoDecoder();
        pVideoDecoder->setInMediaType(MD_TYPE_COMPRESS);
        pVideoDecoder->setOutMediaType(MD_TYPE_RAW_BY_MEMORY);
        pVideoDecoder->setInMediaFile((char *)CS_COMMON_RESOURCE_ROOT_PATH"/video/100.mp4");
        pVideoDecoder->setOutImageMediaParams(inImageParamObj);
        pVideoDecoder->prepare();
        pVideoDecoder->start();
    }
    
    {
        HBMedia::CSVideoPlayer * pVideoPlayer = new HBMedia::CSVideoPlayer();
        pVideoPlayer->setVideoProvider(pVideoDecoder);
        pVideoPlayer->setInImageMediaParams(*(pVideoDecoder->getOutImageMediaParams()));
        pVideoPlayer->prepare();
        pVideoPlayer->doShow();
    }
    return HB_OK;
}

int CSVideoDemo_VideoAnalysis() {
    {
        /** 视频解码，将裸数据以内存缓存队列的方式输出解码数据，内部解码线程以最大的缓冲节点数缓冲解码后的数据，外部通过获取节点的方式进行取帧操作 */
        HBMedia::CSVideoAnalysis* pVideoAnalysiser = new HBMedia::CSVideoAnalysis();
        pVideoAnalysiser->setInMediaType(MD_TYPE_COMPRESS);
        pVideoAnalysiser->setOutMediaType(MD_TYPE_RAW_BY_MEMORY);
        pVideoAnalysiser->setInMediaFile((char *)CS_COMMON_RESOURCE_ROOT_PATH"/video/12541.mp4");
        
        pVideoAnalysiser->prepare();
        pVideoAnalysiser->start();
        AVFrame *pNewFrame = nullptr;
        if (pVideoAnalysiser->getStatus() & S_PREPARED) {
            while (!(pVideoAnalysiser->getStatus() & S_DECODE_END)) {
                pNewFrame = nullptr;
                if ((pVideoAnalysiser->receiveFrame(&pNewFrame) == HB_OK) && pNewFrame) {
                    
                    /** 对传入的视频帧信息进行解析 */
                    pVideoAnalysiser->analysisFrame(pNewFrame, AVMEDIA_TYPE_VIDEO);
                    
                    if (pNewFrame->opaque)
                        av_freep(pNewFrame->opaque);
                    av_frame_free(&pNewFrame);
                }
                
                usleep(10);
            }
        }
        
        pVideoAnalysiser->ExportAnalysisInfo();
        pVideoAnalysiser->stop();
        pVideoAnalysiser->release();
    }
    return HB_ERROR;
}

int CSVideoDemo_VideoEncoder() {
    {   /** 视频编码，将视频裸数据转换成编码数据文件输出 */
        ImageParams inImageParamObj;
        inImageParamObj.mWidth = 480;
        inImageParamObj.mHeight = 480;
        inImageParamObj.mPixFmt = CS_PIX_FMT_YUV420P;
        inImageParamObj.mAlign = 1;
        
        HBMedia::CSVideoEncoder* pVideoEncoder = new HBMedia::CSVideoEncoder();
        pVideoEncoder->setInMediaType(MD_TYPE_RAW_BY_MEMORY);
        pVideoEncoder->setInImageParams(inImageParamObj);
        pVideoEncoder->setOutMediaType(MD_TYPE_COMPRESS);
        pVideoEncoder->setOutMediaFile((char *)VIDEO_RESOURCE_ROOT_PATH"/encode/new100.mp4");

        pVideoEncoder->prepare();
        pVideoEncoder->start();
        
        {
            HBMedia::CSVideoDecoder* pVideoDecoder = new HBMedia::CSVideoDecoder();
            pVideoDecoder->setInMediaType(MD_TYPE_COMPRESS);
            pVideoDecoder->setOutMediaType(MD_TYPE_RAW_BY_MEMORY);
            pVideoDecoder->setInMediaFile((char *)CS_COMMON_RESOURCE_ROOT_PATH"/video/100.mp4");
            
            pVideoDecoder->prepare();
            pVideoDecoder->start();
            AVFrame *pNewFrame = nullptr;
            if (pVideoDecoder->getStatus() & S_PREPARED) {
                while (!(pVideoDecoder->getStatus() & S_DECODE_END)) {
                    pNewFrame = nullptr;
                    if ((pVideoDecoder->receiveFrame(&pNewFrame) == HB_OK) && pNewFrame) {
                        pVideoEncoder->sendFrame(&pNewFrame);
                    }
                }
                pVideoEncoder->sendFrame(NULL);
            }
            pVideoDecoder->stop();
            pVideoDecoder->release();
        }
        
        pVideoEncoder->syncWait();
        pVideoEncoder->stop();
        pVideoEncoder->release();
    }
    return HB_OK;
}
int CSVideoDemo_VideoDecoder() {
//    {
//        /** 视频解码，将裸数据存储文件的方式输出解码数据 */
//        HBMedia::CSVideoDecoder* pVideoDecoder = new HBMedia::CSVideoDecoder();
//        pVideoDecoder->setInMediaType(MD_TYPE_COMPRESS);
//        pVideoDecoder->setOutMediaType(MD_TYPE_RAW_BY_FILE);
//        pVideoDecoder->setInMediaFile((char *)CS_COMMON_RESOURCE_ROOT_PATH"/video/100.mp4");
//        pVideoDecoder->setOutMediaFile((char *)VIDEO_RESOURCE_ROOT_PATH"/decode/100_raw.mp4");
//
//        pVideoDecoder->prepare();
//        pVideoDecoder->start();
//        pVideoDecoder->syncWait();
//        pVideoDecoder->stop();
//        pVideoDecoder->release();
//    }
    {
        /** 视频解码，将裸数据以内存缓存队列的方式输出解码数据，内部解码线程以最大的缓冲节点数缓冲解码后的数据，外部通过获取节点的方式进行取帧操作 */
        HBMedia::CSVideoDecoder* pVideoDecoder = new HBMedia::CSVideoDecoder();
        pVideoDecoder->setInMediaType(MD_TYPE_COMPRESS);
        pVideoDecoder->setOutMediaType(MD_TYPE_RAW_BY_MEMORY);
        pVideoDecoder->setInMediaFile((char *)CS_COMMON_RESOURCE_ROOT_PATH"/video/100.mp4");

        pVideoDecoder->prepare();
        pVideoDecoder->start();
        AVFrame *pNewFrame = nullptr;
        if (pVideoDecoder->getStatus() & S_PREPARED) {
            while (!(pVideoDecoder->getStatus() & S_DECODE_END)) {
                pNewFrame = nullptr;
                if ((pVideoDecoder->receiveFrame(&pNewFrame) == HB_OK) && pNewFrame) {
                    if (pNewFrame->opaque)
                        av_freep(pNewFrame->opaque);
                    av_frame_free(&pNewFrame);
                }
                
                usleep(10);
            }
        }
        pVideoDecoder->stop();
        pVideoDecoder->release();
    }
    
    return HB_OK;
}
