#include "FramePicker.h"
#include "MediaFilter.h"
//#define LOGE printf
#ifndef makeErrorStr
static char errorStr[AV_ERROR_MAX_STRING_SIZE];
#define makeErrorStr(errorCode) av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, errorCode)
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) if(p){delete (p); (p) = NULL;}
#endif

static int open_codec_context(int *stream_idx, AVFormatContext *fmt_ctx, AVCodecContext **dec_ctx, enum AVMediaType type)
{
    int ret;
    AVStream *st;
    AVCodec *dec = NULL;
    if ((ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0)) < 0)
    {
        av_log(NULL,  AV_LOG_ERROR, "Could not find %s stream !(%s)\n",
             av_get_media_type_string(type), makeErrorStr(ret));
        return ret;
    }
    else
    {
        *stream_idx = ret;
        st = fmt_ctx->streams[*stream_idx];
        /* find decoder for the stream */
        *dec_ctx = st->codec;
        dec = avcodec_find_decoder((*dec_ctx)->codec_id);
        if (!dec)
        {
            av_log(NULL,  AV_LOG_ERROR, "Failed to find %s codec(%s) codec id:%d\n",
                 av_get_media_type_string(type), makeErrorStr(ret),(*dec_ctx)->codec_id);
            *stream_idx = -1;
            return -1;
        }
        if ((ret = avcodec_open2(*dec_ctx, dec, NULL)) < 0)
        {
            av_log(NULL,  AV_LOG_ERROR, "Failed to open %s codec(%s)\n",
                 av_get_media_type_string(type), makeErrorStr(ret));
            return ret;
        }
    }
    return 0;
}

static void SaveFrameARGB(unsigned char* pARGB, int width, int height, int iFrame)
{
    FILE *pFile;
    char szFilename[32];
    // Open file

    if (NULL == pARGB) {
        av_log(NULL,  AV_LOG_ERROR, "ARGB data is null");
        return ;
    }

    sprintf(szFilename, "frame%d.ppm", iFrame);
    pFile = fopen(szFilename, "wb");
    if (pFile == NULL)
        return;

    av_log(NULL,  AV_LOG_ERROR, "width :%d height %d", width, height);
    // Write header
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);
    // Write pixel data
    for (int i = width*height; i > 0; i--)
    {
        fwrite(pARGB, 1, 3, pFile);
        pARGB += 4;
    }
    // Close file
    fclose(pFile);

}

CFramePicker::CFramePicker()
{
    m_pFormatContext = NULL;
    m_pVideoStream = NULL;
    m_pSrcFrame = NULL;
    m_pRGBFrame = NULL;
    sws_ctx = NULL;
}


CFramePicker::~CFramePicker()
{
    this->Close();
}

