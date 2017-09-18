//
//  ThumbGenerater.cpp
//  sdl-ffplay
//
//  Created by Javan on 14-5-26.
//  Copyright (c) 2014年 Javan. All rights reserved.
//

#include "ThumbGenerater.h"
//#include "KeyFramePicker.h"
#include "LogHelper.h"
//#define LOGE printf

static char sSavePath[1024] = {0};
static int SaveFrameARGB(unsigned char* pARGB, int width, int height, int iFrame)
{
	FILE *pFile;
	char szFilename[1024] = {0};

	if (pARGB == NULL || width == 0 || height == 0) {
	    return -1;
	}
	// Open file
	sprintf(szFilename, "%sframe%d.ppm", sSavePath,iFrame);
    pFile = fopen(szFilename, "wb");
    if (pFile == NULL) {
        return - 1;
    }
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);
    // Write pixel data
    for (int i = width*height; i > 0; i--)
    {
        fwrite(pARGB, 1, 3, pFile);
        pARGB += 4;
    }
    fclose(pFile);

    return 0;
}

int GenerateThumb(const char *src, const char* save_path, double *times, int length) {
    if (!src || !save_path || !times || length == 0) {
        return -1;
    }

    CKeyFramePicker *picker = new CKeyFramePicker();
    int res = 0;
    int ret = -1;
    sprintf(sSavePath, "%s", save_path);

    ret = picker->Open(src, true);
    if (ret < 0)
    {
        LOGE("key frame picker open error");

        picker->Close();

        delete picker;
        picker = NULL;
        return ret;
    }

    picker->m_SaveFunc = SaveFrameARGB;
    res = picker->GetKeyFrameOrder(times, length);

    picker->Close();
    delete picker;
    picker = NULL;
    return res;
}

