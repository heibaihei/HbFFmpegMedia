#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include "MediaFilter.h"
#include "FramePicker.h"

#include <fstream>
#if ANDROID
#include "LogHelper.h"
#include <android/log.h>
#else
#define LOGI printf
#define LOGE printf
#define LOGD printf
#endif

#define MAX_TASK 5
#define NEEDVIDEO 0x01
#define NEEDAUDIO 0x02

#define _DEBUG_PRINT printf
#define _4K_WIDTH 4096
#define _4K_HEIGHT 2160
#define PRECISION 0.00000001
#define HARDWARE_RATIO_MODEL 0
// fixed point to double
#define CONV_FP(x) ((double) (x)) / (1 << 16)
// double to fixed point
#define CONV_DB(x) (int32_t) ((x) * (1 << 16))

#ifndef makeErrorStr
static char errorStr[AV_ERROR_MAX_STRING_SIZE];
#define makeErrorStr(errorCode) av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, errorCode)
#endif

#define AAC_DECODE_ENCODE 1
#define WRITEYUV 1
#define _DEBUG 0
#define AV_RESAMPLE 1
enum {
    AV_WORK_NORMAL,         /*只解码编码不用到filter，当仅仅是时间裁剪时候用该模式*/
    AV_WORK_IN_FILTERING,   /*采用filter工作模式*/
    AV_WORK_SECTION,
};

enum {
    AV_IN = 0,
    AV_OUT,
};

/*各个avfilter的配置表*/
FilterStr_t filterMap[] = {
    {AV_NULL, "null", ""},
    {AV_ANULL, "anull", ""},
    {AV_CROP, "[%s]crop=w=%d:h=%d:x=%d:y=%d[cv]", "cv"},
    {AV_SCALE, "[%s]scale=%d:%d[sv]", "sv"},
    {AV_TRANSPOSE, "[%s]transpose=%d[tv_%d]", "tv_%d"},
    {AV_WATERMARK, "movie=%s,scale=%d:%d[wm_%d];[%s][wm_%d]overlay=%d:%d:"
        "enable=between(t\\,%.1f\\,%.1f)[wmv_%d]", "wmv_%d"},
    {AV_PAD, "[%s]pad=width=%d:height=%d:x=%d:y=%d:color=%s[pv]", "pv"},
    {AV_REVERSE, "[%s]reverse", "rv"},
    {AV_AREVERSE, "[%s]areverse", "arv"},
    {AV_TRIM, "[%s]trim=start_pts=%ld:end_pts=%ld", "trv"},
    {AV_ATRIM, "[%s]atrim=start_pts=%ld:end_pts=%ld", "atrv"},
    {AV_HFLIP, "[%s]hflip[hfv_%d]", "hfv_%d"},
    {AV_VFLIP, "[%s]vflip[vfv_%d]", "vfv_%d"},
    {AV_ROUNDFLIP, "[%s]hflip[hfv_%d],[hfv_%d]vflip[rfv_%d]", "rfv_%d"},
    {AV_RROUNDFLIP, "[%s]vflip[hfv_%d],[hfv_%d]hflip[rfv_%d]", "rrfv_%d"},
};

AVFrame *p_frame = NULL;
AVFrame *p_frameOut = NULL;
uint8_t **audioSamples = NULL;

void av_log_callback(void* ptr, int level, const char* fmt, va_list vl)
{
    char res[1024];
    vsprintf(res,fmt,vl);
    switch(level) {
        case AV_LOG_ERROR:
            LOGE("%s",res);
            break;
        default:
            LOGD("%s",res);
    }
}


MediaFilter::MediaFilter()
{
    hasVideo = false;
    audioIndex = -1;
    videoIndex = -1;
    fifo = NULL;
    programStat = false;
    m_Rotate = 0;
    mediaDuration = 0;
    nb_streams = 0;
    needCrop = false;
    needRead = 0x00;
    filterModel = -1;
    processRate = 0.0;
    isInit = false;
    listener = NULL;
    parm = NULL;
    frame_cnt = 0;
    frame_cnt_a = 0;
    ifmtCtx = NULL;
    ofmtCtx = NULL;
    filterCtx = NULL;
    inVideoStream = NULL;
    inAudioStream = NULL;
    outVideoStream = NULL;
    outAudioStream = NULL;
    width = 0;
    height = 0;
    isSynBaseOnAudio = true;
    averFrameRate = 0.0;
    segmentTime = NULL;
    memset(outFile, 0, sizeof(outFile));
}

MediaFilter::~MediaFilter()
{
    if (segmentTime) {
        free(segmentTime);
        segmentTime = NULL;
    }
}

int MediaFilter::init()
{
    int ret = 0;

    if (isInit) {
        ret = 0;
        goto TAR_OUT;
    }

    av_register_all();
    avcodec_register_all();
    avfilter_register_all();

    isInit = true;
    endVideoPts = 0L;


TAR_OUT:

    return ret;
}

int MediaFilter::setFFmpegLog()
{
#if ANDROID
    av_log_set_level(AV_LOG_INFO);
    //av_log_set_callback(av_log_callback);
#endif
    return 0;
}


int initFifo(AVAudioFifo **fifo, AVCodecContext *encodecCtx)
{
    if (fifo == NULL || encodecCtx == NULL) {
        return AV_PARM_ERR;
    }

    *fifo = av_audio_fifo_alloc(encodecCtx->sample_fmt,
                                encodecCtx->channels, 1);
    if (*fifo == NULL) {
        LOGE("Alloc audio fifo err!\n");
        return AV_MALLOC_ERR;
    }

    return 0;
}

int initOutputFrame(AVFrame **frame,
                    AVCodecContext *enCtx,
                    int frame_size)
{
    int ret;

    if (frame == NULL) {
        LOGE("Parmater err!\n");
        ret = AV_PARM_ERR;
        goto TAR_OUT;

    }
    if (*frame == NULL) {
        *frame = av_frame_alloc();
        if (*frame == NULL) {
            LOGE("Malloc frame err!\n");
            ret = AV_MALLOC_ERR;
            return ret;
        }
    }

    /**
     * Set the frame's parameters, especially its size and format.
     * av_frame_get_buffer needs this to allocate memory for the
     * audio samples of the frame.
     * Default channel layouts based on the number of channels
     * are assumed for simplicity.
     */
    (*frame)->nb_samples     = frame_size;
    (*frame)->channel_layout = enCtx->channel_layout;
    (*frame)->format         = enCtx->sample_fmt;
    (*frame)->sample_rate    = enCtx->sample_rate;

    /**
     * Allocate the samples of the created frame. This call will make
     * sure that the audio frame can hold as many samples as specified.
     */
    if ((ret = av_frame_get_buffer(*frame, 0)) < 0) {
        av_frame_free(frame);
        return ret;
    }

TAR_OUT:

    return ret;
}

