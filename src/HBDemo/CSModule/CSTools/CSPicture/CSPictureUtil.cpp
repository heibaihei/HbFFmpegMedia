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

int CSPicUtil_ExportYUV420RawData(AVFrame *pFrame) {
    if (!pFrame) {
        LOGE("%s invalid params !", __FUNCTION__);
        return HB_ERROR;
    }
    
    FILE *yuv_file = fopen(RAW_IMAGE_ROOT_PATH"yuv420_raw","ab");
    if (!yuv_file)
        return 0;
    
    char* buf = new char[pFrame->height * pFrame->width * 3 / 2];
    memset(buf, 0, pFrame->height * pFrame->width * 3 / 2);
    int height = pFrame->height;
    int width = pFrame->width;
    printf("decode video ok\n");
    int a = 0, i;
    for (i = 0; i<height; i++)
    {
        memcpy(buf + a, pFrame->data[0] + i * pFrame->linesize[0], width);
        a += width;
    }
    for (i = 0; i<height / 2; i++)
    {
        memcpy(buf + a, pFrame->data[1] + i * pFrame->linesize[1], width / 2);
        a += width / 2;
    }
    for (i = 0; i<height / 2; i++)
    {
        memcpy(buf + a, pFrame->data[2] + i * pFrame->linesize[2], width / 2);
        a += width / 2;
    }
    fwrite(buf, 1, pFrame->height * pFrame->width * 3 / 2, yuv_file);
    delete [] buf;
    buf = NULL;
    
    return HB_OK;
}
