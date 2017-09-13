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

typedef enum IMAGE_PIX_FORMAT {
    CS_PIX_FMT_NONE     = -1,
    CS_PIX_FMT_YUV420P = 0,
    CS_PIX_FMT_YUV422P  = 1,
    CS_PIX_FMT_YUV444P  = 2,
    CS_PIX_FMT_NV12     = 3,
    CS_PIX_FMT_NV21     = 4,
    CS_PIX_FMT_YUVJ420P = 5,
} IMAGE_PIX_FORMAT;

typedef struct _ImageParams {
    IMAGE_PIX_FORMAT mPixFmt;
    float mWidth;
    float mHeight;
    char *mFormatType;
    int   mAlign;
    /** 对应像素格式下，每个帧的空间大小 */
    int   mDataSize;
    int   mBitRate;
    /** 旋转角度 */
    int   mRotate;
} ImageParams;

/** 输入数据类型，裸数据还是压缩数据 */
typedef enum MEDIA_DATA_TYPE {
    MD_TYPE_UNKNOWN = 0,
    MD_TYPE_RAW_BY_FILE = 1,
    MD_TYPE_RAW_BY_MEMORY = 2,
    MD_TYPE_RAW_BY_PROTOCOL = 3,
    MD_TYPE_COMPRESS = 4,
} MEDIA_DATA_TYPE;

typedef enum STREAM_TYPE {
    CS_STREAM_TYPE_NONE      = 0,
    CS_STREAM_TYPE_VIDEO     = 0x01,
    CS_STREAM_TYPE_AUDIO     = 0x02,
    CS_STREAM_TYPE_DATA      = 0x04,
    CS_STREAM_TYPE_ALL       = 0x07,
} STREAM_TYPE;

#endif
