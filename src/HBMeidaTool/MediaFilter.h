#ifndef __MEDIAFILTERING_H__
#define  __MEDIAFILTERING_H__

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

};

#include <iostream>
#include <vector>
#include "FilterParm.h"

class MediaFilter;
class MediaFilterProgressListener {
public:
    virtual ~MediaFilterProgressListener() {}
    /**
     *  Call When editer began process video
     *
     *  @param editer CVideoEditerAny object.
     */
    virtual void MediaFilterProgressBegan(MediaFilter* editer) = 0;

    /**
     *  Call in progcess
     *
     *  @param editer   CVideoEditerAny object.
     *  @param progress
     *  @param total all duration.
     */
    virtual void MediaFilterProgressChanged(MediaFilter* editer, double progress, double total) = 0;

    /**
     *  Call finish video process!
     *
     *  @param editer  CVideoEditerAny object.
     */
    virtual void MediaFilterProgressEnded(MediaFilter* editer) = 0;

    /**
     *  Call canceled video process
     *
     *  @param editer CVideoEditerAny object
     */
    virtual void MediaFilterProgressCanceled(MediaFilter* editer) = 0;
};


class MediaFilter
{
public:
    MediaFilter();
    ~MediaFilter();

    int init();
    bool open(const char *filename);
    int setFFmpegLog();
    void close();
    /*加载文件*/
    int load(const char *filename);
    /*加载后的执行程序*/
    int process();
    /*中断程序*/
    bool abort();
    /*程序运行进度*/
    float progress(void);           /*获取程序运行的进度，当为1的时候则表示程序处理完毕*/
    /*设置水印*/
    int setWatermark(const char *filename, int wmPos_x, int wmPos_y,
                     int wmWidth, int wmHight, float begin, float duration);

    /*设置裁剪的坐标*/
    int setCropPos(int pos_x, int pos_y);
    /*设置裁剪的分辨率*/
    int setCropResolution(int crop_width, int crop_height);
    /*设置输出文件的模式,AV_SCALE_REGULAR,根据最大比例边进行缩放,AV_SCALE_MAX,
     根据最大比例边进行缩放,不足的部分进行颜色填充
     backgroudColor, AV_COLOR_BLOCK 0 黑色 AV_COLOR_White 1 白色
     */
    //int setOutModel(int model, int backgroudColor);
    /*设置裁剪的时间, 单位s*/
    int setCropTime(float begin, float end);
    /*设置输出文件的分辨率，如果不设置，如果有crop则采用crop后的分辨率，无则采用视频原分辨率*/
    int setOutResolution(int out_width, int out_height);
    /*可选设置，设置输出文件文件名，如果不设置，默认在文件后缀前加_filter标示*/
    int setOutFileName(const char *filename);
    /*获取视频显示的高*/
    int getMeidaShowHight();
    /*获取视频显示的宽*/
    int getMediaShowWidth();
    /*获取视频真实的高*/
    int getMediaRealHight();
    /*获取视频真实的宽*/
    int getMediaRealWidth();
    /*获取视频的时长单位us*/
    double getMediaDuration();
    /*获取视频旋转的角度*/
    int getMediaRotate();
    /*获取视频比特率*/
    long long getMediaVideoRate();
    /*获取音频比特率*/
    long long getMediaAudioRate();

    /* 获取视频平均帧率信息*/
    float getAverFrameRate();

    /* 获取视频的真实帧率 */
    float getRealFrameRate();

    /*设置对filter处理的进度监听*/
    void setProgressListener(MediaFilterProgressListener *listener);
    /*获取监听结构*/
    MediaFilterProgressListener *getProgressListener();
    /*快速截取视频*/
    void quickCropVideo(float start, float end);
    /**/
    //const std::pair<int, int> getRealOutputSize();
    /*缩放模式设置*/
    int setScaleModel(int model, int r, int g, int b);
    // 设置反转模式
    int setReverseMedia(int model);
    // 获取反转模式
    int getReverseMedia(void);

    int cancelReverseMedia(void);

    /*
     *   getFrameRGBASize与getFrameRGBAData配套使用,getFrameRGBASize获取到对应的size,
     *   然后自己开辟空间,传入接口获取数据
     */
    int getFrameRGBASize(int *videoWidth, int *videoHeight);

    /*获取RGBA的数据*/
    int getFrameRGBAData(float mediaPos, uint8_t data[], size_t len);
    /*添加需要拼接的视频*/
    int addConcatInVideo(const char *filename);
    /* 拼接视频 */
    int concatVideo(const char *outFilename);
    /* 获取拼接视频，处理后的视频的时长 */
    float *getConcatSegmentDuration();
    /* 获取片段总数 */
    int getSegmentCount();

    /*初始化输入文件，获取视频信息*/
    int initInFIle(const char *filename);

    /*初始化输出文件，不进行再次编码*/
    int initOutFileWithoutEncode(const char *filename);

    int getFileInfo(void);

