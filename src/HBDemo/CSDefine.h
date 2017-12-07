#ifndef __HBDEFINE_H__
#define __HBDEFINE_H__

#include "CSVersion.h"
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

#define DECODE_STATE_UNKNOWN         0x0000
#define DECODE_STATE_READPKT_END     0x0001
#define DECODE_STATE_DECODE_END      0x0002
#define DECODE_STATE_READPKT_ABORT   0x0004
#define DECODE_STATE_DECODE_ABORT    0x0008
#define DECODE_STATE_FLUSH_MODE      0x0010
#define DECODE_STATE_PREPARED        0x0020

#define CS_RECORD_AUDIO_BUFFER  (81920)

#define SAFE_DELETE(p)           do { if(p) { delete (p); (p) = nullptr;} } while(0)
#define SAFE_DELETE_ARRAY(p)     do { if(p) { delete[] (p); (p) = nullptr; } } while(0)
#define SAFE_FREE(p)             do { if(p) { free(p); (p) = nullptr; } } while(0)
#define SAFE_RELEASE(p)          do { if(p) { (p)->release(); } } while(0)
#define SAFE_RELEASE_NULL(p)     do { if(p) { (p)->release(); (p) = nullptr; } } while(0)
#define SAFE_RETAIN(p)           do { if(p) { (p)->retain(); } } while(0)
#define BREAK_IF(cond)           if(cond) break

#define S_EQ(s,t)         ((s) & (t))
#define S_NOT_EQ(s,t)     (!((s) & (t)))

//#define SOUND_TOUCH_MODULE_EXCLUDE  1
//#define LIBYUV_MODULE_EXCLUDE  1

typedef struct KeyFramePts {
    int64_t audioPts;
    int64_t videoPts;
} KeyFramePts;

typedef enum ErrorCode {
    AV_STAT_ERR     = -100,
    AV_NOT_INIT     = -99,
    AV_FILE_ERR     = -98,
    AV_STREAM_ERR   = -97,
    AV_MALLOC_ERR   = -96,
    AV_DECODE_ERR   = -95,
    AV_SEEK_ERR     = -94,
    AV_PARM_ERR     = -93,
    AV_NOT_FOUND    = -92,
    AV_SET_ERR      = -91,
    AV_CONFIG_ERR   = -90,
    AV_ENCODE_ERR   = -89,
    AV_TS_ERR       = -88,
    AV_FIFO_ERR     = -87,
    AV_NOT_SUPPORT  = -86,
    AV_NOT_ENOUGH   = -85,
    AV_TRANSFER_ERR = -84,
    AV_EXIT_NORMAL       = 1,
} ErrorCode;

typedef enum PlayerState_t{
    S_PLAY_ERR   = -1,
    S_PLAY_INIT  = 0,
    S_PLAY_PROCESS = 1,
    S_PLAY_ABORT = 2,
    S_PLAY_PAUSE = 3,
    S_PLAY_END   = 4,
} PlayerState;

/*音频效果配置参数*/
typedef struct AudioEffectParam {
    float atempo;           // 速率（0.25～5）
    float pitch;            // 音调（0.25～2）
} AudioEffectParam;

typedef enum VideoEffectType {
    CS_VIDEO_SCALE_EFFECT   = 0,
    CS_VIDEO_CROP_ROTATE    = 1,
} VideoEffectType;

typedef enum AudioEffectType{
    CS_AUDIO_TEMPO_PITCH = 0,
    CS_AUDIO_RESAMPLER   = 1,
} AudioEffectType;

typedef enum VideoRorate_t {
    MT_Rotate0      = 0,  // No rotation.
    MT_Rotate90     = 90,  // Rotate 90 degrees clockwise.
    MT_Rotate180    = 180,  // Rotate 180 degrees.
    MT_Rotate270    = 270,  // Rotate 270 degrees clockwise.
} VideoRorate;

// 视频裁剪参数
typedef struct CropParam_t {
    int posX;
    int posY;
    int cropWidth;
    int cropHeight;
    int lineSize[4];
} CropParam;

typedef struct VideoEffectParam {
    CropParam *cropParam;
} VideoEffectParam;