int CFramePicker::Open(const char* file, bool isThumbnail)
{
    int ret = -1;
    av_register_all();
    avcodec_register_all();
    int numBytes;
//    uint8_t *rgbBuf = NULL;

    if (m_pFormatContext)
    {
        avformat_close_input(&m_pFormatContext);
        m_pFormatContext = NULL;
        if (rgbBuf) {
            av_free(rgbBuf);
        }
    }


    if ((ret = avformat_open_input(&m_pFormatContext, file, NULL, NULL)) < 0)
    {
        av_log(NULL,  AV_LOG_ERROR, "Error: Could not open %s (%s)\n", file, makeErrorStr(ret));
        avformat_close_input(&m_pFormatContext);
        m_pFormatContext = NULL;
        return -1;
    }

    /* ��ȡ����Ƶ����Ϣ */
    if ((ret = avformat_find_stream_info(m_pFormatContext, NULL)) < 0)
    {
        av_log(NULL,  AV_LOG_ERROR, "Could not find stream information (%s)\n", makeErrorStr(ret));
        avformat_close_input(&m_pFormatContext);
        m_pFormatContext = NULL;
        return -1;
    }

    //open codec context.
    if ((ret = open_codec_context(&m_video_stream_idx, m_pFormatContext, &m_pVideoCodec, AVMEDIA_TYPE_VIDEO)) < 0)
    {
        av_log(NULL,  AV_LOG_ERROR, "No exit video.\n");
    }
    if (m_video_stream_idx >= 0)
    {
        m_pVideoStream = m_pFormatContext->streams[m_video_stream_idx];
    }

    m_VideoWidth = m_pVideoCodec->width;
    m_VideoHeight = m_pVideoCodec->height;
    if (isThumbnail) {
        nailWidth = 176;
        nailHeight = m_VideoHeight* nailWidth / m_VideoWidth;
        if ((nailHeight & 2) == 1) {
            nailHeight++;
        }
    } else {
        nailWidth = m_VideoWidth;
        nailHeight = m_VideoHeight;
    }

    av_log(NULL,  AV_LOG_ERROR, "Frame count : %lld\n", m_pVideoStream->nb_frames);

    if (sws_ctx)
    {
        sws_freeContext(sws_ctx);
        sws_ctx = NULL;
    }

    sws_ctx = sws_getContext(
                             m_VideoWidth,
                             m_VideoHeight,
                             m_pVideoCodec->pix_fmt,
                             nailWidth,
                             nailHeight,
                             AV_PIX_FMT_RGBA,
                             SWS_BICUBIC, NULL, NULL, NULL);

    if (m_pSrcFrame)
    {
        av_frame_free(&m_pSrcFrame);
        m_pSrcFrame = NULL;
    }

    m_pSrcFrame = av_frame_alloc();
    if (m_pSrcFrame == NULL) {
        av_log(NULL,  AV_LOG_ERROR, "Alloc src frame error!");
        ret = -1;
        goto TAR_OUT;
    }

    if (m_pRGBFrame)
    {
        av_frame_free(&m_pRGBFrame);
        m_pRGBFrame = NULL;
    }

    m_pRGBFrame = av_frame_alloc();
    if (m_pRGBFrame == NULL)
    {
        av_log(NULL,  AV_LOG_ERROR, "Alloc RGB frame error!");
        ret = -1;
        goto TAR_OUT;
    }

    numBytes=avpicture_get_size(AV_PIX_FMT_RGBA, nailWidth, nailHeight);
    rgbBuf=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
    avpicture_fill((AVPicture *) m_pRGBFrame, rgbBuf, AV_PIX_FMT_RGBA, nailWidth, nailHeight);

    av_dump_format(m_pFormatContext, 0, file, 0);

TAR_OUT:

    return ret;
}

void CFramePicker::Close()
{

    if (m_pVideoStream && m_pVideoStream->codec)
    {
        avcodec_close(m_pVideoStream->codec);
        m_pVideoCodec = NULL;
        m_pVideoStream = NULL;
    }

    if (m_pFormatContext)
    {
        avformat_close_input(&m_pFormatContext);
        m_pFormatContext = NULL;
    }

    if (m_pSrcFrame)
    {
        av_frame_free(&m_pSrcFrame);
        m_pSrcFrame = NULL;
    }

    if (m_pRGBFrame)
    {
        for (int i = 0; i < AV_NUM_DATA_POINTERS; ++i)
        {
            uint8_t * pDataMem = m_pRGBFrame->data[i];
            if (pDataMem != NULL)
            {
                av_free(pDataMem);
                m_pRGBFrame->data[i] = NULL;
            }
        }
        av_frame_free(&m_pRGBFrame);
        m_pRGBFrame = NULL;
    }

    if (sws_ctx)
    {
        sws_freeContext(sws_ctx);
        sws_ctx = NULL;
    }
}

double CFramePicker::GetVideoDuration()
{
    if (m_pFormatContext == NULL)
    {
        av_log(NULL,  AV_LOG_ERROR, "No any video is open!");
        return 0.0;
    }
    if (m_pFormatContext->duration != AV_NOPTS_VALUE)
    {
        int hours, mins, secs, us;
        int64_t duration = m_pFormatContext->duration + 5000;
        secs = duration / AV_TIME_BASE;
        us = duration % AV_TIME_BASE;
        mins = secs / 60;
        secs %= 60;
        hours = mins / 60;
        mins %= 60;
        av_log(NULL,  AV_LOG_ERROR, "  Duration: %02d:%02d:%02d.%02d\n", hours, mins, secs,(100 * us) / AV_TIME_BASE);
        m_Duration = hours*60.0*60.0 + mins*60.0 + secs + (double)us / AV_TIME_BASE;
        return m_Duration;
    }
    else
    {
        av_log(NULL,  AV_LOG_ERROR, "Could not get video duration (N/A).\n");
        return 0.0;
    }

    return 0.0;
}

