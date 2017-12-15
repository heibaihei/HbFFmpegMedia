//
//  CSPictureUtil.cpp
//  Sample
//
//  Created by zj-db0519 on 2017/12/14.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include <stdio.h>
#include "CSPicture.h"

#define RAW_IMAGE_ROOT_PATH "/Users/zj-db0519/work/code/github/HbFFmpegMedia/resource/video/raw/"

int CSPicUtil_ExportImgaeRawDataByFile(AVFrame *pFrame) {
    if (!pFrame) {
        LOGE("%s invalid params !", __FUNCTION__);
        return HB_ERROR;
    }
    
    char strTargetOutputFile[256] = "";
    sprintf(strTargetOutputFile, RAW_IMAGE_ROOT_PATH"%s_W%d_H%d", \
            getImagePixFmtDescript(getImageExternFormat((AVPixelFormat)pFrame->format)), \
            pFrame->width, pFrame->height);
    FILE *strTargetOutputFileHandle = fopen(strTargetOutputFile,"ab");
    if (!strTargetOutputFileHandle) {
        LOGE("%s open output file failed, %s", __FUNCTION__, strTargetOutputFile);
        return HB_ERROR;
    }
    
    int height = pFrame->height;
    int width = pFrame->width;
    
    if (pFrame->format == AV_PIX_FMT_YUV420P) {
        char* pOutputBuffer = new char[height * width * 3 / 2];
        memset(pOutputBuffer, 0, height * width * 3 / 2);
        
        int a = 0;
        for (int i = 0; i<height; i++)
        {
            memcpy(pOutputBuffer + a, pFrame->data[0] + i * pFrame->linesize[0], width);
            a += width;
        }
        for (int i = 0; i<height / 2; i++)
        {
            memcpy(pOutputBuffer + a, pFrame->data[1] + i * pFrame->linesize[1], width / 2);
            a += width / 2;
        }
        for (int i = 0; i<height / 2; i++)
        {
            memcpy(pOutputBuffer + a, pFrame->data[2] + i * pFrame->linesize[2], width / 2);
            a += width / 2;
        }
        fwrite(pOutputBuffer, 1, pFrame->height * pFrame->width * 3 / 2, strTargetOutputFileHandle);
        delete [] pOutputBuffer;
        pOutputBuffer = NULL;
    }
    return HB_OK;
}
