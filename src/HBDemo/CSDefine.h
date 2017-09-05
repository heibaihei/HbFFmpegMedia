#ifndef __HBDEFINE_H__
#define __HBDEFINE_H__

#include "CSUtil.h"

#define MT_PLATFORM_UNKNOWN            0
#define MT_PLATFORM_IOS                1
#define MT_PLATFORM_ANDROID            2
#define MT_PLATFORM_WIN32              3
#define MT_PLATFORM_LINUX              4
#define MT_PLATFORM_MAC                5

#define MT_TARGET_PLATFORM         MT_PLATFORM_MAC

#define HB_OK     (0)
#define HB_ERROR  (-1)
#define HB_EOF    (-2)

#define DECODE_WITH_MULTI_THREAD_MODE  0

#define DECODE_STATE_READPKT_END     0X0001
#define DECODE_STATE_DECODE_END      0X0002
#define DECODE_STATE_READPKT_ABORT   0X0004
#define DECODE_STATE_DECODE_ABORT    0X0008
#define DECODE_STATE_FLUSH_MODE      0X0010

typedef enum AUDIO_SAMPLE_FORMAT {
    MT_SAMPLE_FMT_NONE = 0,
    MT_SAMPLE_FMT_U8,          ///< unsigned 8 bits
    MT_SAMPLE_FMT_S16,         ///< signed 16 bits
    MT_SAMPLE_FMT_S32,         ///< signed 32 bits
    MT_SAMPLE_FMT_FLT,         ///< float
    MT_SAMPLE_FMT_DBL,         ///< double
    
    MT_SAMPLE_FMT_U8P,         ///< unsigned 8 bits, planar
    MT_SAMPLE_FMT_S16P,        ///< signed 16 bits, planar
    MT_SAMPLE_FMT_S32P,        ///< signed 32 bits, planar
    MT_SAMPLE_FMT_FLTP,        ///< float, planar
    
    MT_SAMPLE_FMT_NB           ///< Number of sample formats. DO NOT USE if linking dynamically
} AUDIO_SAMPLE_FORMAT;

typedef enum VIDEO_PIX_FORMAT {
    MT_PIX_FMT_NONE     = -1,
    MT_PIX_FMT_YUV420P = 0,
    MT_PIX_FMT_YUV422P  = 1,
    MT_PIX_FMT_YUV444P  = 2,
    MT_PIX_FMT_NV12     = 3,
    MT_PIX_FMT_NV21     = 4,
} VIDEO_PIX_FORMAT;

typedef struct _ImageParams {
    enum AVPixelFormat mPixFmt;
    float mWidth;
    float mHeight;
    char *mCodecType;
    int   mAlign;
    int   mDataSize; /** 当前媒体格式下，每个帧的大小 */
} ImageParams;

typedef enum MEDIA_DATA_TYPE {
    PIC_D_TYPE_UNKNOWN = 0,
    PIC_D_TYPE_RAW_BY_FILE = 1,
    PIC_D_TYPE_RAW_BY_MEMORY = 2,
    PIC_D_TYPE_RAW_BY_PROTOCOL = 2,
    PIC_D_TYPE_COMPRESS = 3,
} MEDIA_DATA_TYPE;

#endif