    /*设置反转的视频时间区间*/
    int setReverseInterval(float start, float end);

    float getReverseEnd(void);

    float getReverseStart(void);

    // 视频剥离音频或视频
    int remuxStripMedia(const char *infilename, const char *dstfilename, int type);

    // 获取流时长
    long getVideoStreamDuration();
    long getAudioStreamDuration();

    // 设置最小边大小
    int setMinEage(int minEdge);

    // 获取缩略图
    int generateThumb(const char *src, const char* save_path, double times[], size_t length);

    // 设置音频编码格式 codecname 可设置“mp3” 或 “aac”, "copy"该编码的格式和输入的编码格式相同，默认编码aac
    int setAudioCodec(const char *codecname);

public:
    MediaFilterProgressListener *listener;

private:
#define MT_MAX_STREAM 8
    typedef struct _keyPts_t {
        int64_t audioPts;
        int64_t videoPts;
    }keyPts_t;

    typedef struct _frame_t{
        AVFrame *mediaFrame;
        enum AVMediaType type;
        int stream_index;
    } frame_t;

    typedef struct _FilterCtx_t {
        AVFilterGraph *filterGraph;
        AVFilterContext *buffersinkCtx;
        AVFilterContext *buffersrcCtx;
        enum AVMediaType type;
    } FilterCtx_t;

    std::vector<char *> filenameList;
    std::vector<keyPts_t *> keyFramePts;
    std::vector<WaterMark_t *> waterMarkList;
    FilterParm *parm;
    bool hasVideo;
    bool needCrop;
    int width;
    int height;
    char outFile[1024];
    int nb_streams;
    int filterModel;
    bool programStat;
    int m_Rotate;
    float mediaDuration;
    bool isInit;
    int64_t endVideoPts;
    float realFrameRate;
    float averFrameRate;

    AVFormatContext *ifmtCtx;
    AVFormatContext *ofmtCtx;
    FilterCtx_t *filterCtx;
    AVStream *inVideoStream;
    AVStream *inAudioStream;
    AVStream *outVideoStream;
    AVStream *outAudioStream;
    int audioIndex;
    int videoIndex;
    int64_t startTime[MT_MAX_STREAM];
    int64_t endTime[MT_MAX_STREAM];
    int needRead;

    int64_t duration[MT_MAX_STREAM];
    int64_t streamDuration[MT_MAX_STREAM];

    float processRate;
    long frame_cnt;
    long frame_cnt_a;

    AVAudioFifo *fifo = NULL;
    SwrContext *resample_context = NULL;
    bool audioResample = true;
    enum AVCodecID acodecId;
    bool isSynBaseOnAudio;
    int mediaNum;

    /*因为音频裁剪的原因重新校正视频长度，仅作为临时版本*/
    std::vector<int64_t> segmetduration;
    float *segmentTime;
    int concatSegmentCnt;
private:
    int initOutFile();
    int strInsert(const char *src, char *dst, int dst_len, char flag_c);
    void release();

    int freeKeyFramePts();
    int insertFrameQueue(std::vector<frame_t *> &frameQueue, AVFrame *p_frame, int mediaIndex, enum AVMediaType mediaType);
    int configFilterGraph(FilterCtx_t *filterCtx, const char *inName, char *outName, char *filterSpec);
    int initVideoFilter(FilterCtx_t *filterCtx, AVCodecContext *decCtx, AVCodecContext *enCtx, char *filterSpec, char *outBrif);
    int initAudioFilter(FilterCtx_t *filterCtx, AVCodecContext *decCtx, AVCodecContext *encCtx, char *filterSpec, char *outBrif);
    int autoAsembAudioFilterStr(char *asembStr, int strLen, char *outbrif, int outLen);
    int autoAsembVideoFilterStr(char *asembStr, int strLen, char *outStr, int outLen);
    int encodeWriteVideoFrame(AVFrame *filtFrame, int stream_index, int *gotFrame);
    int writePacket(AVPacket *pkt, int workType, int stream_index, int type);
    int encodeWriteFrame(const AVFrame *frame, int streamIndex, int *gotFrame);
    int flushDecoder(AVFormatContext *ifmtCtx, AVFrame *p_frame, unsigned int streamIndex, int *gotFrame);
    int frameReverse(std::vector<frame_t *>&frameQueue);
    int flushEncoder(AVFormatContext *fmtCtx, unsigned int streamIndex);
    int sectionReverse(int mediaIndex, int64_t start, int64_t end);
    int getKeyFramePts();
    int reverseMedia(int model);
    int initFilters(void);
    int resetAudioFilterDesc(int64_t start, int64_t end, char *filterDesc, int DescLen);
    int resetVideoReverse(int64_t start, int64_t end, char *filterDesc, int DescLen);
    int updateProgress(int64_t pts, int type, int stream_index);
	int initResampler(AVCodecContext *decodecCtx, AVCodecContext *encodecCtx,
		SwrContext **resampleCtx);

    
};

#endif /*__MEDIAFILTERING_H__*/
