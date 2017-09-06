#include <mach/mach_time.h>
#include "CSCommon.h"

double mt_gettime_monotonic() {
    mach_timebase_info_data_t timebase;
    mach_timebase_info(&timebase);
    uint64_t time;
    time = mach_absolute_time();
    double seconds = ((double)time * (double)timebase.numer)/((double)timebase.denom * 1e9);
    return seconds;
}

int globalInitial()
{
    av_register_all();
    avformat_network_init();
    
    return HB_OK;
}

enum AVPixelFormat getImageInnerFormat(IMAGE_PIX_FORMAT pixFormat)
{
    switch (pixFormat) {
        case CS_PIX_FMT_YUV420P:
            return AV_PIX_FMT_YUV420P;
        case CS_PIX_FMT_YUV422P:
            return AV_PIX_FMT_YUV422P;
        case CS_PIX_FMT_YUV444P:
            return AV_PIX_FMT_YUV444P;
        case CS_PIX_FMT_NV12:
            return AV_PIX_FMT_NV12;
        case CS_PIX_FMT_NV21:
            return AV_PIX_FMT_NV21;
        default:
            LOGE("Get image inner format failed !");
            return AV_PIX_FMT_NONE;
    }
}
