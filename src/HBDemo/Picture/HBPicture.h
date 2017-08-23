//
//  Header.h
//  FFmpeg
//
//  Created by zj-db0519 on 2017/8/17.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef _HBPICTURE_H_
#define _HBPICTURE_H_

#ifdef __cplusplus
extern "C" {
#endif
    
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/mem.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"

    
#ifdef __cplusplus
};
#endif

#include "HBSampleDefine.h"
#include "HBCommon.h"

typedef struct _PictureParams {
    enum AVPixelFormat mPixFmt;
    float mWidth;
    float mHeight;
    char *mCodecType;
    int   mAlign;
} PictureParams;

#endif /* _HBPICTURE_H_ */