int MediaFilter::getKeyFramePts()
{
    int ret;
    int curAudioPts = 0;
    static AVPacket pkt;
    keyPts_t *keyFramTs;
    int64_t reverseStart;
    int64_t reverseEnd;

    AVRational defaultTimebase;


    defaultTimebase.num = 1;
    defaultTimebase.den = AV_TIME_BASE;

    reverseStart = av_rescale_q((int64_t)(getReverseStart() * AV_TIME_BASE ),\
                                defaultTimebase, inVideoStream->time_base);
    reverseEnd = av_rescale_q((int64_t)(getReverseEnd() * AV_TIME_BASE ), \
                              defaultTimebase, inVideoStream->time_base);

    LOGE("[%d]reverseStart:reverseEnd [%lld][%lld]\n", __LINE__, reverseStart, reverseEnd);

    av_seek_frame(ifmtCtx, inVideoStream->index, reverseStart , AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(inVideoStream->codec);

    av_init_packet(&pkt);

    while (true) {
        ret = av_read_frame(ifmtCtx, &pkt);
        if (ret < 0) {
            break;
        }

        if (ifmtCtx->streams[pkt.stream_index]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (pkt.flags & AV_PKT_FLAG_KEY) {
                keyFramTs = new keyPts_t;
                if (keyFramTs == NULL) {
                    ret = AV_MALLOC_ERR;
                    goto TAR_OUT;
                }
                keyFramTs->videoPts = pkt.pts;
                keyFramTs->audioPts = curAudioPts;
                //keyFramePts.push_back(keyFramTs);
                keyFramePts.insert(keyFramePts.begin(), keyFramTs);
                if (0 != reverseStart && pkt.pts > reverseEnd) {
                    break;
                }
            }
        } else if (ifmtCtx->streams[pkt.stream_index]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            curAudioPts = pkt.pts;
        }
        av_packet_unref(&pkt);
    }

TAR_OUT:

    av_packet_unref(&pkt);

    return ret;
}

int MediaFilter::freeKeyFramePts()
{
    for (std::vector <keyPts_t *>::iterator it = keyFramePts.begin() ;
         it != keyFramePts.end() ; it++)
    {
        delete *it;
    }

    return 0;
}

int MediaFilter::initOutFileWithoutEncode(const char *filename)
{
    AVStream *inStream;
    AVStream *outStream;
    AVDictionary * dict = NULL;
    int ret = 0;
    int i;

    if (NULL != ofmtCtx) {
        goto TAR_OUT;
    }
    ret = avformat_alloc_output_context2(&ofmtCtx, NULL, NULL, filename);
    if (NULL == ofmtCtx) {
        ret = avformat_alloc_output_context2(&ofmtCtx, NULL, "mpeg", filename);
        if (ret < 0) {
            //LOGI( "Alloc output context err![%s]", ret);
            ret = AV_MALLOC_ERR;
            goto TAR_OUT;
        }
    }
    for (i=0; i<nb_streams; i++) {
        inStream = ifmtCtx->streams[i];
        outStream = avformat_new_stream(ofmtCtx, inStream->codec->codec);
        if (!outStream) {
            ret = AV_STREAM_ERR;
            goto TAR_OUT;
        }

        ret = avcodec_copy_context(outStream->codec, inStream->codec);
        if (ret < 0) {
            LOGE("Copy context err\n");
            ret = AV_ENCODE_ERR;
            goto TAR_OUT;
        }
        outStream->codec->codec_tag = 0;
        if (ofmtCtx->oformat->flags & AVFMT_GLOBALHEADER) {
            outStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
        av_dict_copy(&outStream->metadata, inStream->metadata, AV_DICT_DONT_OVERWRITE);
    }

    if (!(ofmtCtx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmtCtx->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGI("Could not open output file'%s'", filename);
            goto TAR_OUT;
        }
    }

    strcpy(ofmtCtx->filename, filename);
    av_dict_set(&dict, "movflags", "faststart", 0);
    ret = avformat_write_header(ofmtCtx, &dict);
    if (ret < 0) {
        LOGI("Write media header err![%d]", ret);
        ret = AV_FILE_ERR;
        goto TAR_OUT;
    }
    av_dict_free(&dict);

TAR_OUT:

    return ret;
}


/*
 * @brief: init out file and alloc  context to contral outfile
 * @arg: icrop_width: video crop width, crop_hight: video crop height,
 * begin: crop start time, end: crop end time
 * @return 0 is OK, other init error
 */
int MediaFilter::initOutFile()
{
    AVStream *inStream;
    AVStream *outStream;
    AVCodecContext *pCodecCtx;
    AVRational defaultTimebase;
    AVCodecContext *vEnCtx = NULL;
    AVCodecContext *aEnCtx = NULL;
    AVCodec *encodec;
    int64_t mediaDuration;
    AVDictionary * dict = NULL;
    AVDictionary *opts;
    int reverseModel;
    int ret;
    int i;

    ret = avformat_alloc_output_context2(&ofmtCtx, NULL, NULL, outFile);
    if (NULL == ofmtCtx) {
        ret = avformat_alloc_output_context2(&ofmtCtx, NULL, "mpeg", outFile);
        if (ret < 0) {
            LOGI( "Alloc output context err![%d]", ret);
            ret = AV_MALLOC_ERR;
            goto TAR_OUT;
        }
    }

    defaultTimebase.num = 1;
    defaultTimebase.den = AV_TIME_BASE;
    reverseModel = getReverseMedia();
    for (i=0; i<ifmtCtx->nb_streams; i++) {
        inStream  = ifmtCtx->streams[i];
        pCodecCtx = inStream->codec;
        if (pCodecCtx->codec_type == AVMEDIA_TYPE_VIDEO && vEnCtx == NULL) {
            if (reverseModel != 0 && reverseModel == REVERSE_AUDIO_ONLY) {
                continue;
            }
            encodec = avcodec_find_encoder(AV_CODEC_ID_H264);
            if (NULL == encodec) {
                LOGE("Cannot find encoder!");
                ret = AV_NOT_FOUND;
                goto TAR_OUT;
            }
            outVideoStream = avformat_new_stream(ofmtCtx, encodec);
            if (!outVideoStream) {
                LOGE("Failed allocating output stream");
                ret = AVERROR_UNKNOWN;
                goto TAR_OUT;
            }
            outVideoStream->start_time = 0;

            vEnCtx = outVideoStream->codec;
            vEnCtx->max_b_frames = pCodecCtx->max_b_frames;
            vEnCtx = outVideoStream->codec;
            vEnCtx->codec = encodec;
            vEnCtx->pix_fmt = pCodecCtx->pix_fmt;
            vEnCtx->codec_type = AVMEDIA_TYPE_VIDEO;
            vEnCtx->codec_id = AV_CODEC_ID_H264;//pCodecCtx->codec_id;
            //vEnCtx->codec_id = AV_CODEC_ID_MPEG4;

            vEnCtx->width = parm->outWidth;
            vEnCtx->height = parm->outHeight;

            vEnCtx->bit_rate = pCodecCtx->bit_rate;
            vEnCtx->time_base.den = pCodecCtx->time_base.den;
            vEnCtx->time_base.num = pCodecCtx->time_base.num;
            vEnCtx->qmin = pCodecCtx->qmin;
            vEnCtx->qmax = pCodecCtx->qmax;
            vEnCtx->keyint_min = pCodecCtx->keyint_min;
            vEnCtx->framerate = pCodecCtx->framerate;
            vEnCtx->sample_aspect_ratio = pCodecCtx->sample_aspect_ratio;
            vEnCtx->gop_size = pCodecCtx->gop_size;
            vEnCtx->bit_rate = pCodecCtx->bit_rate;

            if (ofmtCtx->oformat->flags & AVFMT_GLOBALHEADER) {
                vEnCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
            }
            av_dict_set(&opts, "profile", "baseline", 0);

            if (vEnCtx->codec_id == AV_CODEC_ID_H264) {
                av_opt_set(vEnCtx->priv_data, "level", "4.1", 0);
                av_opt_set(vEnCtx->priv_data, "preset", "superfast", 0);
                av_opt_set(vEnCtx->priv_data, "tune", "zerolatency", 0);
            }

            if ((ret = avcodec_open2(vEnCtx, encodec, &opts)) < 0) {
                LOGI("Cannot open video encoder[%d]",ret);
                goto TAR_OUT;
            }
            av_dict_free(&opts);
            hasVideo = true;
        } else if (pCodecCtx->codec_type == AVMEDIA_TYPE_AUDIO && aEnCtx == NULL) {
            if (reverseModel != 0 && reverseModel == REVERSE_VIDEO_ONLY) {
                continue;
            }
            encodec = avcodec_find_encoder(acodecId);

            outAudioStream = avformat_new_stream(ofmtCtx, encodec);
            if (!outAudioStream) {
                LOGI("Failed allocating output stream");
                ret = AVERROR_UNKNOWN;
                goto TAR_OUT;
            }
            outAudioStream->start_time = 0;
            aEnCtx = outAudioStream->codec;
#if 0
            ret = avcodec_copy_context(outAudioStream->codec, pCodecCtx);
            if (ret < 0) {
                LOGE("Failed to copy context from input to output stream codec context");
                goto TAR_OUT;
            }

            outAudioStream->codec->codec_tag = 0;
            if (ofmtCtx->oformat->flags & AVFMT_GLOBALHEADER) {
                outAudioStream->codec->codec_tag |= CODEC_FLAG_GLOBAL_HEADER;
            }
#else
            //aEnCtx->profile = FF_PROFILE_PRORES_HQ;
            aEnCtx->codec_id = acodecId;
            //aEnCtx->codec_id = pCodecCtx->codec_id;

            aEnCtx->codec_type = AVMEDIA_TYPE_AUDIO;

            if (acodecId == AV_CODEC_ID_MP3) {
                aEnCtx->sample_fmt = AV_SAMPLE_FMT_S16P;//encodec->sample_fmts[0];//AV_SAMPLE_FMT_S16;
            } else {
                aEnCtx->sample_fmt = AV_SAMPLE_FMT_S16;
            }
            aEnCtx->sample_rate = pCodecCtx->sample_rate;
            aEnCtx->channel_layout = pCodecCtx->channel_layout;
            aEnCtx->channels = pCodecCtx->channels;
            aEnCtx->bit_rate = 44100;
            aEnCtx->codec_tag = 0;

            aEnCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
            outAudioStream->time_base.den = pCodecCtx->sample_rate;
            outAudioStream->time_base.num = 1;
            av_dict_copy(&outAudioStream->metadata, inStream->metadata, AV_DICT_DONT_OVERWRITE);
#endif

            if (ofmtCtx->oformat->flags & AVFMT_GLOBALHEADER) {
                aEnCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
            }
            if ((ret = avcodec_open2(aEnCtx, encodec, NULL)) < 0) {
                LOGE("Cannot open video encoder[%d]",ret);
                goto TAR_OUT;
            }

#if AV_RESAMPLE
            ret = initResampler(pCodecCtx, aEnCtx, &resample_context);
            if (ret < 0) {
                LOGE("Init resample err!\n");
                goto TAR_OUT;
            }
            if (initFifo(&fifo, aEnCtx)) {
                ret = AV_MALLOC_ERR;
                LOGE("Init audio fifo err!\n");
                goto TAR_OUT;
            }
#endif
        } else {
            continue;
        }

        startTime[i] = av_rescale_q((int64_t)(parm->startTime * AV_TIME_BASE ),\
                                    defaultTimebase, inStream->time_base);
        endTime[i] = av_rescale_q((int64_t)(parm->endTime * AV_TIME_BASE ), \
                                  defaultTimebase, inStream->time_base);
        mediaDuration = av_rescale_q(inStream->duration, \
                                     inStream->time_base, defaultTimebase);
        if ((int64_t)(parm->endTime * AV_TIME_BASE) - mediaDuration > 0) {
            duration[i] = av_rescale_q(mediaDuration, \
                                       defaultTimebase, inStream->time_base);
            LOGI("duration[i]: [%lld]", duration[i]);
        } else {
            duration[i] = endTime[i];
        }
    }
    for (i=0; i<ofmtCtx->nb_streams; i++) {
        outStream  = ofmtCtx->streams[i];
        pCodecCtx = outStream->codec;
        if (pCodecCtx->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIndex = i;
        } else if (pCodecCtx->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioIndex = i;
        }
    }
    if (!(ofmtCtx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmtCtx->pb, outFile, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("Could not open output file'%s'", outFile);
            goto TAR_OUT;
        }
    }

    strcpy(ofmtCtx->filename, outFile);
    av_dict_set(&dict, "movflags", "faststart", 0);
    ret = avformat_write_header(ofmtCtx, &dict);
    if (ret < 0) {
        LOGE("Write media header err![%d]", ret);
        ret = AV_FILE_ERR;
        goto TAR_OUT;
    }
    av_dict_free(&dict);
TAR_OUT:

    return ret;
}

int MediaFilter::setCropResolution(int crop_width, int crop_height)
{
    int ret = 0;

    if (true != hasVideo || crop_width <= 0 || crop_height <= 0) {
        ret = AV_PARM_ERR;
        goto TAR_OUT;
    }

    if (parm->cropPosX < 0 || parm->cropPosY < 0 ||
        (parm->cropPosX+parm->cropWidth > parm->showWidth) ||
        (parm->cropPosY+parm->cropHeight > parm->showHeight)) {
        ret = AV_PARM_ERR;
        goto TAR_OUT;
    }

    parm->cropWidth = crop_width;
    parm->cropHeight = crop_height;

    if (parm->outWidth == parm->showWidth && \
        parm->outHeight == parm->showHeight) {
        parm->outWidth = crop_width;
        parm->outHeight = crop_height;
    }

TAR_OUT:

    return ret;
}

int MediaFilter::setCropPos(int pos_x, int pos_y)
{
    int ret = 0;

    if (true != hasVideo ||pos_x < 0 || pos_y < 0) {
        ret = AV_PARM_ERR;
        goto TAR_OUT;
    }
    if ((pos_x+parm->cropWidth > parm->showWidth) ||
        (pos_y+parm->cropHeight > parm->showHeight)) {
        LOGI( "Error !crop pos:[%d:%d], ratio [%d:%d]", \
             pos_x, pos_y, parm->cropWidth, parm->cropHeight);
        ret = AV_PARM_ERR;
        goto TAR_OUT;
    }
    parm->cropPosX = pos_x;
    parm->cropPosY = pos_y;

TAR_OUT:
    return ret;
}

int MediaFilter::setCropTime(float begin, float end)
{
    int ret = 0;

    if (!programStat) {
        ret = AV_STAT_ERR;
        goto TAR_OUT;
    }
    if (begin - end > PRECISION) {
        LOGI( "Error! crop time is illegal![%.2f~%.2f]", \
             begin, end);
        ret = AV_PARM_ERR;
        goto TAR_OUT;
    }
    parm->startTime = begin;
    parm->endTime = end;

    LOGI( "Crop time [%.2f~%.2f]", \
         parm->startTime,parm->endTime);

TAR_OUT:

    return ret;
}

int MediaFilter::setOutResolution(int out_width, int out_height)
{
    int ret = 0;

    if (true != hasVideo || out_width < 0 || out_height < 0) {
        ret = AV_PARM_ERR;
        goto TAR_OUT;
    }
    if ((parm->outHeight == out_height) && (parm->outWidth == out_width)) {
        LOGI("Video resolution ratio [%d:%d]", \
             out_width, out_height);
        ret = 0;
        goto TAR_OUT;
    }

#if HARDWARE_RATIO_MODEL
    if (out_height % 16 != 0) {
        parm->outHeight = (out_height / 16) * 16 + 16;
    } else {
        parm->outHeight = (out_height / 16) * 16;
    }
    if (out_width % 16 != 0) {
        parm->outWidth = (out_width / 16) * 16 + 16;
    } else {
        parm->outWidth = (out_width / 16) * 16;
    }
#else
    if ((out_height & 1) != 0) {
        parm->outHeight = out_height + 1;
        LOGE("Change height: %d", parm->outHeight);
    } else {
        parm->outHeight = out_height;
    }
    if ((out_width & 1) != 0) {
        parm->outWidth = out_width + 1;
        LOGE("Change width: %d", parm->outWidth);
    } else {
        parm->outWidth = out_width;
    }
#endif

TAR_OUT:

    return ret;
}

float MediaFilter::getRealFrameRate()
{
    return realFrameRate;
}

float MediaFilter::getAverFrameRate()
{
    return averFrameRate;
}

int MediaFilter::setOutFileName(const char *filename)
{
    int ret = 0;
    if (filename == NULL) {
        ret = AV_PARM_ERR;
        goto TAR_OUT;
    }
    memset(outFile, 0, sizeof(outFile));
    strncpy(outFile, filename, strlen(filename));

TAR_OUT:

    return ret;
}

int MediaFilter::setWatermark(const char *filename, int wmPos_x, int wmPos_y,
                              int wmWidth, int wmHight, float begin, float duration)
{
    AVFormatContext *testIfmtCtx = NULL;
    WaterMark_t *wm = NULL;
    int ret;

    if (true != hasVideo || filename == NULL \
        || wmPos_x < 0 || wmPos_y < 0 || begin < 0 \
        || duration < 0 || wmWidth <= 0 || wmHight <= 0) {
        LOGE("[setWatermark]Parm err!\n");
        ret = AV_PARM_ERR;
        goto TAR_OUT;
    }

    LOGD("Waterfile : %s", filename);
    ret = avformat_open_input(&testIfmtCtx, filename, NULL, NULL);
    if (ret < 0) {
        LOGE("Open input file err[%d]!", ret);
        ret = AV_FILE_ERR;
        goto TAR_OUT;
    }
    if (testIfmtCtx != NULL) {
        avformat_close_input(&testIfmtCtx);
    }

    wm = new WaterMark_t;
    if (NULL == wm) {
        LOGE("Malloc waterMark err!\n");
        ret = AV_MALLOC_ERR;
        goto TAR_OUT;
    }

    wm->filename = strdup(filename);
    wm->start = 0;              /*在ffmpeg中对应的配置时间是实际1500*/
    wm->end = (begin + duration) * 2600;
    wm->wHight = wmHight;
    wm->wWidth = wmWidth;
    wm->wPosX = wmPos_x;
    wm->wPosY = wmPos_y;

    waterMarkList.push_back(wm);
TAR_OUT:

    return ret;
}

int MediaFilter::setReverseInterval(float start, float end)
{
    if (start > end) {
        return AV_PARM_ERR;
    }

    if (parm) {
        parm->reverseStart = start;
        parm->reverseEnd = end;

        return 0;
    }

    return AV_STAT_ERR;
}

int MediaFilter::setScaleModel(int model, int r, int g, int b)
{
    int ret = 0;
    char color[32];

    if (true != hasVideo || r > 255 ||g > 255 || b > 255) {
        ret = AV_PARM_ERR;
        goto TAR_OUT;
    }

    parm->filterMode = model;

    snprintf(color, sizeof(color), "0x%02X%02X%02X", r, g, b);

    if (parm->backgroudColor != NULL) {
        free(parm->backgroudColor);
    }

    parm->backgroudColor = strndup(color, strlen(color));

TAR_OUT:

    return ret;
}

double MediaFilter::getMediaDuration()
{
    if (programStat) {
        return (double)parm->mediaDuration / 1000000;
    }

    return 0;
}

int MediaFilter::getMeidaShowHight()
{
    if (programStat) {
        return parm->showHeight;
    }

    return 0;
}

int MediaFilter::getMediaShowWidth()
{
    if (programStat) {
        return parm->showWidth;
    }

    return 0;
}

int MediaFilter::getMediaRealHight()
{
    if (programStat) {
        return parm->realHeight;
    }

    return 0;
}

int MediaFilter::getMediaRealWidth()
{
    if (programStat) {
        return parm->realWidth;
    }

    return 0;
}

int MediaFilter::getMediaRotate()
{
    if (programStat) {
        return parm->rotate;
    }

    return 0;
}

int64_t MediaFilter::getMediaVideoRate()
{
    if (programStat) {
        return parm->videoBitrate;
    }

    return 0;
}

int64_t MediaFilter::getMediaAudioRate()
{
    if (programStat) {
        return parm->audioBitrate;
    }

    return 0;
}

void MediaFilter::setProgressListener(MediaFilterProgressListener* listener)
{
    this->listener = listener;
}

MediaFilterProgressListener* MediaFilter::getProgressListener()
{
    return this->listener;
}


double getDisplayRotation(const int32_t matrix[9])
{
    double rotation, scale[2];

    scale[0] = hypot(CONV_FP(matrix[0]), CONV_FP(matrix[3]));
    scale[1] = hypot(CONV_FP(matrix[1]), CONV_FP(matrix[4]));

    if (scale[0] == 0.0 || scale[1] == 0.0)
        return NAN;

    rotation = atan2(CONV_FP(matrix[1]) / scale[1],
                     CONV_FP(matrix[0]) / scale[0]) * 180 / M_PI;

    return -rotation;
}

double getDisplayMatrix(AVStream *st)
{
    AVDictionaryEntry *rotate_tag = av_dict_get(st->metadata, "rotate", NULL, 0);
    uint8_t* displaymatrix = av_stream_get_side_data(st,
                                                     AV_PKT_DATA_DISPLAYMATRIX, NULL);
    double theta = 0;

    if (rotate_tag && *rotate_tag->value && strcmp(rotate_tag->value, "0")) {
        char *tail;
        theta = strtod(rotate_tag->value, &tail);
        if (*tail) {
            theta = 0;
        }
    }

    if (displaymatrix) {
        theta = -getDisplayRotation((int32_t*) displaymatrix);
    }

    theta -= 360*floor(theta/360 + 0.9/360);
    if (fabs(theta - 90*round(theta/90)) > 2) {
        theta = 0;
    }

    return theta;
}

int getRotate(AVDictionary *metadata)
{
    AVDictionaryEntry *tag = NULL;
    int m_Rotate;
    int angle;

    tag = av_dict_get(metadata, "rotate", tag, 0);

    if (tag == NULL) {
        m_Rotate = MT_VIDEO_ROTATE_0;
        goto TAR_OUT;
    }

    angle = atoi(tag->value);
    angle %= 360;

    switch(angle) {
        case 90:
            m_Rotate = MT_VIDEO_ROTATE_90;
            break;
        case 180:
            m_Rotate = MT_VIDEO_ROTATE_180;
            break;
        case 270:
            m_Rotate = MT_VIDEO_ROTATE_270;
            break;
        default:
            m_Rotate = MT_VIDEO_ROTATE_0;
            break;
    }

TAR_OUT:

    return m_Rotate;
}

int MediaFilter::setReverseMedia(int model)
{
    if (programStat) {
        parm->reverseModel = model;
    } else {
        return AV_STAT_ERR;
    }
    return 0;
}

int MediaFilter::setMinEage(int minEdge)
{
    if (programStat) {
        parm->minEdge = minEdge;
    } else {
        return AV_STAT_ERR;
    }
    return 0;
}

int MediaFilter::getReverseMedia(void)
{
    if (programStat) {
        return parm->reverseModel;
    } else {
        return AV_STAT_ERR;
    }
}

int MediaFilter::cancelReverseMedia(void)
{
    if (programStat) {
        parm->reverseModel = REVERSE_NONE;
    } else {
        return AV_STAT_ERR;
    }

    return 0;
}

float MediaFilter::getReverseStart(void)
{
    if (programStat) {
        return parm->reverseStart;
    } else {
        return 0.0;
    }
}

float MediaFilter::getReverseEnd(void)
{
    if (programStat) {
        return parm->reverseEnd;
    } else {
        return 0.0;
    }
}

bool MediaFilter::open(const char *filename)
{
    int ret;

    ret = load(filename);
    if (ret < 0) {
        return false;
    }
    return true;
}

void MediaFilter::close()
{
    return release();
}

void setMediaRatio(FilterParm *parm, int m_Rotate, int width, int height)
{

    parm->realWidth = width;
    parm->realHeight = height;

    parm->rotate = m_Rotate;
    if (m_Rotate == MT_VIDEO_ROTATE_90 || \
        m_Rotate == MT_VIDEO_ROTATE_270) {
        parm->showHeight = width;
        parm->showWidth = height;
        parm->outHeight = width;
        parm->outWidth = height;
    } else {
        parm->showHeight = height;
        parm->showWidth = width;
        parm->outHeight = height;
        parm->outWidth = width;
    }

}

int MediaFilter::initInFIle(const char *filename)
{
    int ret;
    ret = avformat_open_input(&ifmtCtx, filename, NULL, NULL);
    if (ret < 0) {
        LOGE("Open input file err[%d]!", ret);
        ret = AV_FILE_ERR;
        goto TAR_OUT;
    }
    ret = avformat_find_stream_info(ifmtCtx, NULL);
    if (ret < 0) {
        ret = AV_STREAM_ERR;
        LOGE("Cannot find any stream!");
        goto TAR_OUT;
    }
    nb_streams = ifmtCtx->nb_streams;
TAR_OUT:

    return ret;
}

int MediaFilter::getFileInfo(void)
{
    AVCodecContext *pCodecCtx;
    AVStream *pStream;
    AVMediaType codecType;
    int ret = 0;
    int i;

    for (i=0; i<nb_streams; i++) {
        pStream = ifmtCtx->streams[i];
        pCodecCtx = pStream->codec;
        codecType = pCodecCtx->codec_type;

        if (codecType == AVMEDIA_TYPE_VIDEO) {
            inVideoStream = pStream;
            hasVideo = true;
            needRead |= NEEDVIDEO;
            m_Rotate = getRotate(inVideoStream->metadata);
            LOGE("getRotate\n");
            //parm->rotate = getTranspose(m_Rotate);
            setMediaRatio(parm, m_Rotate, pCodecCtx->width, pCodecCtx->height);
            //parm->videoBitrate = pCodecCtx->bit_rate;
        } else if (codecType == AVMEDIA_TYPE_AUDIO) {
            inAudioStream = pStream;
            needRead |= NEEDAUDIO;
            parm->audioBitrate = inAudioStream->codec->bit_rate;
        } else {
            LOGI("Unsuport media type! type id[%d]", codecType);
            continue;
        }

        if ((ret = avcodec_open2(pStream->codec,\
                                 avcodec_find_decoder(pCodecCtx->codec_id), NULL)) < 0) {
            LOGE("Cannot open decoder[%s]", avcodec_get_name(pCodecCtx->codec_id));
            goto TAR_OUT;
        }
    }

    parm->mediaDuration = ifmtCtx->duration;

TAR_OUT:

    for (i=0; i<nb_streams; i++) {
        if (ifmtCtx->streams[i]->codec) {
            avcodec_close(ifmtCtx->streams[i]->codec);
        }
    }

    return ret;
}

int getTranspose(int rotate)
{
    int ret;

    switch (rotate) {
        case MT_VIDEO_ROTATE_90:
            ret = 1;
            break;
        case MT_VIDEO_ROTATE_180:
            ret = 4;
            break;
        case MT_VIDEO_ROTATE_270:
            ret = 2;
            break;
        default:
            ret = 0;
            break;
    }

    return ret;
}

int getRotateParm(AVStream *st, int transpose[], int flip[])
{
    if (NULL == st) {
        return AV_PARM_ERR;
    }

    AVDictionaryEntry *rotate_tag = NULL;
    uint8_t* displaymatrix = NULL;
    double theta = 0;

    rotate_tag = av_dict_get(st->metadata, "rotate", NULL, 0);
    displaymatrix = av_stream_get_side_data(st, AV_PKT_DATA_DISPLAYMATRIX, NULL);
    if (rotate_tag && *rotate_tag->value && strcmp(rotate_tag->value, "0")) {
        if (!strcmp(rotate_tag->value, "90")) {
            transpose[0] = CLOCK;
        } else if (!strcmp(rotate_tag->value, "180")) {
            flip[0] = AV_ROUNDFLIP;
        } else if (!strcmp(rotate_tag->value, "270")) {
            transpose[0] = CCLOCK;
        } else {
            char rotate_buf[64];
            snprintf(rotate_buf, sizeof(rotate_buf), "%s*PI/180", rotate_tag->value);
            LOGE("%s*PI/180\n", rotate_tag->value);
        }
    } else if (displaymatrix) {
        double rot = getDisplayRotation((int32_t*) displaymatrix);
        if (rot < -135 || rot > 135) {
            LOGE("vflip hflip\n");
            flip[0] = AV_RROUNDFLIP;
        } else if (rot < -45) {
            LOGE("transpose dir=clock\n");
            transpose[0] = CLOCK;
        } else if (rot > 45) {
            LOGE("transpose dir=cclock\n");
            transpose[0] = CCLOCK;
        }
        char *tail;
        theta = strtod(rotate_tag->value, &tail);
        if (*tail) {
            theta = 0;
        }
    }
    return 0;
}

long MediaFilter::getAudioStreamDuration()
{
    return streamDuration[audioIndex];
}

long MediaFilter::getVideoStreamDuration()
{
    return streamDuration[videoIndex];
}

int MediaFilter::setAudioCodec(const char *codecname)
{
    if (strcasecmp("mp3", codecname) == 0) {
        acodecId = AV_CODEC_ID_MP3;
        return 0;
    } else if (strcasecmp("aac", codecname) == 0) {
        acodecId = AV_CODEC_ID_AAC;
        return 0;
    } else {
        acodecId = AV_CODEC_ID_AAC;
        return 1;
    }
}

int MediaFilter::load(const char *filename)
{
    AVCodecContext *pCodecCtx;
    AVStream *pStream;
    AVMediaType codecType;
    int ret = 0;
    int i;
    int displaymatrix;
    AVRational defaultTimebase;

    if (NULL == filename) {
        ret = AV_FILE_ERR;
        goto TAR_OUT;
    }

    if (programStat) {
        LOGI( "Reload file!");
        release();
    }
    init();

    if (parm) {
        delete parm;
        parm = NULL;
    }
    parm = new FilterParm;
    if (NULL == parm) {
        ret = AV_MALLOC_ERR;
        goto TAR_OUT;
    }
    memset(parm, 0, sizeof(FilterParm));
    parm->startTime = 0;
    parm->endTime = 9999999;
    parm->reverseEnd = 999999.999;
    parm->reverseModel = 0;
    parm->transpose[0] = -1;
    parm->transpose[1] = -1;

    filterModel = -1;
    acodecId = AV_CODEC_ID_AAC;

    LOGI("[load]Open input file %s!", filename);
    ret = avformat_open_input(&ifmtCtx, filename, NULL, NULL);
    if (ret < 0) {
        LOGE("Open input file err[%d]!", ret);
        ret = AV_FILE_ERR;
        goto TAR_OUT;
    }

    ret = avformat_find_stream_info(ifmtCtx, NULL);
    if (ret < 0) {
        ret = AV_STREAM_ERR;
        LOGE("Cannot find any stream!");
        goto TAR_OUT;
    }
    parm->endTime = ifmtCtx->duration;
    nb_streams = ifmtCtx->nb_streams;
    LOGI("nb_streams: %d", nb_streams);
    if (nb_streams > MT_MAX_STREAM) {
        ret = AV_FILE_ERR;
        LOGE("Warning : Too many streams in this input file! %d streams", nb_streams);
        goto TAR_OUT;
    }

    defaultTimebase.num = 1;
    defaultTimebase.den = AV_TIME_BASE;

    for (i=0; i<nb_streams; i++) {
        pStream = ifmtCtx->streams[i];
        pCodecCtx = pStream->codec;
        codecType = pCodecCtx->codec_type;

        if (codecType == AVMEDIA_TYPE_VIDEO && inVideoStream == NULL) {
            inVideoStream = pStream;
            hasVideo = true;
            needRead |= NEEDVIDEO;
            LOGI("Find video stream!");
            getRotateParm(inVideoStream, parm->transpose, parm->flip);
            m_Rotate = getRotate(inVideoStream->metadata);
            displaymatrix = getDisplayMatrix(inVideoStream);
            setMediaRatio(parm, m_Rotate, pCodecCtx->width, pCodecCtx->height);
            parm->videoBitrate = pCodecCtx->bit_rate;
            averFrameRate = inVideoStream->avg_frame_rate.num * 1.0 / inVideoStream->avg_frame_rate.den;
            realFrameRate = inVideoStream->r_frame_rate.num *1.0 /inVideoStream->r_frame_rate.den;
            videoIndex = i;
        } else if (codecType == AVMEDIA_TYPE_AUDIO && inAudioStream == NULL) {
            inAudioStream = pStream;
            LOGI("Find audio stream!");
            needRead |= NEEDAUDIO;
            parm->audioBitrate = inAudioStream->codec->bit_rate;
            pCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
            audioIndex = i;
        } else {
            LOGE("Unsuport media type! type id[%d]", codecType);
            continue;
        }

        streamDuration[i] = av_rescale_q(pStream->duration, \
                                         pStream->time_base, defaultTimebase);
        if ((ret = avcodec_open2(pStream->codec,\
                                 avcodec_find_decoder(pCodecCtx->codec_id), NULL)) < 0) {
            LOGE("Cannot open decoder");
            goto TAR_OUT;
        }
    }
    parm->mediaDuration = ifmtCtx->duration;
    memset(outFile, 0, sizeof(outFile));
    /*Auto generate out file, filename_filtes.suffix*/
    ret = strInsert(filename, outFile, sizeof(outFile), '.');
    if (ret < 0) {
        LOGE("strInsert err: %d", ret);
        goto TAR_OUT;
    }

    LOGI("needRead : %d", needRead);
    programStat = true;
    processRate = 0.0;

TAR_OUT:
    if (ret < 0) {
        if (NULL == parm) {
            delete parm;
            parm = NULL;
        }
        if(ifmtCtx != NULL) {
            LOGI("Close input file[%d]", ret);
            avformat_close_input(&ifmtCtx);
        }
    }
    return ret;
}

int MediaFilter::encodeWriteFrame(const AVFrame *frame, int streamIndex, int *gotFrame)
{
    int ret;
    int got_frame_local;
    AVPacket enc_pkt;
    AVCodecContext *deCtx;
    AVCodecContext *enCtx;
    AVStream *in_stream, *out_stream;
    int pktIndex;
    int (*encode)(AVCodecContext*, AVPacket*, const AVFrame*, int*) = NULL;

    /* encode filtered frame */
    in_stream = ifmtCtx->streams[streamIndex];
    deCtx = in_stream->codec;
    if (deCtx->codec_type == AVMEDIA_TYPE_AUDIO) {
        encode = avcodec_encode_audio2;
        pktIndex = audioIndex;
    } else if (deCtx->codec_type == AVMEDIA_TYPE_VIDEO) {
        encode = avcodec_encode_video2;
        pktIndex = videoIndex;
    } else {
        ret = 0;
        av_log(NULL, AV_LOG_WARNING, "Unsupport media type\n");
        goto TAR_OUT;
    }

    out_stream = ofmtCtx->streams[pktIndex];
    enCtx = out_stream->codec;

    if (!gotFrame) {
        gotFrame = &got_frame_local;
    }

    enc_pkt.data = NULL;
    enc_pkt.size = 0;
    av_init_packet(&enc_pkt);
    ret = encode(enCtx, &enc_pkt, frame, gotFrame);
    if (ret < 0) {
        ret = AV_ENCODE_ERR;
        goto TAR_OUT;
    }
    if (0 == (*gotFrame)) {
        goto TAR_OUT;
    }
    enc_pkt.stream_index = pktIndex;
    av_packet_rescale_ts(&enc_pkt,
                         in_stream->time_base,
                         out_stream->time_base);
    /* mux encoded frame */
    ret = av_interleaved_write_frame(ofmtCtx, &enc_pkt);
    if (ret < 0) {
        LOGE("Interleaved frame err![%d]", ret);
        goto TAR_OUT;
    }

TAR_OUT:

    av_packet_unref(&enc_pkt);

    return ret;
}

int MediaFilter::encodeWriteVideoFrame(AVFrame *frame, int streamIndex, int *gotFrame)
{
    int ret;
    int got_frame_local;
    AVPacket enc_pkt;
    AVCodecContext *enCtx;
    AVStream *in_stream, *out_stream;

    in_stream = ifmtCtx->streams[streamIndex];
    out_stream = ofmtCtx->streams[videoIndex];
    if (!gotFrame) {
        gotFrame = &got_frame_local;
    }
    enCtx = out_stream->codec;
    /* encode filtered frame */
    enc_pkt.data = NULL;
    enc_pkt.size = 0;
    av_init_packet(&enc_pkt);
    ret = avcodec_encode_video2(enCtx, &enc_pkt, frame, gotFrame);
    if (ret < 0) {
        ret = AV_ENCODE_ERR;
        LOGI("Encode video err![%d]", ret);
        goto TAR_OUT;
    }

    if (0 == (*gotFrame)) {
        goto TAR_OUT;
    }
    /* prepare packet for muxing */
    enc_pkt.stream_index = videoIndex;
    av_packet_rescale_ts(&enc_pkt,
                         in_stream->time_base,
                         out_stream->time_base);
    /* mux encoded frame */
    ret = av_interleaved_write_frame(ofmtCtx, &enc_pkt);
    if (ret < 0) {
        LOGI("Muxing file err![%d]", ret);
        goto TAR_OUT;
    }

TAR_OUT:

    av_packet_unref(&enc_pkt);

    return ret;
}

int MediaFilter::initResampler(AVCodecContext *decodecCtx, AVCodecContext *encodecCtx,
                               SwrContext **resampleCtx)
{
    int ret;

    *resampleCtx = swr_alloc_set_opts(NULL, av_get_default_channel_layout(encodecCtx->channels),
                                      encodecCtx->sample_fmt, encodecCtx->sample_rate, av_get_default_channel_layout(decodecCtx->channels),
                                      decodecCtx->sample_fmt, decodecCtx->sample_rate, 0, NULL);
    if (*resampleCtx == NULL) {
        LOGE("Alloc resample context err!\n");
        ret = AV_MALLOC_ERR;
        goto TAR_OUT;
    }
    av_assert0(decodecCtx->sample_rate == encodecCtx->sample_rate);

    ret = swr_init(*resampleCtx);
    if (ret < 0) {
        LOGE("Init swresample err!\n");
        swr_free(resampleCtx);
    }

TAR_OUT:

    return ret;
}

uint8_t **initConvertSample(AVCodecContext *encodecCtx, int frame_size)
{
    uint8_t **inputFrame = NULL;
    int ret;

    inputFrame = (uint8_t **)av_calloc(encodecCtx->channels, sizeof(**inputFrame));
    if (inputFrame == NULL) {
        goto TAR_OUT;
    }

    ret = av_samples_alloc(inputFrame, NULL, encodecCtx->channels, frame_size,
                           encodecCtx->sample_fmt, 0);
    if (ret < 0) {
        LOGE("Alloc sample err!\n");
        av_freep(&(inputFrame)[0]);
        free(inputFrame);
        inputFrame = NULL;
    }

TAR_OUT:

    return inputFrame;
}

int addSamplesToFifo(AVAudioFifo *fifo,
                     uint8_t **inputSamples,
                     const int frame_size)
{
    int ret;

    ret = av_audio_fifo_realloc(fifo, frame_size);
    if (ret < 0) {
        LOGE("Audio fifo realloc err!\n");
        ret = AV_MALLOC_ERR;
        goto TAR_OUT;
    }

    ret = av_audio_fifo_write(fifo, (void **)inputSamples, frame_size);
    if (ret < frame_size) {
        LOGE("Audio fifo write data err![%d]\n", ret);
        goto TAR_OUT;
    }

TAR_OUT:

    return ret;
}

int MediaFilter::updateProgress(int64_t pts, int type, int stream_index)
{
    if (hasVideo) {
        if (type == AVMEDIA_TYPE_VIDEO && listener) {
            processRate = (float)pts / duration[stream_index];
            if (processRate > 1.0) {
                processRate = 1.0;
            }

            listener->MediaFilterProgressChanged(this, processRate, 1.0);
        }
    } else {
        processRate = (float)pts / duration[stream_index];
        if (processRate > 1.0) {
            processRate = 1.0;
        }
        if (listener != NULL) {
            listener->MediaFilterProgressChanged(this, processRate, 1.0);
        }
    }

    return 0;
}

int MediaFilter::writePacket(AVPacket *pkt, int workType, int stream_index, int type)
{
    int ret = 0;
    int got_frame;
    AVStream *in_stream;
    AVStream *out_stream;
    AVCodecContext *DeCtx;
    AVCodecContext *enCtx;
    AVFilterContext *buffersrcCtx;
    AVFilterContext *buffersinkCtx;
    int outFrameSize = 0;
    AVFrame *p_frame = NULL;
    AVFrame *p_frameOut = NULL;
    int frame_size;

    if (stream_index >= MT_MAX_STREAM - 1) {
        //av_log(NULL, AV_LOG_WARNING, "stream index: %d", stream_index);
        return 0;
    }
    p_frame = av_frame_alloc();
    if (NULL == p_frame) {
        LOGE( "Alloc frame err!");
        ret = AV_MALLOC_ERR;
        goto TAR_OUT;
    }
    p_frameOut = av_frame_alloc();
    if (NULL == p_frameOut) {
        LOGE( "Alloc out frame err!");
        ret = AV_MALLOC_ERR;
        goto TAR_OUT;
    }
    in_stream  = ifmtCtx->streams[stream_index];
    DeCtx = in_stream->codec;
    buffersrcCtx = filterCtx[stream_index].buffersrcCtx;
    buffersinkCtx = filterCtx[stream_index].buffersinkCtx;

    updateProgress(pkt->pts, type, stream_index);

    //encodeWriteFrame();
    /*视频数据处理入口*/
    if (type == AVMEDIA_TYPE_VIDEO) {
        out_stream = ofmtCtx->streams[videoIndex];
        got_frame = 0;
        ret = avcodec_decode_video2(DeCtx, p_frame, &got_frame, pkt);
        if (ret < 0) {
            ret = AV_ENCODE_ERR;
            LOGI( "Error decoding video");
            goto TAR_OUT;
        }
        if (0 == got_frame) {
            goto TAR_OUT;
        }
        p_frame->pts = av_frame_get_best_effort_timestamp(p_frame);
        if (p_frame->pts > endTime[stream_index]) {
            needRead &= ~NEEDVIDEO;
            ret = 0;
            goto TAR_OUT;
        } else if (p_frame->pts < startTime[stream_index]) {
            ret = 0;
            goto TAR_OUT;
        }
        if (workType == AV_WORK_IN_FILTERING) {
            p_frame->pts -= startTime[stream_index];
            /*将解码数据加入到filter的缓存队列中*/
            if (av_buffersrc_add_frame(buffersrcCtx, p_frame) < 0) {
                LOGE( "Error while feeding the filtergraph");
                goto TAR_OUT;
            }

            while (true) {
                /*从filter的缓存队列中获取经过filter处理的数据*/
                ret = av_buffersink_get_frame(buffersinkCtx, p_frameOut);
                if (ret < 0) {
                    break;
                }
                ret = encodeWriteFrame(p_frameOut, stream_index, NULL);
                if (ret < 0) {
                    LOGI( "[%d]Write video frame err![%d]", __LINE__, ret);
                    break;
                }
                av_frame_unref(p_frameOut);
            }
            av_frame_unref(p_frame);
        } else {
            ret = encodeWriteFrame(p_frame, stream_index, NULL);
            if (ret < 0) {
                LOGI( "[%d]Write video frame err![%d]", __LINE__, ret);
            }
            av_frame_unref(p_frame);
        }

    } else if (type == AVMEDIA_TYPE_AUDIO) {
#if AAC_DECODE_ENCODE
        out_stream = ofmtCtx->streams[audioIndex];
        enCtx = out_stream->codec;
        /*Use the encoder's desired frame size for processing*/
        outFrameSize = enCtx->frame_size;
        if (audioResample  || acodecId != enCtx->codec_id) {
            ret = avcodec_decode_audio4(DeCtx, p_frame, &got_frame, pkt);
            if (ret < 0) {
                ret = AV_ENCODE_ERR;
                LOGE( "Error decoding video");
                goto TAR_OUT;
            }
            if (0 == got_frame) {
                goto TAR_OUT;
            }
            if (p_frame->nb_samples == outFrameSize) {
                audioResample = false;
            }
            p_frame->pts = av_frame_get_best_effort_timestamp(p_frame);
            if (p_frame->pts > endTime[stream_index]) {
                needRead &= ~NEEDAUDIO;
                ret = 0;
                LOGE("time [%lld][%lld]\n", p_frame->pts, endTime[stream_index]);
                goto TAR_OUT;
            } else if (p_frame->pts < startTime[stream_index]) {
                ret = 0;
                goto TAR_OUT;
            }

            if (av_audio_fifo_size(fifo) < outFrameSize) {
                if (audioSamples == NULL) {
                    audioSamples = initConvertSample(DeCtx, p_frame->nb_samples);
                    if (audioSamples == NULL) {
                        LOGE("Audio sample is null");
                        ret = AV_MALLOC_ERR;
                        goto TAR_OUT;
                    }
                }
                ret = swr_convert(resample_context, audioSamples,
                                  p_frame->nb_samples,
                                  (const uint8_t **)p_frame->extended_data,
                                  p_frame->nb_samples);
                if (ret < 0) {
                    LOGE("Conver audio err!\n");
                    goto TAR_OUT;
                }

                ret = addSamplesToFifo(fifo, audioSamples, p_frame->nb_samples);
                if (ret < 0) {
                    LOGE("Add sample to fifo err!\n");
                    goto TAR_OUT;
                }
            }

            if (av_audio_fifo_size(fifo) >= outFrameSize) {
                while (true) {
                    if (av_audio_fifo_size(fifo) < outFrameSize) {
                        break;
                    }
                    frame_size = FFMIN(av_audio_fifo_size(fifo), outFrameSize);
                    initOutputFrame(&p_frameOut, enCtx, outFrameSize);
                    ret = av_audio_fifo_read(fifo, (void**)p_frameOut->data, frame_size);
                    if (ret < outFrameSize) {
                        LOGE("Read audio fifo err! read size[%d]\n", ret);
                        ret = AV_FIFO_ERR;
                        goto TAR_OUT;
                    }
                    ret = encodeWriteFrame(p_frameOut, stream_index, NULL);
                    if (ret < 0) {
                        LOGI( "Write audio frame err![%d]", ret);
                        goto TAR_OUT;
                    }
                    av_frame_unref(p_frame);
                }
            }
        } else {
            if (pkt->pts > endTime[stream_index]) {
                needRead &= ~NEEDAUDIO;
                ret = 0;
                goto TAR_OUT;
            } else if (pkt->pts < startTime[stream_index]) {
                ret = 0;
                goto TAR_OUT;
            }

            pkt->pts -= startTime[audioIndex];
            pkt->dts -= startTime[audioIndex];
            out_stream = ofmtCtx->streams[audioIndex];
            av_packet_rescale_ts(pkt,
                                 in_stream->time_base,\
                                 out_stream->time_base);
            pkt->stream_index = audioIndex;
            ret = av_interleaved_write_frame(ofmtCtx, pkt);
            if (ret < 0) {
                LOGI( "Error muxing packet");
                goto TAR_OUT;
            }
        }
#endif
    } else {
        ret = 0;
        LOGI( "Unsuport media type![%d]", stream_index);
    }

TAR_OUT:

    if (p_frame) {
        av_frame_free(&p_frame);
    }
    if (p_frameOut) {
        av_frame_free(&p_frameOut);
    }

    return ret;
}

int MediaFilter::flushDecoder(AVFormatContext *ifmtCtx, AVFrame *p_frame, unsigned int streamIndex, int *gotFrame)
{
    int ret;
    AVStream *inStream;
    AVCodecContext *decCtx;
    AVPacket pkt;

    int (*decode)(AVCodecContext *, AVFrame *, int *, const AVPacket *) = NULL;

    inStream = ifmtCtx->streams[streamIndex];
    decCtx = inStream->codec;
    if (decCtx->codec_type == AVMEDIA_TYPE_VIDEO) {
        decode = avcodec_decode_video2;
    } else if (decCtx->codec_type == AVMEDIA_TYPE_AUDIO) {
        decode = avcodec_decode_audio4;
    }

    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    ret = decode(decCtx, p_frame, gotFrame, &pkt);
    if (ret < 0 || 0 == gotFrame) {
        return AV_STAT_ERR;
    }

    return 0;
}

int MediaFilter::flushEncoder(AVFormatContext *fmtCtx, unsigned int streamIndex)
{
    int ret;
    int gotFrame;
    AVPacket encPkt;
    AVStream *in_stream;
    AVStream *out_stream;
    AVCodecContext *deCtx;
    AVCodecContext *enCtx;
    int pktIndex;
    int (*encode)(AVCodecContext*, AVPacket*, const AVFrame*, int*) = NULL;

    /* encode filtered frame */
    in_stream = ifmtCtx->streams[streamIndex];
    if (in_stream == NULL) {
        ret = AV_STREAM_ERR;
        goto TAR_OUT;
    }

    deCtx = in_stream->codec;
    if (deCtx->codec_type == AVMEDIA_TYPE_AUDIO) {
        encode = avcodec_encode_audio2;
        pktIndex = audioIndex;
    } else if (deCtx->codec_type == AVMEDIA_TYPE_VIDEO) {
        encode = avcodec_encode_video2;
        pktIndex = videoIndex;
    } else {
        pktIndex = -1;
    }
    if (pktIndex < 0) {
        ret = AV_STREAM_ERR;
        goto TAR_OUT;
    }
    out_stream = ofmtCtx->streams[pktIndex];
    if (out_stream == NULL) {
        ret = AV_STREAM_ERR;
        goto TAR_OUT;
    }
    enCtx = out_stream->codec;
    if (enCtx == NULL) {
        ret = AV_ENCODE_ERR;
        goto TAR_OUT;
    }

    while (true) {
        encPkt.data = NULL;
        encPkt.size = 0;
        av_init_packet(&encPkt);

        ret = encode(enCtx, &encPkt, NULL, &gotFrame);
        av_frame_free(NULL);
        if (ret < 0) {
            LOGE("Flush encoder err!\n");
            break;
        }
        if (!gotFrame) {
            av_log(NULL, AV_LOG_INFO, "[Warning] Didn't find frame in encoder!\r\n");
            break;
        }
        /* mux encoded frame */
        av_packet_rescale_ts(&encPkt, in_stream->time_base,\
                             out_stream->time_base);
        encPkt.stream_index = pktIndex;
        ret = av_interleaved_write_frame(ofmtCtx, &encPkt);
        if (ret < 0) {
            break;
        }
        av_packet_unref(&encPkt);
    }

TAR_OUT:

    return ret;
}

void MediaFilter::release()
{
    AVStream *stream;
    keyPts_t *keyFramTs;
    int i, j;

#if ANDROID
    av_log_set_callback(av_log_default_callback);
#endif
    for(std::vector <keyPts_t *>::iterator it = keyFramePts.begin(); it != keyFramePts.end(); it++) {
        delete *it;
    }
    std::vector<keyPts_t *>().swap(keyFramePts);
    for(std::vector <WaterMark_t *>::iterator it = waterMarkList.begin(); it != waterMarkList.end(); it++) {
        delete *it;
    }
    std::vector<WaterMark_t *>().swap(waterMarkList);
    if (NULL != filterCtx) {
        for (i=0; i<nb_streams; i++) {
            if (filterCtx[i].filterGraph) {
                avfilter_graph_free(&filterCtx[i].filterGraph);
            }
        }
        av_freep(&filterCtx);
    }

    if (resample_context) {
        swr_close(resample_context);
        swr_free(&resample_context);
    }
    if (fifo) {
        av_audio_fifo_free(fifo);
        fifo = NULL;
    }
    if (NULL != ofmtCtx) {
        for (i=0; i<ofmtCtx->nb_streams; i++) {
            flushEncoder(ofmtCtx, i);
        }
        av_write_trailer(ofmtCtx);
        for (int i=0; i<ofmtCtx->nb_streams; i++) {
            stream = ofmtCtx->streams[i];
            if (stream && stream->codec) {
                avcodec_close(stream->codec);
            }
        }
        avformat_close_input(&ofmtCtx);
    }

    if (NULL != ifmtCtx) {
        for (i=0; i<nb_streams; i++) {
            stream = ifmtCtx->streams[i];
            if (stream && stream->codec) {
                if (stream->codec->codec_type == AVMEDIA_TYPE_AUDIO &&
                    audioSamples != NULL) {
                    av_freep(&audioSamples);
                }
                avcodec_close(stream->codec);
            }
        }
        avformat_close_input(&ifmtCtx);
    }

    if (parm->backgroudColor) {
        free(parm->backgroudColor);
    }
    if (parm) {
        delete parm;
        parm = NULL;
    }

    if (listener) {
        listener->MediaFilterProgressEnded(this);
        delete listener;
        listener = NULL;
    }
    processRate = 1.0;
    programStat = false;
    inVideoStream = NULL;
    inAudioStream = NULL;
    outVideoStream = NULL;
    outAudioStream = NULL;
    isInit = false;
}


float MediaFilter::progress()
{
    return processRate;
}

void MediaFilter::quickCropVideo(float start, float end)
{
    return;
}

int MediaFilter::frameReverse(std::vector<frame_t *>&frameQueue)
{
    std::vector<frame_t *>::iterator frameElm;
    AVRational defaultTimebase;
    int64_t reversePtsStart;
    int64_t reversePtsEnd;
    int64_t duration;
    enum AVMediaType type;
    AVFrame *p_frameOut;
    int streamIndex;
    float progress = 0.0;
    int gotPkt;
    int ret = 0;


    defaultTimebase.num = 1;
    defaultTimebase.den = AV_TIME_BASE;

    reversePtsStart = av_rescale_q((int64_t)(getReverseStart() * AV_TIME_BASE ), \
                                   defaultTimebase, inVideoStream->time_base);
    reversePtsEnd = av_rescale_q((int64_t)(getReverseEnd() * AV_TIME_BASE ), \
                                 defaultTimebase, inVideoStream->time_base);
    if (reversePtsEnd > endVideoPts) {
        duration = endVideoPts;
    } else {
        duration = reversePtsEnd;
    }

    p_frameOut = av_frame_alloc();
    if (NULL == p_frameOut) {
        LOGE("Alloc frame err!\n");
        ret = AV_MALLOC_ERR;
        goto TAR_OUT;
    }

    for (frameElm = frameQueue.begin(); frameElm != frameQueue.end(); frameElm++) {
        type = (*frameElm)->type;
        if (type == AVMEDIA_TYPE_VIDEO) {
            (*frameElm)->mediaFrame->pts = endVideoPts - (*frameElm)->mediaFrame->pts;
            if (listener) {
                progress = (float)((*frameElm)->mediaFrame->pts + reversePtsStart)/ duration;
                if (progress > 1.0) {
                    progress = 1.0;
                }
                listener->MediaFilterProgressChanged(this, progress, 1.0);
            }

        } else if (type == AVMEDIA_TYPE_AUDIO) {
            (*frameElm)->mediaFrame->pts = \
            (frame_cnt_a++) * (inAudioStream->time_base.den/\
                               (inAudioStream->time_base.num * \
                                av_q2d(inAudioStream->r_frame_rate)));
        }

        streamIndex = (*frameElm)->stream_index;
        if (av_buffersrc_add_frame(filterCtx[streamIndex].buffersrcCtx, (*frameElm)->mediaFrame) < 0) {
            LOGE("Error while feeding the filtergraph\n");
            goto TAR_OUT;
        }
        while (true) {
            ret = av_buffersink_get_frame(filterCtx[streamIndex].buffersinkCtx, p_frameOut);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            if (ret < 0) {
                LOGE("Get frame err!\n");
                break;
            }

            ret = encodeWriteFrame(p_frameOut, streamIndex, &gotPkt);
            av_frame_unref(p_frameOut);
            if (ret < 0) {
                LOGE("Encode or write frame err!\n");
                break;
            }
        }

        av_frame_free(&((*frameElm)->mediaFrame));

        delete *frameElm;
    }

    av_frame_free(&p_frameOut);
    ret = 0;

TAR_OUT:

    return ret;
}

int MediaFilter::insertFrameQueue(std::vector<frame_t *> &frameQueue, AVFrame *p_frame, int mediaIndex, enum AVMediaType mediaType)
{
    frame_t *frameBuf = NULL;

    frameBuf = new frame_t;
    if (NULL == frameBuf) {
        return AV_MALLOC_ERR;
    }

    frameBuf->mediaFrame = av_frame_clone(p_frame);
    frameBuf->type = mediaType;
    frameBuf->stream_index = mediaIndex;
    frameQueue.insert(frameQueue.begin(), frameBuf);

    return 0;
}

int MediaFilter::sectionReverse(int mediaIndex, int64_t start, int64_t end)
{
    int ret;
    AVPacket pkt;
    int streamIndex;
    AVStream *in_stream;
    AVStream *out_stream;
    AVCodecContext *deCtx;
    enum AVMediaType type;
    AVFrame *p_frame;
    frame_t *frameBuf;
    bool isDone = false;
    std::vector<frame_t *>::iterator frameElm;
    std::vector<frame_t *> frameQueue;
    int64_t reversePtsStart;
    int64_t reversePtsEnd;
    int gotFrame;
    int reverseModel;
    int i;
    float processRate;

    int (*decode)(AVCodecContext *, AVFrame *, int *, const AVPacket *) = NULL;

    AVRational defaultTimebase;

    defaultTimebase.num = 1;
    defaultTimebase.den = AV_TIME_BASE;

    reversePtsStart = av_rescale_q((int64_t)(getReverseStart() * AV_TIME_BASE ),\
                                   defaultTimebase, inVideoStream->time_base);
    reversePtsEnd = av_rescale_q((int64_t)(getReverseEnd() * AV_TIME_BASE ), \
                                 defaultTimebase, inVideoStream->time_base);

    if (0 != reversePtsEnd && start > reversePtsEnd) {
        ret = 0;
        goto TAR_OUT;
    }
    av_seek_frame(ifmtCtx, mediaIndex, start, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(ifmtCtx->streams[mediaIndex]->codec);
    p_frame = av_frame_alloc();
    if (NULL == p_frame) {
        LOGE("Alloc frame err!\n");
        ret = AV_MALLOC_ERR;
        goto TAR_OUT;
    }

    reverseModel = getReverseMedia();

    while (programStat) {
        if (isDone) {
            break;
        }
        ret = av_read_frame(ifmtCtx, &pkt);
        if (ret < 0) {
            break;
        }
        streamIndex = pkt.stream_index;
        in_stream  = ifmtCtx->streams[streamIndex];
        deCtx = in_stream->codec;
        type = deCtx->codec_type;

        if (type == AVMEDIA_TYPE_VIDEO) {
            if ((REVERSE_VIDEO_ONLY & reverseModel) != REVERSE_VIDEO_ONLY) {
                av_packet_unref(&pkt);
                continue;
            }
            if (end != 0 && (pkt.pts >= end || \
                             (pkt.pts >= reversePtsEnd))) {
                isDone = true;
                av_packet_unref(&pkt);
                continue;
            }

            decode = avcodec_decode_video2;
        } else if (type == AVMEDIA_TYPE_AUDIO) {
            if ((REVERSE_AUDIO_ONLY & reverseModel) != REVERSE_AUDIO_ONLY) {
                av_packet_unref(&pkt);
                continue;
            }
            decode = avcodec_decode_audio4;
        } else {
            av_packet_unref(&pkt);
            continue;
        }

        ret = decode(deCtx, p_frame, &gotFrame, &pkt);
        if (ret < 0) {
            ret = AV_ENCODE_ERR;
            LOGE("Error when decoding %s\n", \
                   type == AVMEDIA_TYPE_AUDIO ? "audio": "video");
            goto TAR_OUT;
        }
        if (1 == gotFrame) {
            p_frame->pts = av_frame_get_best_effort_timestamp(p_frame);
            if (p_frame->pts >= reversePtsStart && p_frame->pts <= reversePtsEnd) {
                if (endVideoPts < p_frame->pts) {
                    endVideoPts = p_frame->pts;
                }
                insertFrameQueue(frameQueue, p_frame, streamIndex, type);
            }
            av_frame_unref(p_frame);
        }
        av_packet_unref(&pkt);
    }
#if 1

    for (i=0; i<nb_streams; i++) {
        in_stream = ifmtCtx->streams[i];
        deCtx = in_stream->codec;
        type = deCtx->codec_type;

        if (type == AVMEDIA_TYPE_VIDEO) {
            if ((REVERSE_VIDEO_ONLY & reverseModel) != REVERSE_VIDEO_ONLY) {
                continue;
            }
        } else if (type == AVMEDIA_TYPE_AUDIO) {
            if ((REVERSE_AUDIO_ONLY & reverseModel) != REVERSE_AUDIO_ONLY) {
                continue;
            }
        } else {
            continue;
        }
        while (1) {
            flushDecoder(ifmtCtx, p_frame, i, &gotFrame);
            if (gotFrame) {
                p_frame->pts = av_frame_get_best_effort_timestamp(p_frame);
                if (p_frame->pts >= reversePtsStart && p_frame->pts <= reversePtsEnd) {
                    if (type == AVMEDIA_TYPE_VIDEO) {
                        if (endVideoPts < p_frame->pts) {
                            endVideoPts = p_frame->pts;
                        }
                    }
                    insertFrameQueue(frameQueue, p_frame, i, deCtx->codec_type);
                }

                av_frame_unref(p_frame);
            } else {
                break;
            }
        }
    }
#endif
    av_frame_free(&p_frame);

    ret = frameReverse(frameQueue);
    if (ret < 0) {
        LOGE("Reverse video err![%d]\n", ret);
        goto TAR_OUT;
    }

TAR_OUT:


    std::vector<frame_t *>().swap(frameQueue);

    return ret;
}


int MediaFilter::reverseMedia(int model)
{
    keyPts_t *keypts0, *keypts1;
    std::vector <keyPts_t *>::iterator it;
    int streamIndex;
    float progress;
    int ret = 0;

    getKeyFramePts();

    streamIndex = inVideoStream->index;
    it = keyFramePts.begin();
    keypts0 = *it;
    ++it;

    endVideoPts = 0;
    sectionReverse(streamIndex, keypts0->videoPts, 0);

    for(; it != keyFramePts.end(); it++) {
        if (!programStat) {
            break;
        }

        keypts1 = *it;
        ret = sectionReverse(streamIndex, keypts1->videoPts, keypts0->videoPts);
        if (ret < 0) {
            LOGE("Section reverse video err!\n");
            goto TAR_OUT;
        }

        keypts0 = keypts1;
    }
TAR_OUT:

    return ret;
}


/*
 * 添加要拼接的视频路径
 * filename 文件路径
 */
int MediaFilter::addConcatInVideo(const char *filename)
{
    char *name;

    init();
    if (filename != NULL) {
        name = strdup(filename);
        filenameList.push_back(name);

    }
    return 0;
}


#define FILE_BUF_SIZE 2048

int fileCopy(const char *infile, const char *outfile)
{
    long n;
    char  buffer[FILE_BUF_SIZE];

    if (infile == NULL || outfile == NULL) {
        return AV_PARM_ERR;
    }

    std::ifstream in(infile, std::ios_base::in|std::ios_base::binary);
    std::ofstream out(outfile, std::ios_base::out|std::ios_base::binary);

    while(!in.eof()) {
        in.read(buffer, FILE_BUF_SIZE);       //从文件中读取256个字节的数据到缓存区
        n = in.gcount();             //由于最后一行不知读取了多少字节的数据，所以用函数计算一下。
        out.write(buffer, n);       //写入那个字节的数据
    }

    in.close();
    out.close();

    return 0;
}

/*
 * 对视频进行拼接的接口,目前支持同类型的视频进行拼接.
 *@parm outFilename 输出文件名
 */
int MediaFilter::concatVideo(const char *outFilename)
{
    int i = 0;
    int ret = 0;
    char *filename = nullptr;
    AVStream *stream, *outStream;
    AVStream *audioSteam = NULL, *videoStream = NULL;
    AVCodecContext *dectx;
    AVPacket pkt;
    int64_t audioDuration;
    int64_t videoDuration;
    int64_t dealTime = 0;
    std::vector<char *>::iterator it;
    int64_t segmentDuration = 0;
    int64_t frameCnt;
    int64_t delta = 0;
    int64_t deltaDP = 0;
    bool initout = false;
    bool readFirstAudio = false;
    bool readFirstVideo = false;
    int64_t firstAudioFramePts = 0;
    int64_t firstVideoFramePts = 0;
    int64_t firstVideoFrameDts = 0;
    int64_t endAudioFramePts = 0;
    int64_t endVideoFramePts = 0;
    int64_t endAudioFrameDts = 0;
    int64_t endVideoFrameDts = 0;
    int64_t curAudioFramePts = 0;
    int64_t curVideoFramePts = 0;
    int64_t endVideoDuration = 0;
    int64_t endAudioDuration = 0;
    AVRational defaultTimebase;
    concatSegmentCnt = 0;

    programStat = true;
    int64_t v_segment = 0;
    int64_t a_segment = 0;
    it=filenameList.begin();

    if (listener) {
        LOGD("listener :%p", listener);
        listener->MediaFilterProgressBegan(this);
    }

    if (filenameList.size() == 1) {
        it=filenameList.begin();
        ret = fileCopy(*it, outFilename);
        ret = initInFIle(*it);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Init in file context err[%d][%s]!", ret, filename);
        }
        concatSegmentCnt = 1;
        segmentDuration = ifmtCtx->duration;
        segmetduration.push_back(segmentDuration);
        goto TAR_OUT;
    }

    defaultTimebase.num = 1;
    defaultTimebase.den = AV_TIME_BASE;

    concatSegmentCnt = filenameList.size();
    if (segmentTime) {
        free(segmentTime);
        segmentTime = NULL;
    }
    for (it=filenameList.begin(); it != filenameList.end(); it++) {
        if (!programStat) {
            av_log(NULL, AV_LOG_ERROR, "programStat");
            break;
        }

        audioSteam = NULL;
        videoStream = NULL;
        if (listener) {
            listener->MediaFilterProgressChanged(this, i, filenameList.size());
        }

        filename = *it;
        audioSteam = NULL;
        videoStream = NULL;
        ret = initInFIle(filename);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Init in file context err[%d][%s]!", ret, filename);
            goto TAR_OUT;
        }
        segmentDuration = ifmtCtx->duration;
        av_log(NULL, AV_LOG_DEBUG, "Deal with %s \n", filename);
        if (!initout) {
            av_log(NULL, AV_LOG_ERROR, "init out file!");
            ret = initOutFileWithoutEncode(outFilename);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "init out file err!");
                goto TAR_OUT;
            }
            initout = true;
        }
        frameCnt = 0;

        readFirstVideo = false;
        readFirstAudio = false;

        delta = 0;
        audioDuration = 0;
        videoDuration = 0;
        v_segment = 0;
        a_segment = 0;

        while (programStat) {
            ret = av_read_frame(ifmtCtx, &pkt);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Read exit [%s]\n", makeErrorStr(ret));
                break;
            }

            stream = ifmtCtx->streams[pkt.stream_index];
            outStream = ofmtCtx->streams[pkt.stream_index];
            dectx = stream->codec;

            dealTime = av_rescale_q_rnd(pkt.pts, stream->time_base, defaultTimebase, AV_ROUND_INF);

            av_packet_rescale_ts(&pkt, stream->time_base,\
                                 outStream->time_base);
            frameCnt++;

            av_log(NULL, AV_LOG_INFO, "packet duration : %lld\n", pkt.duration);
            if (dectx->codec_type == AVMEDIA_TYPE_AUDIO) {
                if (!readFirstAudio) {
                    audioSteam = outStream;
                    firstAudioFramePts = pkt.pts;
                    readFirstAudio = true;
                }
                if (audioDuration == 0) {
                    audioDuration = av_rescale_q(stream->duration, stream->time_base, defaultTimebase);
                    av_log(NULL, AV_LOG_INFO, "Audioduration : %lld\n", audioDuration);
                }

                if (isSynBaseOnAudio && videoDuration != 0 \
                    && audioDuration != 0 \
                    && videoDuration < audioDuration \
                    && dealTime > videoDuration) {
                    av_log(NULL, AV_LOG_INFO, "Cut audio, deal now time [%lld]\n", dealTime);
                    av_packet_unref(&pkt);
                    continue;
                }

                pkt.pts = pkt.pts - firstAudioFramePts + av_rescale_q(endAudioDuration, AV_TIME_BASE_Q, outStream->time_base);
                curAudioFramePts = pkt.pts;
                pkt.dts = pkt.dts - firstAudioFramePts + av_rescale_q(endAudioDuration, AV_TIME_BASE_Q, outStream->time_base);
            } else if (dectx->codec_type == AVMEDIA_TYPE_VIDEO) {
                if (!readFirstVideo) {
                    videoStream = outStream;
                    firstVideoFramePts = pkt.pts;
                    firstVideoFrameDts = pkt.dts;
                    deltaDP = firstVideoFramePts-firstVideoFrameDts;
                    if (deltaDP < 0) {
                        deltaDP = -deltaDP;
                    }
                    readFirstVideo = true;
                }

                if (videoDuration == 0) {
                    videoDuration = av_rescale_q(stream->duration, stream->time_base, defaultTimebase);
                    av_log(NULL, AV_LOG_INFO, "videoDuration : %lld\n", videoDuration);
                }

                pkt.pts =  pkt.pts - firstVideoFramePts +  av_rescale_q(endVideoDuration, AV_TIME_BASE_Q, outStream->time_base);;

                if (firstVideoFramePts > 0) {
                    pkt.dts =  pkt.dts - firstVideoFramePts + av_rescale_q(endVideoDuration, AV_TIME_BASE_Q, outStream->time_base);;
                } else {
                    pkt.dts =  pkt.dts + firstVideoFramePts + av_rescale_q(endVideoDuration, AV_TIME_BASE_Q, outStream->time_base);;
                }
                curVideoFramePts = pkt.pts;

                /* 预留开发配置接口，暂时对多余的视频不做处理*/
                if (/* DISABLES CODE */ (false) && isSynBaseOnAudio && videoDuration != 0 \
                    && audioDuration != 0 \
                    && videoDuration > audioDuration \
                    && dealTime >= audioDuration ) {
                    av_log(NULL, AV_LOG_INFO, "Cut video\n");
                    av_packet_unref(&pkt);
                    continue;
                }
            }

            dealTime = av_rescale_q_rnd(pkt.pts, outStream->time_base, defaultTimebase, AV_ROUND_INF);
            av_log(NULL, AV_LOG_ERROR, "Deal time %lld ", dealTime);
            if (av_interleaved_write_frame(ofmtCtx, &pkt) < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error muxing packet\n");
                goto TAR_OUT;
            }
            av_packet_unref(&pkt);
        }

        endAudioFramePts = curAudioFramePts+1;
        endVideoFramePts = curVideoFramePts+1;
        if (videoStream) {
            v_segment = av_rescale_q_rnd(endVideoFramePts, videoStream->time_base, AV_TIME_BASE_Q, AV_ROUND_INF) - endVideoDuration;
            endVideoDuration = av_rescale_q_rnd(endVideoFramePts, videoStream->time_base, AV_TIME_BASE_Q, AV_ROUND_INF);
        }
        if (audioSteam) {
            a_segment = av_rescale_q_rnd(endAudioFramePts, audioSteam->time_base, AV_TIME_BASE_Q, AV_ROUND_INF) - endAudioDuration;
            endAudioDuration = av_rescale_q_rnd(endAudioFramePts, audioSteam->time_base, AV_TIME_BASE_Q, AV_ROUND_INF);
        }
        segmentDuration = v_segment > a_segment ? v_segment : a_segment;
        segmetduration.push_back(segmentDuration);

        if (audioSteam != NULL && videoStream != NULL) {
            if (endAudioDuration > endVideoDuration) {
                endVideoDuration = endAudioDuration;
            }
        }
        avformat_close_input(&ifmtCtx);
        i++;

    }
    if (ret == AVERROR_EOF) {
        ret = 0;
    }
    if (listener) {
        LOGD("listener :%p", listener);
        listener->MediaFilterProgressEnded(this);
    }

    av_packet_unref(&pkt);