int CFramePicker::GetKeyFrameOrder(double* stemps, int length, void *obj)
{
    int numBytes = 0;
    uint8_t *rgbBuf = NULL;

    if (stemps == NULL || length <= 0 || m_pFormatContext == NULL)
    {
        av_log(NULL,  AV_LOG_ERROR, "unvalid parameter (stemps:%p,length:%d,context:%p)", stemps, length, m_pFormatContext);
        return -1;
    }

    int ret = 0;

    double duration = this->GetVideoDuration();
    int64_t *stemps_lld = new int64_t[length];
    for (int i = 0; i < length; i++)
    {
        if (stemps[i] > duration)
        {
            stemps[i] = duration;
        }
        int64_t tsms = stemps[i] * 1000;
        int64_t DesiredFrameNumber = av_rescale(tsms, m_pVideoStream->time_base.den, m_pVideoStream->time_base.num);
        stemps_lld[i] = DesiredFrameNumber / 1000;
    }

    int nGotFrame = 0;
    AVPacket packet = { 0 };
    av_init_packet(&packet);
    while (av_read_frame(m_pFormatContext, &packet) >= 0)
    {
        if (packet.stream_index == m_video_stream_idx)
        {
            int frameFinished = 0;
            ret = avcodec_decode_video2(m_pVideoCodec, m_pSrcFrame, &frameFinished, &packet);
            if (ret < 0)
            {
                av_log(NULL,  AV_LOG_ERROR, "error in decode video.(%s)\n", makeErrorStr(ret));
                break;
            }

            if (frameFinished)
            {
                if (packet.pts > stemps_lld[nGotFrame])
                {
                    //m_pRGBFrame->linesize[0] = m_VideoWidth * 4;
                    sws_scale(sws_ctx, m_pSrcFrame->data, m_pSrcFrame->linesize, 0, m_pVideoCodec->height, \
                              m_pRGBFrame->data, m_pRGBFrame->linesize);
                    if (!this->m_SaveFunc) {
                        SaveFrameARGB(m_pRGBFrame->data[0], nailWidth, nailHeight, nGotFrame);
                    } else {
                        this->m_SaveFunc(m_pRGBFrame->data[0], nailWidth, nailHeight, nGotFrame);
                    }
                    if (this->notifyProgess) {

                        this->notifyProgess(obj, nGotFrame);
                    }
                    nGotFrame++;

                    if (nGotFrame == length)
                    {
                        break;
                    }
                }
            }
        }
        av_packet_unref(&packet);
        av_init_packet(&packet);
    }
    av_packet_unref(&packet);

    int frameFinished = 0;
    if (m_pVideoCodec && nGotFrame < length)
    {
        for (;;)
        {
            av_init_packet(&packet);
            ret = avcodec_decode_video2(m_pVideoCodec, m_pSrcFrame, &frameFinished, &packet);

            if (ret < 0)
            {
                av_packet_unref(&packet);
                break;
            }


            if (frameFinished) {
                sws_scale(sws_ctx, m_pSrcFrame->data, m_pSrcFrame->linesize, 0, m_pVideoCodec->height, m_pRGBFrame->data, m_pRGBFrame->linesize);
                if (!this->m_SaveFunc) {
                    SaveFrameARGB(m_pRGBFrame->data[0], nailWidth, nailHeight, nGotFrame);
                } else {
                    this->m_SaveFunc(m_pRGBFrame->data[0], nailWidth, nailHeight, nGotFrame);
                }
                if (this->notifyProgess) {
                    this->notifyProgess((MediaFilter *)obj, nGotFrame);
                }
                nGotFrame++;
                if (nGotFrame == length)
                {
                    av_packet_unref(&packet);
                    break;
                }
            }
            else
            {
                av_packet_unref(&packet);
                break;
            }
            av_packet_unref(&packet);
        }
    }

    while (m_pRGBFrame->data[0]&&nGotFrame < length)
    {
        if (!this->m_SaveFunc) {
            SaveFrameARGB(m_pRGBFrame->data[0], nailWidth, nailHeight, nGotFrame++);
        } else {
            this->m_SaveFunc(m_pRGBFrame->data[0], nailWidth, nailHeight, nGotFrame++);
        }
        if (this->notifyProgess) {
            this->notifyProgess((MediaFilter *)obj, nGotFrame);
        }
    }

    if (stemps_lld) {
        delete []stemps_lld;
        stemps_lld = NULL;
    }

    return nGotFrame;

}
