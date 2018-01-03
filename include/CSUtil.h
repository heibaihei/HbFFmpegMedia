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
#include "libavutil/time.h"
#include "libavutil/imgutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/error.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libswscale/swscale.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
    
#ifdef __cplusplus
};
#endif

#ifndef makeErrorStr
static char errorStr[AV_ERROR_MAX_STRING_SIZE];
#define makeErrorStr(errorCode) av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, errorCode)
#endif

typedef struct CSMediaCodec {
public:
    CSMediaCodec() {
        mCodec = nullptr;
        mCodecCtx = nullptr;
        mStream = nullptr;
        mFormat = nullptr;
    };
    ~CSMediaCodec() {};
    
    AVCodec         *mCodec;
    AVCodecContext  *mCodecCtx;
    AVStream        *mStream;
    AVFormatContext *mFormat;
} CSCodec;

#endif /* _CSUTIL_H_ */