// 视频裁剪参数
typedef struct ScaleParam_t {
    int inWidth;
    int inHeight;
    int outWidth;
    int outHeight;
    int scaleMode;   // 0~3越大，速度越慢，质量越高
    int lineSize[4];
} ScaleParam;

typedef enum MT_RECORD_STAT_t {
    MT_EXCEPTION            = -1,
    MT_IDEL                 = 0,
    MT_START                = 1,
    MT_PROCESS              = 2,
    MT_STOP                 = 3,
    MT_RELEASE              = 4,
    MT_AUDIO_ENCODE_START   = 5,
    MT_AUDIO_ENCODE_STOP    = 6,
    MT_VIDEO_ENCODE_START   = 7,
    MT_VIDEO_ENCODE_STOP    = 8,
    MT_WRITE_START          = 9,
    MT_WRITE_STOP           = 10,
} MT_RECORD_STAT_t;

typedef enum AUDIO_SAMPLE_FORMAT {
    CS_SAMPLE_FMT_NONE = 0,
    CS_SAMPLE_FMT_U8,          ///< unsigned 8 bits
    CS_SAMPLE_FMT_S16,         ///< signed 16 bits
    CS_SAMPLE_FMT_S32,         ///< signed 32 bits
    CS_SAMPLE_FMT_FLT,         ///< float
    CS_SAMPLE_FMT_DBL,         ///< double
    
    CS_SAMPLE_FMT_U8P,         ///< unsigned 8 bits, planar
    CS_SAMPLE_FMT_S16P,        ///< signed 16 bits, planar
    CS_SAMPLE_FMT_S32P,        ///< signed 32 bits, planar
    CS_SAMPLE_FMT_FLTP,        ///< float, planar
    
    CS_SAMPLE_FMT_NB           ///< Number of sample formats. DO NOT USE if linking dynamically
} AUDIO_SAMPLE_FORMAT;

typedef enum IMAGE_PIX_FORMAT {
    CS_PIX_FMT_NONE     = -1,
    CS_PIX_FMT_YUV420P = 0,
    CS_PIX_FMT_YUV422P  = 1,
    CS_PIX_FMT_YUV444P  = 2,
    CS_PIX_FMT_NV12     = 3,
    CS_PIX_FMT_NV21     = 4,
    CS_PIX_FMT_YUVJ420P = 5,
    CS_PIX_FMT_BGRA     = 6,
    CS_PIX_FMT_RGB8     = 7,
} IMAGE_PIX_FORMAT;

typedef struct _ImageParams {
    IMAGE_PIX_FORMAT mPixFmt;
    float mWidth;
    float mHeight;
    char *mFormatType;
    int   mAlign;
    /** 对应像素格式下，每个帧的空间大小 */
    int   mPreImagePixBufferSize;
    int64_t   mBitRate;
    float  mVideoCRF;
    float mFrameRate;
    /** 旋转角度 */
    int   mRotate;
} ImageParams;

typedef struct _AudioParams {
    int sample_rate;  /** 采样率 */
    int channels;
    int64_t channel_layout;
    AUDIO_SAMPLE_FORMAT pri_sample_fmt;
    
    /** frame_size & bytes_per_sec 使用意义待定 */
    int frame_size; /** 对应编码格式一帧有多少个samples */
    int bytes_per_sec;
    
    long mbitRate;  /** 比特率 */
    int   mAlign;
} AudioParams;

/** 输入数据类型，裸数据还是压缩数据 */
typedef enum MEDIA_DATA_TYPE {
    MD_TYPE_UNKNOWN = 0,
    MD_TYPE_RAW_BY_FILE = 1,
    MD_TYPE_RAW_BY_MEMORY = 2,
    MD_TYPE_COMPRESS = 3,
} MEDIA_DATA_TYPE;

typedef enum STREAM_TYPE {
    CS_STREAM_TYPE_NONE      = 0,
    CS_STREAM_TYPE_VIDEO     = 0x01,
    CS_STREAM_TYPE_AUDIO     = 0x02,
    CS_STREAM_TYPE_DATA      = 0x04,
    CS_STREAM_TYPE_ALL       = 0x07,
} STREAM_TYPE;

#define CS_COMMON_RESOURCE_ROOT_PATH "/Users/zj-db0519/work/resource/folder"

#endif