TAR_OUT:

    if (NULL != ofmtCtx) {
        av_write_trailer(ofmtCtx);
        avformat_close_input(&ofmtCtx);
    }
    if (NULL != ifmtCtx) {
        avformat_close_input(&ifmtCtx);
    }
    for (it=filenameList.begin(); it != filenameList.end(); it++) {
        filename = *it;
        free(filename);
    }
    std::vector<char *>().swap(filenameList);

    if (listener) {
        listener->MediaFilterProgressEnded(this);
        delete listener;
        listener = NULL;
    }
    return ret;
}

float *MediaFilter::getConcatSegmentDuration()
{
    int index = 0;
    int64_t time;
    if (segmentTime) {
        return segmentTime;
    }
    segmentTime = (float *)malloc(concatSegmentCnt * sizeof(float *));
    if (segmentTime == NULL) {
        LOGE("Alloc segment time error!\n");
        return NULL;
    }

    for(int i=0; i<segmetduration.size(); i++) {
        segmentTime[i] = segmetduration[i] / 1000000.0;
    }

    std::vector<int64_t>().swap(segmetduration);

    return segmentTime;
}

/* 获取片段总数 */
int MediaFilter::getSegmentCount()
{
    return concatSegmentCnt;
}

int MediaFilter::remuxStripMedia(const char *inFilename, const char *dstfilename, int type)
{
    int ret;
    AVOutputFormat *ofmt = NULL;
    AVStream *in_stream, *out_stream;
    AVCodecContext *decodec = NULL;
    AVPacket pkt;
    int i;
    AVRational defaultTimebase;
    int stream_index;

    init();

    if (listener) {
        listener->MediaFilterProgressBegan(this);
    }

    if ((ret = avformat_open_input(&ifmtCtx, inFilename, 0, 0)) < 0) {
        LOGE("Could not open input file '%s'[%s]", inFilename, makeErrorStr(ret));
        goto TAR_OUT;
    }

    if ((ret = avformat_find_stream_info(ifmtCtx, 0)) < 0) {
        LOGE("Failed to retrieve input stream information");
        ret = AV_STREAM_ERR;
        goto TAR_OUT;
    }
    avformat_alloc_output_context2(&ofmtCtx, NULL, NULL, dstfilename);
    if (!ofmtCtx) {
        LOGE("Could not create output context\n");
        ret = AV_MALLOC_ERR;
        goto TAR_OUT;
    }

    ofmt = ofmtCtx->oformat;

    for (i = 0; i < ifmtCtx->nb_streams; i++) {
        in_stream = ifmtCtx->streams[i];
        decodec = in_stream->codec;
        if (decodec && decodec->codec_type == AVMEDIA_TYPE_AUDIO && type == AV_AUDIO) {
            continue;
        } else if (decodec && decodec->codec_type == AVMEDIA_TYPE_VIDEO && type == AV_VIDEO) {
            continue;
        } else {
            //av_log(NULL, AV_LOG_INFO, "Add outfile stream %d[v%d a%d]\n", decodec->codec_type, AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_AUDIO);
        }
        out_stream = avformat_new_stream(ofmtCtx, in_stream->codec->codec);
        if (!out_stream) {
            LOGE("Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            goto TAR_OUT;
        }

        ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
        if (ret < 0) {
            LOGE("Failed to copy context from input to output stream codec context\n");
            goto TAR_OUT;
        }
        av_dict_copy(&out_stream->metadata, in_stream->metadata, AV_DICT_DONT_OVERWRITE);
        out_stream->codec->codec_tag = 0;
        if (ofmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
            out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        defaultTimebase.num = 1;
        defaultTimebase.den = AV_TIME_BASE;

        duration[i] = av_rescale_q(in_stream->duration, \
                                   in_stream->time_base, defaultTimebase);
    }

    for (i=0; i<ofmtCtx->nb_streams; i++) {
        out_stream  = ofmtCtx->streams[i];
        av_log(NULL, AV_LOG_WARNING, "out_stream: %p\n", out_stream);
        if (out_stream == NULL) {
            av_log(NULL, AV_LOG_WARNING, "out stream null\n");
            continue;
        }
        decodec = out_stream->codec;
        if (decodec && decodec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIndex = i;
        } else if (decodec && decodec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioIndex = i;
        } else {
            av_log(NULL, AV_LOG_WARNING, "Unknown type!\n");
            continue;
        }
    }

    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmtCtx->pb, dstfilename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open output file '%s'", dstfilename);
            goto TAR_OUT;
        }
    }

    programStat = true;

    ret = avformat_write_header(ofmtCtx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        goto TAR_OUT;
    }

    while (programStat) {
        ret = av_read_frame(ifmtCtx, &pkt);
        if (ret < 0)
            break;

        in_stream  = ifmtCtx->streams[pkt.stream_index];
        decodec = in_stream->codec;


        if (decodec->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (AV_AUDIO == type) {
                av_free_packet(&pkt);
                continue;
            }
            stream_index = audioIndex;
        } else if (decodec->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (AV_VIDEO == type) {
                av_free_packet(&pkt);
                continue;
            }
            stream_index = videoIndex;
        }
        out_stream = ofmtCtx->streams[stream_index];
        /* copy packet */
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, \
                                   (enum AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, \
                                   (enum AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        pkt.stream_index = stream_index;
        //log_packet(ofmtCtx, &pkt, "out");

        ret = av_interleaved_write_frame(ofmtCtx, &pkt);
        if (ret < 0) {
            LOGE("Error muxing packet\n");
            break;
        }
        av_free_packet(&pkt);
    }

    av_free_packet(&pkt);

    if (ret == AVERROR_EOF) {
        ret = 0;
    }

    av_write_trailer(ofmtCtx);

TAR_OUT:

    programStat = false;

    if (ifmtCtx) {
        avformat_close_input(&ifmtCtx);
    }

    /* close output */
    if (ofmtCtx) {
        avformat_close_input(&ofmtCtx);
    }

    if (listener) {
        listener->MediaFilterProgressEnded(this);
        delete listener;
        listener = NULL;
    }
    return ret;
}
/*
 * 对图像进行上下反转
 */
int imageReverse(AVFrame *frame, int height, int width)
{
    frame->data[0] += frame->linesize[0] * (height - 1);
    frame->linesize[0] *= -1;
    frame->data[1] += frame->linesize[1] * (height / 2 - 1);
    frame->linesize[1] *= -1;
    frame->data[2] += frame->linesize[2] * (height / 2 - 1);
    frame->linesize[2] *= -1;

    return 0;
}

int MediaFilter::getFrameRGBASize(int *videoWidth, int *videoHeight)
{
    if (NULL == videoWidth || NULL == videoHeight) {
        LOGE("width or height is null");
        return 0;
    }

    *videoWidth = getMediaShowWidth();
    *videoHeight = getMeidaShowHight();

    av_log(NULL, AV_LOG_INFO, "[Show] width %d height %d", *videoWidth, *videoHeight);
    return avpicture_get_size(AV_PIX_FMT_RGBA, *videoWidth, *videoHeight);
}

int MediaFilter::getFrameRGBAData(float getTime, uint8_t data[], size_t len)
{
    AVPacket pkt;
    AVFrame *p_frame = NULL;
    AVFrame *p_frameRGB = NULL;
    AVFrame *p_frameOut = NULL;
    int stream_index;
    AVStream *in_stream = NULL;;
    AVCodecContext *vDeCtx = NULL;;
    enum AVMediaType type;
    AVRational defaultTimebase;
    AVFilterContext *buffersrcCtx = NULL;
    AVFilterContext *buffersinkCtx = NULL;
    int gotFrame;
    float seekTime;
    float seekPoint = 0.0;
    int videoStreamIndex;
    int64_t cropTime;
    int64_t seekCropTime;
    struct SwsContext *swsCtx;
    int srcSlickH;
    int srcSlickW;
    int ret;
    bool isGetFrame = false;
    uint8_t *framedata;

    if (true != programStat) {
        LOGI( "Please Load file first, program exit!");
        ret = AV_STAT_ERR;
        goto TAR_OUT;
    }
    if (filterModel == -1) {
        initFilters();
        filterModel = AV_NORMAL;
    }

    if (NULL == data || len <= 0) {
        ret = AV_PARM_ERR;
        goto TAR_OUT;
    }

    framedata = (uint8_t *)av_malloc(len);
    LOGE("[%d]Getframe data : %p", __LINE__, data);
    if (getTime * AV_TIME_BASE - streamDuration[videoIndex] >= 0) {
        LOGE("Get time more than video duration duraion %lld Need time : %f", \
               streamDuration[videoIndex], getTime);
        ret = AV_SEEK_ERR;
        seekTime = (streamDuration[videoIndex] * 1.0) / AV_TIME_BASE;
        seekPoint = streamDuration[videoIndex] / AV_TIME_BASE - 0.5;
    } else {
        seekPoint = getTime;
        seekTime = getTime;
    }
    defaultTimebase.num = 1;
    defaultTimebase.den = AV_TIME_BASE;

    if (inVideoStream != NULL) {
        LOGE("check  time : %f", seekTime);
        seekCropTime = av_rescale_q((int64_t)(seekPoint * AV_TIME_BASE ),\
                                    defaultTimebase, inVideoStream->time_base);
        cropTime = av_rescale_q((int64_t)(seekTime * AV_TIME_BASE ),\
                                defaultTimebase, inVideoStream->time_base);
    } else {
        LOGE("Video stream isn't init!");
        goto TAR_OUT;
    }

    p_frame = av_frame_alloc();
    if (NULL == p_frame) {
        LOGI( "Alloc frame err!");
        ret = AV_MALLOC_ERR;
        goto TAR_OUT;
    }

    p_frameRGB = av_frame_alloc();
    if (NULL == p_frameRGB) {
        LOGI( "Alloc RGB frame err!");
        ret = AV_MALLOC_ERR;
        goto TAR_OUT;
    }

    srcSlickH = getMeidaShowHight();
    srcSlickW = getMediaShowWidth();

    ret = av_seek_frame(ifmtCtx, inVideoStream->index, seekCropTime, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(inVideoStream->codec);

    while (programStat) {
        ret = av_read_frame(ifmtCtx, &pkt);
        if (ret < 0) {
            LOGE("Stop read frame!![%s]", makeErrorStr(ret));
            break;
        }
        stream_index = pkt.stream_index;
        in_stream  = ifmtCtx->streams[stream_index];
        vDeCtx = in_stream->codec;
        type = vDeCtx->codec_type;

        if (type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = stream_index;
            av_frame_unref(p_frame);
            ret = avcodec_decode_video2(vDeCtx, p_frame, &gotFrame, &pkt);
            if (ret < 0) {
                LOGE("Decode video error!");
                goto TAR_OUT;
            }
            if (gotFrame == 1) {
                p_frame->pts = av_frame_get_best_effort_timestamp(p_frame);
                if (p_frame->pts < cropTime) {
                    av_packet_unref(&pkt);
                    continue;
                }

                buffersrcCtx = filterCtx[stream_index].buffersrcCtx;
                buffersinkCtx = filterCtx[stream_index].buffersinkCtx;

                if (buffersrcCtx == NULL || buffersinkCtx == NULL) {
                    av_log(ifmtCtx, AV_LOG_ERROR, "Get filter buffer or buffersink err!\n");
                    ret = AV_PARM_ERR;
                    goto TAR_OUT;
                }
                if (av_buffersrc_add_frame(buffersrcCtx, p_frame) < 0) {
                    LOGE("Error while feeding the filtergraph");
                    goto TAR_OUT;
                }

                p_frameOut = av_frame_alloc();
                if (NULL == p_frameOut) {
                    LOGE("Alloc out frame err!");
                    ret = AV_MALLOC_ERR;
                    goto TAR_OUT;
                }

                while (true) {
                    ret = av_buffersink_get_frame(buffersinkCtx, p_frameOut);
                    if (ret < 0) {
                        LOGE("Get buffer sink frame err!\n");
                        goto TAR_OUT;
                    }
                    break;
                }
                av_frame_unref(p_frame);

                avpicture_fill((AVPicture *)p_frameRGB, framedata, AV_PIX_FMT_RGBA, \
                               srcSlickW, srcSlickH);
                swsCtx = sws_getContext(srcSlickW, srcSlickH, vDeCtx->pix_fmt,
                                        srcSlickW, srcSlickH, AV_PIX_FMT_RGBA,
                                        SWS_BICUBIC, NULL, NULL, NULL);

                sws_scale(swsCtx, p_frameOut->data,
                          p_frameOut->linesize, 0, srcSlickH,
                          p_frameRGB->data, p_frameRGB->linesize);
                isGetFrame = true;

                sws_freeContext(swsCtx);
                ret = 0;

                break;
            }
        }
        av_packet_unref(&pkt);
    }

    av_packet_unref(&pkt);

    if (isGetFrame == false && (ret == 0 || ret == AVERROR_EOF)) {
        buffersrcCtx = filterCtx[videoStreamIndex].buffersrcCtx;
        buffersinkCtx = filterCtx[videoStreamIndex].buffersinkCtx;
        if (buffersrcCtx == NULL || buffersinkCtx == NULL) {
            av_log(ifmtCtx, AV_LOG_ERROR, "Get filter buffer or buffersink err!\n");
            ret = AV_PARM_ERR;
            goto TAR_OUT;
        }
        if (av_buffersrc_add_frame(buffersrcCtx, p_frame) < 0) {
            LOGE("Error while feeding the filtergraph");
            goto TAR_OUT;
        }

        p_frameOut = av_frame_alloc();
        if (NULL == p_frameOut) {
            LOGE("Alloc out frame err!");
            ret = AV_MALLOC_ERR;
            goto TAR_OUT;
        }

        while (true) {
            ret = av_buffersink_get_frame(buffersinkCtx, p_frameOut);
            if (ret < 0) {
                LOGE("Get buffer sink frame err!\n");
                goto TAR_OUT;
            }
            break;
        }
        av_frame_unref(p_frame);

        in_stream  = ifmtCtx->streams[videoStreamIndex];
        vDeCtx = in_stream->codec;

        avpicture_fill((AVPicture *)p_frameRGB, framedata, AV_PIX_FMT_RGBA, \
                       srcSlickW, srcSlickH);
        swsCtx = sws_getContext(srcSlickW, srcSlickH, vDeCtx->pix_fmt,
                                srcSlickW, srcSlickH, AV_PIX_FMT_RGBA,
                                SWS_BICUBIC, NULL, NULL, NULL);

        sws_scale(swsCtx, p_frameOut->data,
                  p_frameOut->linesize, 0, srcSlickH,
                  p_frameRGB->data, p_frameRGB->linesize);

        //data = p_frameRGB->data[0];
        sws_freeContext(swsCtx);
    }
    memcpy(data, framedata, len);

    LOGE("[%d]Getframe data : %p", __LINE__, data);
TAR_OUT:

    if (p_frame) {
        av_frame_free(&p_frame);
    }
    if (p_frameRGB) {
        av_frame_free(&p_frameRGB);
    }

    if (p_frameOut) {
        av_frame_free(&p_frameOut);
    }
    if (framedata) {
        av_freep(&framedata);
    }

    return ret;
}

/*
 * @brief: After load file, Filtering video or audio without encode and decode
 * @arg begin: starting point ; end stopping point
 * @return: 0 is OK, other exception
 */
#define PACKET_NUM  40

int MediaFilter::process(void)
{
    int i;
    int ret;
    int workType;
    AVPacket pkt;
    int type;
    int stream_index;
    int reverseModel;
    AVCodecContext *DeCtx = NULL;
    AVStream *in_stream;
    int dataTypeFlag = 0;

    if (true != programStat) {
        LOGI( "Please Load file first, program exit!");
        ret = AV_STAT_ERR;
        goto TAR_OUT;
    }

    ret = initFilters();
    if (ret < 0) {
        LOGE( "Init filter err![%d]", ret);
        goto TAR_OUT;
    }

    ret = initOutFile();
    if (ret < 0) {
        LOGE( "Init out file err![%d]", ret);
        ret = AV_FILE_ERR;
        goto TAR_OUT;
    }

    if (listener) {
        LOGD("listener :%p", listener);
        listener->MediaFilterProgressBegan(this);
    }

    reverseModel = getReverseMedia();
    if (reverseModel > 0) {
        LOGE("reverseMedia : %d", reverseModel);
        ret = reverseMedia(reverseModel);
        goto TAR_OUT;
    }

    workType = AV_WORK_IN_FILTERING;
    while (programStat) {
        ret = av_read_frame(ifmtCtx, &pkt);
        if (ret < 0) {
            LOGE("Stop read frame![%s]", makeErrorStr(ret));
            break;
        }

        stream_index = pkt.stream_index;
        in_stream  = ifmtCtx->streams[stream_index];
        DeCtx = in_stream->codec;
        type = DeCtx->codec_type;

        if (type != AVMEDIA_TYPE_VIDEO && type != AVMEDIA_TYPE_AUDIO) {
            continue;
        }
        if (0 == needRead) {
            LOGE( "Read full");
            av_packet_unref(&pkt);
            break;
        } else if (((NEEDVIDEO & needRead) == 0x00) &&
                   (type == AVMEDIA_TYPE_VIDEO)) {
            av_packet_unref(&pkt);
            continue;
        } else if ((NEEDAUDIO & needRead) == 0x00 &&
                   (type == AVMEDIA_TYPE_AUDIO)) {
            av_packet_unref(&pkt);
            continue;
        }
        writePacket(&pkt, workType, stream_index, type);

        av_packet_unref(&pkt);
    }
    LOGI("Quit program: %d", ret);

TAR_OUT:
    if (ret == AVERROR_EOF || ret > 0 || programStat == false) {
        ret = 0;
    }

    return ret;
}


bool MediaFilter::abort()
{
    programStat = false;

    if (listener) {
        listener->MediaFilterProgressCanceled(this);
    }
    return true;
}

int MediaFilter::autoAsembAudioFilterStr(char *asembStr, int strLen, char *outbrif, int outLen)
{
    int ret = 0;

    strncpy(asembStr, "anull", strLen);
    strncpy(outbrif, "out", outLen);

TAR_OUT:

    return ret;
}
#if 0
int MediaFilter::resetAudioFilterDesc(int64_t start, int64_t end, char *filterDesc, int DescLen)
{
    char complexStr[512] = {0};
    char filterStr[512] = {0};
    char linkStr[8] = "in";
    int filterCnt = 0;
    FilterStr_t *filterDsc = NULL;
    const char *base = "atrim=start_pts=%ld:end_pts=%ld,areverse";
    //const char *in = "in";

    if (parm->reverseModel & REVERSE_AUDIO_ONLY) {
#if 0
        snprintf(filterStr, sizeof(filterStr), filterMap[AV_ATRIM].baseStr, linkStr, start, end);
        strncpy(linkStr, filterMap[AV_ATRIM].brif, strlen(filterMap[AV_ATRIM].brif));
        strncat(complexStr, filterStr, strlen(filterStr));
        filterCnt++;
        filterDsc = &filterMap[AV_ATRIM];
#else
        snprintf(filterDesc, DescLen, base, start, end);
#endif
    }
    return 0;
}
#endif

int MediaFilter::resetVideoReverse(int64_t start, int64_t end, char *filterDesc, int DescLen)
{
    const char *base = "trim=start_pts=%ld:end_pts=%ld,reverse";

    if (parm->reverseModel & REVERSE_AUDIO_ONLY) {
#if 0
        snprintf(filterStr, sizeof(filterStr), filterMap[AV_ATRIM].baseStr, linkStr, start, end);
        strncpy(linkStr, filterMap[AV_ATRIM].brif, strlen(filterMap[AV_ATRIM].brif));
        strncat(complexStr, filterStr, strlen(filterStr));
        filterCnt++;
        filterDsc = &filterMap[AV_ATRIM];
#else
        snprintf(filterDesc, DescLen, base, start, end);
#endif
    }
    return 0;
}


int MediaFilter::autoAsembVideoFilterStr(char *asembStr, int strLen, char *outbrif, int outLen)
{
    int ret = 0;
    int i;
    char complexStr[512] = {0};
    char filterStr[512] = {0};
    char linkStr[8] = "in";
    FilterStr_t *filterDsc = NULL;
    const char *delm = ",";
    int filterCnt = 0;
    float rateW, rateH;
    int padPosX = 0;
    int padPosY = 0;
    bool crop_video_flag = false;
    std::vector <keyPts_t *>::iterator it;
    std::vector <WaterMark_t *>::iterator wmIt;
    WaterMark_t *wm;
    int wm_cnt = 0;

    if (NULL == asembStr || 0 == strLen || NULL == outbrif || 0 == outLen) {
        LOGE("autoAsembVideoFilterStr parmerter err!\n");
        ret = AV_PARM_ERR;
        goto TAR_OUT;
    }

    for (i=0; i<2; i++) {
        if (parm->transpose[i] >= 0) {
            if (filterCnt > 0) {
                strncat(complexStr, delm, strlen(delm));
            }
            snprintf(filterStr, sizeof(filterStr), filterMap[AV_TRANSPOSE].baseStr, \
                     linkStr, parm->transpose[i], i);
            memset(linkStr, 0, sizeof(linkStr));
            snprintf(linkStr, sizeof(linkStr), filterMap[AV_TRANSPOSE].brif, i);
            strncat(complexStr, filterStr, strlen(filterStr));
            filterCnt++;
            filterDsc = &filterMap[AV_TRANSPOSE];
        }
    }

    for (i=0; i<2; i++) {
        if (parm->flip[i] > 0) {
            if (filterCnt > 0) {
                strncat(complexStr, delm, strlen(delm));
            }
            if (parm->flip[i] == AV_VFLIP) {
                snprintf(filterStr, sizeof(filterStr), filterMap[AV_VFLIP].baseStr, \
                         linkStr, i);
            } else if (parm->flip[i] == AV_HFLIP) {
                snprintf(filterStr, sizeof(filterStr), filterMap[AV_HFLIP].baseStr, \
                         linkStr, i);
            } else if (parm->flip[i] == AV_ROUNDFLIP) {
                snprintf(filterStr, sizeof(filterStr), filterMap[AV_ROUNDFLIP].baseStr, \
                         linkStr, i, i, i);
            } else if (parm->flip[i] == AV_RROUNDFLIP) {
                snprintf(filterStr, sizeof(filterStr), filterMap[AV_ROUNDFLIP].baseStr, \
                         linkStr, i, i, i);
            } else {
                ret = AV_CONFIG_ERR;
                LOGE("Flip type err!\n");
                memset(complexStr, 0, sizeof(complexStr));
                goto TAR_OUT;
            }
            memset(linkStr, 0, sizeof(linkStr));
            snprintf(linkStr, sizeof(linkStr), filterMap[parm->flip[i]].brif, i);
            //            strncpy(linkStr, filterMap[parm->flip].brif, strlen(filterMap[parm->flip].brif));
            strncat(complexStr, filterStr, strlen(filterStr));
            filterCnt++;
            //filterDsc = &filterMap[parm->flip];
        }
    }

    if (parm->cropHeight > 0 && parm->cropWidth > 0) {
        if (filterCnt > 0) {
            strncat(complexStr, delm, strlen(delm));
        }
        snprintf(filterStr, sizeof(filterStr), filterMap[AV_CROP].baseStr, \
                 linkStr, parm->cropWidth,  parm->cropHeight, parm->cropPosX, parm->cropPosY);
        memset(linkStr, 0, sizeof(linkStr));
        strncpy(linkStr, filterMap[AV_CROP].brif, strlen(filterMap[AV_CROP].brif));
        strncat(complexStr, filterStr, strlen(filterStr));
        filterCnt++;
        crop_video_flag = true;
        filterDsc = &filterMap[AV_CROP];
    }

    if ((parm->cropHeight > 0 && parm->cropWidth > 0) && \
        ((parm->outHeight != parm->cropHeight || \
          parm->cropWidth != parm->outWidth) || \
         (parm->outHeight == 0 && \
          parm->outWidth == 0))) {
             if (filterCnt > 0) {
                 strncat(complexStr, delm, strlen(delm));
             }
             if ((parm->filterMode & AV_SCALE_REGULAR) == AV_SCALE_REGULAR) {
                 rateH = (parm->outHeight * 10000) / parm->cropHeight;
                 rateW = (parm->outWidth * 10000) / parm->cropWidth;

                 if (rateH == rateW) {
                     parm->scaleWidth = parm->outWidth;
                     parm->scaleHeight = parm->outHeight;
                 } else if (rateH > rateW) {
                     LOGE("%d rateH > rateW", __LINE__);
                     parm->scaleWidth = parm->outWidth;
                     parm->scaleHeight = ceil(parm->cropHeight * rateW / 10000 );
                     padPosY = (parm->outHeight - parm->scaleHeight) / 2;
                 } else {
                     LOGE("%d rateH < rateW", __LINE__);
                     parm->scaleHeight = parm->outHeight;
                     parm->scaleWidth = ceil(parm->cropWidth * rateH / 10000) ;
                     padPosX = ((parm->outWidth) - parm->scaleWidth) / 2;
                 }
                 if ((parm->scaleWidth & 1) != 0) {
                     parm->scaleWidth += 1;
                 }
                 if ((parm->scaleHeight & 1) != 0) {
                     parm->scaleHeight += 1;
                 }
                 snprintf(filterStr, sizeof(filterStr), filterMap[AV_SCALE].baseStr, \
                          linkStr, parm->scaleWidth, parm->scaleHeight);
                 memset(linkStr, 0, sizeof(linkStr));
                 strncpy(linkStr, filterMap[AV_SCALE].brif, strlen(filterMap[AV_SCALE].brif));
                 strncat(complexStr, filterStr, strlen(filterStr));
                 filterDsc = &filterMap[AV_SCALE];
                 filterCnt++;
                 strncat(complexStr, delm, strlen(delm));
                 LOGE("%d AV_SCALE_REGULAR[%d][%d]", __LINE__, padPosX, padPosY);
                 snprintf(filterStr, sizeof(filterStr), filterMap[AV_PAD].baseStr, \
                          linkStr, parm->outWidth, parm->outHeight, padPosX, padPosY,
                          parm->backgroudColor);
                 strncpy(linkStr, filterMap[AV_PAD].brif, strlen(filterMap[AV_PAD].brif));
                 strncat(complexStr, filterStr, strlen(filterStr));
                 filterCnt++;
                 filterDsc = &filterMap[AV_PAD];
             } else if (((parm->filterMode & AV_SCALE_MAX) == AV_SCALE_MAX))  {
                 if (parm->cropHeight > parm->cropWidth) {
                     parm->outWidth = parm->minEdge;
                     parm->outHeight = (int)((float)parm->minEdge / parm->cropWidth  * parm->cropHeight + 0.5);
                 } else {
                     parm->outHeight = parm->minEdge;
                     parm->outWidth = (int)((float)parm->minEdge / parm->outHeight * parm->cropWidth + 0.5);
                 }
                 if ((parm->outWidth & 1) != 0) {
                     parm->outWidth += 1;
                 }
                 if ((parm->outHeight & 1) != 0) {
                     parm->outHeight += 1;
                 }

                 snprintf(filterStr, sizeof(filterStr), filterMap[AV_SCALE].baseStr, \
                          linkStr, parm->outWidth, parm->outHeight);
                 memset(linkStr, 0, sizeof(linkStr));
                 strncpy(linkStr, filterMap[AV_SCALE].brif, strlen(filterMap[AV_SCALE].brif));
                 strncat(complexStr, filterStr, strlen(filterStr));
                 filterDsc = &filterMap[AV_SCALE];
                 filterCnt++;
             } else {
                 snprintf(filterStr, sizeof(filterStr), filterMap[AV_SCALE].baseStr, \
                          linkStr, parm->outWidth, parm->outHeight);
                 memset(linkStr, 0, sizeof(linkStr));
                 strncpy(linkStr, filterMap[AV_SCALE].brif, strlen(filterMap[AV_SCALE].brif));
                 strncat(complexStr, filterStr, strlen(filterStr));
                 filterDsc = &filterMap[AV_SCALE];
                 filterCnt++;
             }
         }else if ((parm->cropHeight == 0 && parm->cropWidth == 0) && \
                   (parm->outHeight > 0 && parm->outWidth > 0) && \
                   (parm->outHeight != parm->realHeight || \
                    parm->outWidth != parm->realWidth)) {
                       if (filterCnt > 0) {
                           strncat(complexStr, delm, strlen(delm));
                       }
                       snprintf(filterStr, sizeof(filterStr), filterMap[AV_SCALE].baseStr, \
                                linkStr, parm->outWidth, parm->outHeight);
                       memset(linkStr, 0, sizeof(linkStr));
                       strncpy(linkStr, filterMap[AV_SCALE].brif, strlen(filterMap[AV_SCALE].brif));
                       strncat(complexStr, filterStr, strlen(filterStr));
                       filterDsc = &filterMap[AV_SCALE];
                       filterCnt++;
                   }


    for(wmIt = waterMarkList.begin(); wmIt != waterMarkList.end(); wmIt++, wm_cnt++) {
        wm = *wmIt;

        if (filterCnt > 0) {
            strncat(complexStr, delm, strlen(delm));
        }

        snprintf(filterStr, sizeof(filterStr), filterMap[AV_WATERMARK].baseStr, \
                 wm->filename, wm->wWidth, wm->wHight, wm_cnt, linkStr, wm_cnt, \
                 wm->wPosX, wm->wPosY, wm->start, wm->end, wm_cnt, wm_cnt);
        snprintf(linkStr, sizeof(linkStr), filterMap[AV_WATERMARK].brif, wm_cnt);
        strncat(complexStr, filterStr, strlen(filterStr));
        filterCnt++;
        filterDsc = &filterMap[AV_WATERMARK];
    }


    if (strlen(complexStr) > strLen) {
        ret = AV_PARM_ERR;
        LOGE("String too short, need %zu Byte string!", strlen(complexStr));
        goto TAR_OUT;
    } else if (strlen(complexStr) > 0) {
        strncpy(asembStr, complexStr, strLen);
        strncpy(outbrif, linkStr, outLen);
        ret = 0;
    } else {
        strncpy(asembStr, filterMap[AV_NULL].baseStr, strLen);
        strncpy(outbrif, "out", outLen);
        ret = 0;
    }
TAR_OUT:

    return ret;
}

int MediaFilter::configFilterGraph(FilterCtx_t *filterCtx, const char *inName, char *outName, char *filterSpec)
{
    AVFilterInOut *outputs = NULL;
    AVFilterInOut *inputs = NULL;
    int ret;

    outputs = avfilter_inout_alloc();
    if (NULL == outputs) {
        ret = AV_MALLOC_ERR;
        goto TAR_OUT;
    }
    inputs  = avfilter_inout_alloc();
    if (NULL == inputs) {
        ret = AV_MALLOC_ERR;
        goto TAR_OUT;
    }

    outputs->name		= av_strdup(inName);
    outputs->filter_ctx = filterCtx->buffersrcCtx;
    outputs->pad_idx	= 0;
    outputs->next		= NULL;

    inputs->name	   = av_strdup(outName);
    inputs->filter_ctx = filterCtx->buffersinkCtx;
    inputs->pad_idx    = 0;
    inputs->next	   = NULL;

    if (NULL == outputs->name || NULL == inputs->name) {
        ret = AV_MALLOC_ERR;
        LOGE("strdup filter inout name err!\n");
        goto TAR_OUT;
    }

    if ((ret = avfilter_graph_parse_ptr(filterCtx->filterGraph, filterSpec,
                                        &inputs, &outputs, NULL)) < 0) {
        ret = AV_CONFIG_ERR;
        goto TAR_OUT;
    }

    if ((ret = avfilter_graph_config(filterCtx->filterGraph, NULL)) < 0) {
        ret = AV_CONFIG_ERR;
        goto TAR_OUT;
    }

TAR_OUT:

    if (outputs) {
        if (outputs->name) {
            av_free(outputs->name);
        }
        avfilter_inout_free(&outputs);
    }
    if (inputs) {
        if (inputs->name) {
            av_free(inputs->name);
        }
        avfilter_inout_free(&inputs);
    }

    return ret;
}

int MediaFilter::initVideoFilter(FilterCtx_t *filterCtx, AVCodecContext *decCtx, AVCodecContext *enCtx, char *filterSpec, char *outBrif)
{
    enum AVPixelFormat pixFmts[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
    AVBufferSinkParams *params = NULL;
    AVFilterContext *buffersrcCtx = NULL;
    AVFilterContext *buffersinkCtx = NULL;
    AVFilterGraph *filterGraph = NULL;
    AVFilter *buffersrc = NULL;
    AVFilter *buffersink = NULL;
    const char *inname = "in";
    char args[128] = {0};
    int ret;

    if (NULL == filterCtx || NULL == decCtx || NULL == enCtx || NULL == filterSpec) {
        LOGE("[%s]Parm err!\n", __func__);
        ret = AV_PARM_ERR;
        goto TAR_OUT;
    }
    buffersrc = avfilter_get_by_name("buffer");
    if (NULL == buffersrc) {
        ret = AV_NOT_FOUND;
        LOGE("Not found buffer filter!\n");
        goto TAR_OUT;
    }
    buffersink = avfilter_get_by_name("buffersink");
    if (!buffersrc || !buffersink) {
        LOGE("filtering source or sink element not found\n");
        ret = AV_NOT_FOUND;
        goto TAR_OUT;
    }

    filterGraph = avfilter_graph_alloc();
    if (NULL == filterGraph) {
        ret = AV_MALLOC_ERR;
        LOGE("Alloc filter graph err!\n");
        goto TAR_OUT;
    }
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             decCtx->width, decCtx->height, decCtx->pix_fmt,
             decCtx->time_base.num, decCtx->time_base.den,
             decCtx->sample_aspect_ratio.num, decCtx->sample_aspect_ratio.den);

    ret = avfilter_graph_create_filter(&buffersrcCtx, buffersrc, "in",
                                       args, NULL, filterGraph);
    if (ret < 0) {
        LOGE("Cannot create buffer source\n");
        goto TAR_OUT;
    }

    params = av_buffersink_params_alloc();
    if (NULL == params) {
        ret = AV_MALLOC_ERR;
        LOGI("Alloc params err!");
        goto TAR_OUT;
    }

    params->pixel_fmts = pixFmts;
    ret = avfilter_graph_create_filter(&buffersinkCtx, buffersink, "out",
                                       NULL, params, filterGraph);
    av_freep(&params);
    if (ret < 0) {
        LOGE("Cannot create buffer sink\n");
        return ret;
    }

    filterCtx->buffersrcCtx = buffersrcCtx;
    filterCtx->buffersinkCtx = buffersinkCtx;
    filterCtx->filterGraph = filterGraph;

    ret = configFilterGraph(filterCtx, inname, outBrif, filterSpec);
    if (ret < 0) {
        LOGE("Configure filter graph err!\n");
        goto TAR_OUT;
    }

TAR_OUT:

    if (ret < 0) {
        if (filterGraph) {
            avfilter_graph_free(&filterGraph);
        }
        LOGE("Init video filter err![%d]\n", ret);
    }

    return ret;

}

int MediaFilter::initAudioFilter(FilterCtx_t *filterCtx, AVCodecContext *decCtx, AVCodecContext *encCtx, char *filterSpec, char *outBrif)
{
    AVFilterGraph *filterGraph = NULL;
    AVFilterContext *buffersinkCtx = NULL;
    AVFilterContext *buffersrcCtx = NULL;
    enum AVSampleFormat out_sample_fmts[2];
    int64_t out_channel_layouts[2];
    int out_sample_rates[2] = {0};
    AVFilter *buffersrc = NULL;
    AVFilter *buffersink = NULL;
    const char *inname = "in";
    char args[512];
    int ret;

    if (NULL == filterCtx || NULL == decCtx || NULL == encCtx || NULL == filterSpec) {
        ret = AV_PARM_ERR;
        goto TAR_OUT;
    }

    buffersrc = avfilter_get_by_name("abuffer");
    if (NULL == buffersrc) {
        LOGE("Not found abuffer!\n");
        ret = AV_NOT_FOUND;
        goto TAR_OUT;
    }
    buffersink = avfilter_get_by_name("abuffersink");
    if (NULL == buffersink) {
        LOGE("Not found abuffersink!\n");
        ret = AV_NOT_FOUND;
        goto TAR_OUT;
    }
    filterGraph = avfilter_graph_alloc();
    if (NULL == filterGraph) {
        ret = AV_MALLOC_ERR;
        LOGE("Alloc filter graph err!\n");
        goto TAR_OUT;
    }
    if (!decCtx->channel_layout) {
        decCtx->channel_layout =
        av_get_default_channel_layout(decCtx->channels);
    }

    snprintf(args, sizeof(args),
             "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
             decCtx->time_base.num, decCtx->time_base.den, decCtx->sample_rate,
             av_get_sample_fmt_name(decCtx->sample_fmt), decCtx->channel_layout);
    ret = avfilter_graph_create_filter(&buffersrcCtx, buffersrc, "in",
                                       args, NULL, filterGraph);
    if (ret < 0) {
        LOGE("Cannot create audio buffer source\n");
        goto TAR_OUT;
    }
    LOGE("args: %s\n", args);
    ret = avfilter_graph_create_filter(&buffersinkCtx, buffersink, "out",
                                       NULL, NULL, filterGraph);
    if (ret < 0) {
        LOGE("Cannot create buffer sink\n");
        goto TAR_OUT;
    }

    out_sample_fmts[0] = AV_SAMPLE_FMT_S16;
    out_sample_fmts[1] = AV_SAMPLE_FMT_NONE;
    ret = av_opt_set_int_list(buffersinkCtx, "sample_fmts", out_sample_fmts, \
                              -1, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        LOGE("Cannot set output sample format\n");
        goto TAR_OUT;
    }

    out_channel_layouts[0] = encCtx->channel_layout;
    out_channel_layouts[1] = -1;
    ret = av_opt_set_int_list(buffersinkCtx, "channel_layouts", out_channel_layouts, \
                              -1, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        LOGE("Cannot set output channel layout\n");
        goto TAR_OUT;
    }
    out_sample_rates[0] = encCtx->sample_rate;
    out_sample_rates[1] = -1;
    ret = av_opt_set_int_list(buffersinkCtx, "sample_rates", out_sample_rates, \
                              -1, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        LOGE("Cannot set output sample rate\n");
        goto TAR_OUT;
    }

    filterCtx->buffersrcCtx = buffersrcCtx;
    filterCtx->buffersinkCtx = buffersinkCtx;
    filterCtx->filterGraph = filterGraph;
    ret = configFilterGraph(filterCtx, inname, outBrif, filterSpec);
    if (ret < 0) {
        LOGE("Configure filter graph err!\n");
        goto TAR_OUT;
    }

TAR_OUT:

    if (ret < 0) {
        if (filterGraph) {
            avfilter_graph_free(&filterGraph);
        }
        LOGE("Init audio filter err![%d]\n", ret);
    }

    return ret;

}

int MediaFilter::initFilters(void)
{
    AVStream *pStream;
    AVCodecContext *pCodecCtx;
    AVCodecContext *pEncodecCtx;
    enum AVMediaType codecType;
    char filterDesp[1024];
    int reverseModel;
    char outBrif[8];
    int ret = 0;
    int i;

    if (!programStat) {
        ret = AV_STAT_ERR;
        goto TAR_OUT;
    }

    filterCtx = (FilterCtx_t*)av_malloc_array(nb_streams, sizeof(FilterCtx_t));
    if (NULL == filterCtx) {
        ret = AV_MALLOC_ERR;
        goto TAR_OUT;
    }
    memset(filterCtx, 0, nb_streams*sizeof(FilterCtx_t));

    reverseModel = getReverseMedia();

    for (i=0; i<nb_streams; i++) {
        pStream = ifmtCtx->streams[i];
        pCodecCtx = pStream->codec;
        codecType = pCodecCtx->codec_type;

        if (codecType == AVMEDIA_TYPE_AUDIO) {
            if (reverseModel == 0 || reverseModel != REVERSE_VIDEO_ONLY) {
                pEncodecCtx = inAudioStream->codec;
                ret = autoAsembAudioFilterStr(filterDesp, sizeof(filterDesp), outBrif, sizeof(outBrif));
                if (ret < 0) {
                    ret = AV_CONFIG_ERR;
                    goto TAR_OUT;
                }
                LOGE("Audio filterDesp: %s outBrif: %s\n", filterDesp, outBrif);
                ret = initAudioFilter(&filterCtx[i], pCodecCtx, pEncodecCtx, filterDesp, outBrif);
                if (ret < 0) {
                    LOGE("Init audio filter");
                    goto TAR_OUT;
                }
                filterCtx[i].type = AVMEDIA_TYPE_AUDIO;
            }
        } else if (codecType == AVMEDIA_TYPE_VIDEO) {
            if (reverseModel == 0 || reverseModel != REVERSE_AUDIO_ONLY) {
                pEncodecCtx = inVideoStream->codec;
                ret = autoAsembVideoFilterStr(filterDesp, sizeof(filterDesp), outBrif, sizeof(outBrif));
                if (ret < 0) {
                    LOGE("Asemb video filter str err!\n");
                    goto TAR_OUT;
                }
#if _DEBUG
                memset(filterDesp, 0, sizeof(filterDesp));
                LOGE("filterDesp:%s outBrif:%s\n", filterDesp, outBrif);
                strcpy(filterDesp, "[in]transpose=1[tv_0],[tv_0]scale=720:1280[sv],movie=water1.jpg,scale=100:100[wm_0];[sv][wm_0]overlay=100:200:enable=between(t\\,5.0\\,10.0)[wmv_0]");
                memset(outBrif, 0, sizeof(outBrif));
                strcpy(outBrif, "wmv_0");
                LOGE("filterDesp:%s outBrif:%s\r\n", filterDesp, outBrif);
#endif
                LOGD("filterDesp:%s outBrif:%s\n", filterDesp, outBrif);
                ret = initVideoFilter(&filterCtx[i], pCodecCtx, pEncodecCtx, filterDesp, outBrif);
                if (ret < 0) {
                    LOGE("Init video filter err!\n");
                    goto TAR_OUT;
                }
                filterCtx[i].type = AVMEDIA_TYPE_VIDEO;
            }
        } else {
            //LOGE("Unsupport media type!\n");
        }
    }

TAR_OUT:

    return ret;
}

int MediaFilter::strInsert(const char *src, char *dst, int dst_len, char flag_c)
{
    int ret = 0;
    int insertPos = 0;
    const char *sub = "_filters";
    char *p_src = NULL;
    char *p_dst = NULL;
    size_t len;

    if (NULL == src || NULL == dst || 0 == dst_len) {
        LOGI( "Insert string parmer err![%s][%p][%d]", src, dst, dst_len);
        ret = AV_PARM_ERR;
        goto TAR_OUT;
    }
    insertPos = strchr(src, flag_c) - src;
    if (insertPos < 0  || dst_len < strlen(src)) {
        LOGI( "%s find illegal postion[%d]", src, insertPos);
    }

    len = strlen(sub);
    p_src = (char *)src;
    p_dst = dst;
    strncpy(p_dst, p_src, insertPos);
    p_dst += insertPos;
    p_src += insertPos;
    strncpy(p_dst, sub, strlen(sub));
    p_dst += strlen(sub);
    strncpy(p_dst, p_src, strlen(p_src));

TAR_OUT:

    return ret;
}

static char sSavePath[1024] = {0};
static int SaveFrameARGB(unsigned char* pARGB, int width, int height, int iFrame)
{
    FILE *pFile;
    char szFilename[1024] = {0};
    if (pARGB == NULL || width == 0 || height == 0) {
        return -1;
    }
    // Open file
    sprintf(szFilename, "%sframe%d.ppm", sSavePath,iFrame);
    pFile = fopen(szFilename, "wb");
    if (pFile == NULL) {
        return - 1;
    }
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);
    // Write pixel data
    for (int i = width*height; i > 0; i--)
    {
        fwrite(pARGB, 1, 3, pFile);
        pARGB += 4;
    }
    fclose(pFile);

    return 0;
}

static int notify(void *obj, int nFrame)
{
    MediaFilter *pmediaFilter;
    if (obj != NULL) {
        pmediaFilter = (MediaFilter *)obj;
        pmediaFilter->listener->MediaFilterProgressChanged(pmediaFilter, (double)nFrame, (double)0);
    }

    return 0;
}

int MediaFilter::generateThumb(const char *src, const char* save_path, double times[], size_t length)
{
    CFramePicker *picker = new CFramePicker();
    int res = 0;
    int ret = -1;
    sprintf(sSavePath, "%s", save_path);

    ret = picker->Open(src, true);
    if (ret < 0)
    {
        LOGE("key frame picker open error");
        picker->Close();

        delete picker;
        picker = NULL;
        return ret;
    }

    picker->m_SaveFunc = SaveFrameARGB;
    picker->notifyProgess = notify;
    res = picker->GetKeyFrameOrder(times, length, this);

    picker->Close();
    delete picker;
    picker = NULL;
    if (listener) {
        delete listener;
        listener = NULL;
    }

    return res;

}

