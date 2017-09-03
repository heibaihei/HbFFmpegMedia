//
//  CSVideo.h
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/1.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSVideo_h
#define CSVideo_h

#include <stdio.h>
#include "HBCSDefine.h"
#include "HBCommon.h"

typedef struct VideoParams {
    int mVideoWidth;
    int mVideoHeight;
    VIDEO_PIX_FORMAT mPixFmt;
    int mBitRate;
    int mRorate;
    /** 视频封装格式 类型 */
    /** 视频编码格式 类型 */
} VideoParams;

#endif /* CSVideo_h */
