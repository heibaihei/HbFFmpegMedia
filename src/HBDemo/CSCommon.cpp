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

int cropParamInit(CropParam* param) {
    if (!param) {
        LOGE("Image param args is invalid !");
        return HB_ERROR;
    }
    memset(param, 0x00, sizeof(CropParam));
    param->posX = 1;
    param->posY = 0;
    param->cropWidth = 0;
    param->cropHeight = CS_PIX_FMT_NONE;
    return HB_OK;
}

int audioParamInit(AudioParams* param) {
    if (!param) {
        LOGE("Image param args is invalid !");
        return HB_ERROR;
    }

    memset(param, 0x00, sizeof(CropParam));
    param->sample_rate = 0;
    param->channels = 0;
    param->channel_layout = 0;
    param->pri_sample_fmt = CS_SAMPLE_FMT_NONE;
    param->frame_size = 0;
    param->channel_layout = 0;
    param->bytes_per_sec = 0;
    param->mbitRate = 0;
    param->mAlign = 1;
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
    param->mPreImagePixBufferSize = 0;
    return HB_OK;
}

enum AVPixelFormat getImageInnerFormat(IMAGE_PIX_FORMAT pixFormat) {
    switch (pixFormat) {
        case CS_PIX_FMT_YUV420P:
            return AV_PIX_FMT_YUV420P;
        case CS_PIX_FMT_BGRA:
            return AV_PIX_FMT_BGRA;
        case CS_PIX_FMT_RGB8:
            return AV_PIX_FMT_RGB8;
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
        case AV_PIX_FMT_BGRA:
            return CS_PIX_FMT_BGRA;
        case AV_PIX_FMT_RGB8:
            return CS_PIX_FMT_RGB8;
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
        case CS_PIX_FMT_BGRA:
            return (char*)"CS_PIX_FMT_BGRA";
        case CS_PIX_FMT_RGB8:
            return (char*)"CS_PIX_FMT_RGB8";
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

enum AVSampleFormat getAudioInnerFormat(enum AUDIO_SAMPLE_FORMAT outFormat)
{
    switch (outFormat) {
        case CS_SAMPLE_FMT_U8:
            return AV_SAMPLE_FMT_U8;
        case CS_SAMPLE_FMT_S16:
            return AV_SAMPLE_FMT_S16;
        case CS_SAMPLE_FMT_S32:
            return AV_SAMPLE_FMT_S32;
        case CS_SAMPLE_FMT_FLT:
            return AV_SAMPLE_FMT_FLT;
        case CS_SAMPLE_FMT_DBL:
            return AV_SAMPLE_FMT_DBL;
        case CS_SAMPLE_FMT_U8P:
            return AV_SAMPLE_FMT_U8P;
        case CS_SAMPLE_FMT_S16P:
            return AV_SAMPLE_FMT_S16P;
        case CS_SAMPLE_FMT_S32P:
            return AV_SAMPLE_FMT_S32P;
        case CS_SAMPLE_FMT_FLTP:
            return AV_SAMPLE_FMT_FLTP;
        default:
            return AV_SAMPLE_FMT_NONE;
    }
}

enum AUDIO_SAMPLE_FORMAT getAudioOuterFormat(enum AVSampleFormat outFormat)
{
    switch (outFormat) {
        case AV_SAMPLE_FMT_U8:
            return CS_SAMPLE_FMT_U8;
        case AV_SAMPLE_FMT_S16:
            return CS_SAMPLE_FMT_S16;
        case AV_SAMPLE_FMT_S32:
            return CS_SAMPLE_FMT_S32;
        case AV_SAMPLE_FMT_FLT:
            return CS_SAMPLE_FMT_FLT;
        case AV_SAMPLE_FMT_DBL:
            return CS_SAMPLE_FMT_DBL;
        case AV_SAMPLE_FMT_U8P:
            return CS_SAMPLE_FMT_U8P;
        case AV_SAMPLE_FMT_S16P:
            return CS_SAMPLE_FMT_S16P;
        case AV_SAMPLE_FMT_S32P:
            return CS_SAMPLE_FMT_S32P;
        case AV_SAMPLE_FMT_FLTP:
            return CS_SAMPLE_FMT_FLTP;
        default:
            return CS_SAMPLE_FMT_NONE;
    }
}

bool needRescaleVideo(ImageParams *inParam, ImageParams *outParam)
{
    return !((inParam->mWidth == outParam->mWidth) &&
             (inParam->mHeight == outParam->mHeight) &&
             (inParam->mPixFmt == outParam->mPixFmt));
}

bool needResampleAudio(AudioParams *param1, AudioParams *param2)
{
    return !((param1->channels == param2->channels &&
              param1->pri_sample_fmt == param2->pri_sample_fmt &&
              param1->sample_rate == param2->sample_rate));
}


