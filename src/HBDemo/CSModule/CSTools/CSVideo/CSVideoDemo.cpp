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
        if (pVideoDecoder->getStatus() & DECODE_STATE_PREPARED) {
            while (!(pVideoDecoder->getStatus() & DECODE_STATE_DECODE_END)) {
                pNewFrame = nullptr;
                if ((pVideoDecoder->receiveFrame(pNewFrame) == HB_OK) && pNewFrame) {
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
