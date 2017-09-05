//
//  Header.h
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/5.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef _CSUTIL_H_
#define _CSUTIL_H_

#ifdef __cplusplus
extern "C" {
#endif
    
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libavutil/mem.h"
#include "libavutil/imgutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/error.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
    
#ifdef __cplusplus
};
#endif

#ifndef makeErrorStr
static char errorStr[AV_ERROR_MAX_STRING_SIZE];
#define makeErrorStr(errorCode) av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, errorCode)
#endif

#endif /* _CSUTIL_H_ */
