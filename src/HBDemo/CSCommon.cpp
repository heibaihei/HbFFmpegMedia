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

int imageParamInit(ImageParams* param) {
    if (!param) {
        LOGE("Image param args is invalid !");
        return HB_ERROR;
    }
    
    memset(param, 0x00, sizeof(ImageParams));
    param->mAlign = 1;
    param->mWidth = 0;
    param->mHeight = 0;
    param->mPixFmt = CS_PIX_FMT_NONE;
    param->mFormatType = NULL;
    param->mDataSize = 0;
    return HB_OK;
}

enum AVPixelFormat getImageInnerFormat(IMAGE_PIX_FORMAT pixFormat) {
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
        case CS_PIX_FMT_YUVJ420P:
            return AV_PIX_FMT_YUVJ420P;
        default:
            LOGE("Get image inner format failed !");
            exit(0);
    }
}

enum IMAGE_PIX_FORMAT getImageExternFormat(AVPixelFormat pixFormat) {
    switch (pixFormat) {
        case AV_PIX_FMT_YUV420P:
            return CS_PIX_FMT_YUV420P;
        case AV_PIX_FMT_YUV422P:
            return CS_PIX_FMT_YUV422P;
        case AV_PIX_FMT_YUV444P:
            return CS_PIX_FMT_YUV444P;
        case AV_PIX_FMT_NV12:
            return CS_PIX_FMT_NV12;
        case AV_PIX_FMT_NV21:
            return CS_PIX_FMT_NV21;
        case AV_PIX_FMT_YUVJ420P:
            return CS_PIX_FMT_YUVJ420P;
        default:
            LOGE("Get image inner format failed !");
            exit(0);
    }
}

char* getImagePixFmtDescript(IMAGE_PIX_FORMAT dataType) {
    switch (dataType) {
        case CS_PIX_FMT_NONE:
            return (char*)"CS_PIX_FMT_NONE";
        case CS_PIX_FMT_YUV420P:
            return (char*)"CS_PIX_FMT_YUV420P";
        case CS_PIX_FMT_YUV422P:
            return (char*)"CS_PIX_FMT_YUV422P";
        case CS_PIX_FMT_YUV444P:
            return (char*)"CS_PIX_FMT_YUV444P";
        case CS_PIX_FMT_NV12:
            return (char*)"CS_PIX_FMT_NV12";
        case CS_PIX_FMT_NV21:
            return (char*)"CS_PIX_FMT_NV21";
        case CS_PIX_FMT_YUVJ420P:
            return (char*)"CS_PIX_FMT_YUVJ420P";
        default:
            LOGE("Get image pixmat inner format failed !");
            exit(0);
    }

}

char* getMediaDataTypeDescript(MEDIA_DATA_TYPE dataType) {
    switch (dataType) {
        case MD_TYPE_UNKNOWN:
            return (char*)"MD_TYPE_UNKNOWN";
        case MD_TYPE_RAW_BY_FILE:
            return (char*)"MD_TYPE_RAW_BY_FILE";
        case MD_TYPE_RAW_BY_MEMORY:
            return (char*)"MD_TYPE_RAW_BY_MEMORY";
        case MD_TYPE_COMPRESS:
            return (char*)"MD_TYPE_COMPRESS";
        case MD_TYPE_RAW_BY_PROTOCOL:
            return (char*)"MD_TYPE_RAW_BY_PROTOCOL";
        default:
            LOGE("Get image inner format failed !");
            exit(0);
    }
}
