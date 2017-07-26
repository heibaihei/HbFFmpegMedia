#ifndef __HB_DEMO_COMMON_H__
#define __HB_DEMO_COMMON_H__

#include "HBLog.h"
#include "HBPickPicture.h"

double mt_gettime_monotonic();

int simplest_ffmpeg_demuxer(int argc, char* argv[]);

int simplest_video_encoder(int argc, char* argv[]);

#endif
