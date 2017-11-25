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
#include "CSDefine.h"
#include "CSCommon.h"
#include "CSMediaBase.h"

int TVideoFasterTranfor(int argc, const char *argv[]);

/**
 *  视频媒体文件解码
 */
int CSVideoDemo_VideoDecoder();

#endif /* CSVideo_h */
