#ifndef MULTIMEDIATOOLS_SRC_MAIN_JNI_MEDIA_FILTERPARM_H_
#define MULTIMEDIATOOLS_SRC_MAIN_JNI_MEDIA_FILTERPARM_H_

#ifndef INT64_MAX
#define INT64_MAX	0x7fffffffffffffff
#endif
#ifndef INT64_MIN
#define INT64_MIN        (-0x7fffffffffffffff - 1)
#endif
#include <vector>
#include "Error.h"

/*Filter 类型归集*/
enum  Filter_t{
	AV_NULL	    = 0,
	AV_ANULL	= 1,
    AV_CROP     = 2,
    AV_SCALE    = 3,
    AV_TRANSPOSE = 4,
    AV_WATERMARK = 5,
    AV_PAD		 = 6,
    AV_REVERSE	 = 7,
    AV_AREVERSE	 = 8,
    AV_TRIM		 = 9,
    AV_ATRIM     = 10,
    AV_HFLIP     = 11,
    AV_VFLIP     = 12,
    AV_ROUNDFLIP = 13,     /*先水平翻转，后纵向翻转*/
    AV_RROUNDFLIP = 14,  /*先纵向翻转，后水平翻转*/
};

enum ReverseModel_t {
	REVERSE_NONE		= -1,
	REVERSE_AUDIO_ONLY	= 0x01,
	REVERSE_VIDEO_ONLY	= 0x02,
	REVERSE_MEDIA		= 0x03,
};

typedef struct _filterStr_t {
    int type;
    char baseStr[128];
    char brif[8];
} FilterStr_t;

enum Filter_mode_t{
    AV_NORMAL         = 0x00,    /*对视频进行设定的长宽进行裁剪，不进行缩放*/
    AV_SCALE_REGULAR  = 0x01,     /*scale的固定模式，按照设定的输出大小，最大边和输出大小一致，另一边不足部分进行颜色填充。*/
    AV_SCALE_MAX      = 0x02,     /*scale的最大模式，根据最大比例的一边对视频进行缩放处理，另一边不足部分进行颜色填充*/
};

enum {
    AV_COLOR_BLACK = 0,
    AV_COLOR_WHITE = 1,
};

enum {
    AV_AUDIO = 0,
    AV_VIDEO = 1,
};

enum {
    MT_VIDEO_ROTATE_0 = 0,
    MT_VIDEO_ROTATE_90 = 90,
    MT_VIDEO_ROTATE_180 = 180,
    MT_VIDEO_ROTATE_270 = 270,
};

enum {
    CCLOCK_FLIP = 0,
    CLOCK       = 1,
    CCLOCK      = 2,
    CLOCK_FLIP  = 3,
};


typedef struct _WaterMark {
    char *filename;
    int wPosX;
    int wPosY;
    int wWidth;
    int wHight;
    double start;
    double end;
    //struct _WaterMark *wmNext;
    void *opque;
} WaterMark_t;


class FilterParm {

public:
    int filterMode;
    int realWidth;
    int realHeight;
    int showWidth;
    int showHeight;
    int cropPosX;
    int cropPosY;
    int cropWidth;
    int cropHeight;
    int scaleWidth;
    int scaleHeight;
    int flip[2];
    int transpose[2];
    float reverseStart;
    float reverseEnd;
    long long videoBitrate;
    long long audioBitrate;
    int outHeight;
    int outWidth;
    int rotate;
    float startTime;
    float endTime;
    float mediaDuration;
    int model;
    char* backgroudColor;
	int reverseModel;
	int minEdge;
};

#endif /*__FILTERPARM_H__*/