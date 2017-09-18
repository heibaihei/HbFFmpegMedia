//
//  Common.h
//  admanagerment
//
//  Created by meitu on 2017/4/26.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef Common_h
#define Common_h


extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/mathematics.h"
#include "libavutil/time.h"
#include "libavutil/opt.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
#include "libavutil/imgutils.h"
#include "libavutil/mathematics.h"
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
#include "libswresample/swresample.h"
#include "libavutil/avassert.h"
#include "libavutil/bprint.h"
    
};

#ifndef DEBUG_MODE
#define DEBUG_MODE 1
#endif

#endif /* Common_h */
