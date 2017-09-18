#include "VideoEditerAny.h"
#include "LogHelper.h"

#include <cassert>

//////////////////////////////////////////////////////////////////////////

#ifndef makeErrorStr
static char errorStr[AV_ERROR_MAX_STRING_SIZE];
#define makeErrorStr(errorCode) av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, errorCode)
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) if(p){delete (p); (p) = NULL;}
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) if(p){delete [] (p); (p) = NULL;}
#endif

#ifndef INT64_MIN
#define INT64_MIN        (-0x7fffffffffffffff - 1)
#endif

#ifndef INT64_MAX
#define INT64_MAX	0x7fffffffffffffff
#endif
#ifndef TIME_MICROSECOND
#define TIME_MICROSECOND 1000000.0f
#endif
#define DEFAULE_SIZE 480

static __inline int RGBToY(uint8 r, uint8 g, uint8 b) {
    return (66 * r + 129 * g +  25 * b + 0x1080) >> 8;
}

static __inline int RGBToU(uint8 r, uint8 g, uint8 b) {
    return (112 * b - 74 * g - 38 * r + 0x8080) >> 8;
}
static __inline int RGBToV(uint8 r, uint8 g, uint8 b) {
    return (112 * r - 94 * g - 18 * b + 0x8080) >> 8;
}

static int open_codec_context(int *stream_idx, AVFormatContext *fmt_ctx, AVCodecContext **dec_ctx, enum AVMediaType type)
{
	int ret;
	AVStream *st;
	AVCodec *dec = NULL;
	*stream_idx = -1;
	if ((ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0)) < 0)
	{
		LOGE("Could not find %s stream !(%s)\n",
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
			LOGE("Failed to find %s codec(%s)\n",
				av_get_media_type_string(type), makeErrorStr(ret));
			return ret;
		}
		if ((ret = avcodec_open2(*dec_ctx, dec, NULL)) < 0)
		{
			LOGE("Failed to open %s codec(%s)\n",
				av_get_media_type_string(type), makeErrorStr(ret));
			return ret;
		}
	}
	return 0;
}


static void *aligned_malloc(size_t required_bytes, size_t alignment) {
	void *p1;
	void **p2;
	size_t offset = alignment - 1 + sizeof(void*);
	p1 = malloc(required_bytes + offset);               // the line you are missing
	p2 = (void**)(((size_t)(p1)+offset)&~(alignment - 1));  //line 5
	p2[-1] = p1; //line 6
	return p2;
}

static void aligned_free(void* p) {
	void* p1 = ((void**)p)[-1];         // get the pointer to the buffer we allocated
	free(p1);
}

static void FillVideoFrame(unsigned char* pFillRes,AVFrame * frame,unsigned char cY,unsigned char cB,unsigned char cR,int width,int height)
{
    int dstWidth = frame->width;
    int dstHeight = frame->height;
    unsigned char* pSrcY = pFillRes;
    unsigned char* pSrcU = pFillRes + width*height;
    unsigned char* pSrcV = pSrcU + width*height/4;
    memset(pSrcY,cY,width*height);
    memset(pSrcU,cB,width*height/4);
    memset(pSrcV,cR,width*height/4);
    
    unsigned char* pDstY = frame->data[0];
    unsigned char* pDstU = frame->data[1];
    unsigned char* pDstV = frame->data[2];
    int dst_stride_y = frame->linesize[0];
    int dst_stride_u = frame->linesize[1];
    int dst_stride_v = frame->linesize[2];
    
    if(dstWidth == width && dstHeight == height)
    {
        for(int i = 0 ; i < height ; i ++)
        {
            memcpy(pSrcY,pDstY,width*sizeof(unsigned char));
            pSrcY += width;
            pDstY += dst_stride_y;
            if(i&1)
            {
                memcpy(pSrcU,pDstU,width/2*sizeof(unsigned char));
                pSrcU += width/2;
                pDstU += dst_stride_u;
                memcpy(pSrcV,pDstV,width/2*sizeof(unsigned char));
                pSrcV += width/2;
                pDstV += dst_stride_v;
            }
        }
        return ;
    }
    
    if(dstWidth < width)
    {
        int offset = (width - dstWidth)/2;
        
        for(int i = 0 ; i < height ; i ++)
        {
            memcpy(pSrcY + i*width + offset,pDstY + dst_stride_y*i,dstWidth*sizeof(unsigned char));
        }
        for(int i = 0 ; i < height/2 ; i ++)
        {
            memcpy(pSrcU + i*width/2 + offset/2,pDstU + dst_stride_u*i,dstWidth/2*sizeof(unsigned char));
            memcpy(pSrcV + i*width/2 + offset/2,pDstV + dst_stride_v*i,dstWidth/2*sizeof(unsigned char));
        }
    }
    else
    {
        int offset =  (height - dstHeight)/2;
        pSrcY += offset*width;
        pSrcU += (offset/2)*width/2;
        pSrcV += (offset/2)*width/2;
        int ending = height-offset;
        for(int  i = offset;i < ending;i++)
        {
            memcpy(pSrcY,pDstY,width*sizeof(unsigned char));
            pSrcY += width;
            pDstY += dst_stride_y;
            if(i&1)
            {
                memcpy(pSrcU,pDstU,width/2*sizeof(unsigned char));
                pSrcU += width/2;
                pDstU += dst_stride_u;
                memcpy(pSrcV,pDstV,width/2*sizeof(unsigned char));
                pSrcV += width/2;
                pDstV += dst_stride_v;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
CVideoEditerAny::CVideoEditerAny(void)
{
	m_pFormatContext = NULL;
	m_pVideoStream = NULL;
	m_pVideoCodec = NULL;
	m_pAudioStream = NULL;
	m_pAudioCodec = NULL;
	m_pSrcFrame = NULL;
	m_YuvFrame = NULL;
	yuv_sws_ctx = NULL;
	m_WatermarkType = WATERMARK_NONE;
	m_EndingWatermarkType = WATERMARK_NONE;
	m_WatermarkPath = NULL;
	m_EndingWatermarkPath = NULL;
	m_DstVideoWidth = -1;
	m_DstVideoHeight = -1;
	m_ExitAudio = false;
	m_video_stream_idx = -1;
	m_audio_stream_idx = -1;
	m_Rotate = MT_VIDEO_ROTATE_0;
	m_ImportMode = MT_IMPORT_MIN_SIZE;
	m_StandSize = DEFAULE_SIZE;
	m_RealEncodeWidth = -1;
	m_RealEncodeHeight = -1;
	m_ProcessBar = 0.0;
	m_bInterrupt = false;
    
    // add by Javan
    memset(&packet, 0, sizeof(AVPacket));
    resYuvDataSize = 0;
    listener = nullptr;

    clipX = clipY = 0;
    clipWidth = clipHeight = -1;
}


CVideoEditerAny::~CVideoEditerAny(void)
{
	this->Close();
    SAFE_DELETE(listener);
}

void CVideoEditerAny::Close()
{
	if (m_pVideoStream && m_pVideoStream->codec)
	{
		avcodec_close(m_pVideoStream->codec);
		m_pVideoCodec = NULL;
		m_pVideoStream = NULL;
	}

	if (m_pAudioStream && m_pAudioStream->codec)
	{
		avcodec_close(m_pAudioStream->codec);
		m_pAudioCodec = NULL;
		m_pAudioStream = NULL;
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
	if (yuv_sws_ctx)
	{
		sws_freeContext(yuv_sws_ctx);
		yuv_sws_ctx = NULL;
	}
	if (m_YuvFrame)
	{
		if (m_YuvFrame->data[0])
		{
			av_free(m_YuvFrame->data[0]);
			m_YuvFrame->data[0] = NULL;
		}
		av_frame_free(&m_YuvFrame);
		m_YuvFrame = NULL;
	}
	m_DstVideoWidth = -1;
	m_DstVideoHeight = -1;
	m_RealEncodeWidth = -1;
	m_RealEncodeHeight = -1;
	m_ExitAudio = false;
	m_video_stream_idx = -1;
	m_audio_stream_idx = -1;
	m_Rotate = MT_VIDEO_ROTATE_0;
	m_ImportMode = MT_IMPORT_MIN_SIZE;
	m_StandSize = DEFAULE_SIZE;
	m_EndingWatermarkType = WATERMARK_NONE;
	m_WatermarkType = WATERMARK_NONE;
	SAFE_DELETE_ARRAY(m_WatermarkPath);
	SAFE_DELETE_ARRAY(m_EndingWatermarkPath);
	m_ProcessBar = 0.0;
	m_bInterrupt = false;
	LOGE("wfcan videoediterany close finish.");
    
    // add by Javan
    SAFE_DELETE_ARRAY(pResYuvData);
    resYuvDataSize = 0;
    nYuvRotateMode = kRotate0;
    av_packet_unref(&packet);
    av_init_packet(&packet);
}

double CVideoEditerAny::GetVideoDuration()
{
	if (m_pFormatContext == NULL || m_pVideoStream == NULL)
	{
		LOGE("No any video is open or no have video stream!");
		return 0.0;
	}

		// delete by Javan .2015.9.10 fix video duration not right
//    if (m_pVideoStream && m_pVideoStream->duration != AV_NOPTS_VALUE)
//    {
//        double divideFactor;
//        divideFactor = (double)1 / av_q2d(m_pVideoStream->time_base);
//        double durationOfVideo = (double)m_pVideoStream->duration / divideFactor;
//        return durationOfVideo;
//    }

	else if (m_pFormatContext->duration != AV_NOPTS_VALUE)
	{
		int hours, mins, secs, us;
		int64_t duration = m_pFormatContext->duration;
		secs = duration / AV_TIME_BASE;
		us = duration % AV_TIME_BASE;
		mins = secs / 60;
		secs %= 60;
		hours = mins / 60;
		mins %= 60;
		LOGE("  Duration: %02d:%02d:%02d.%02d\n", hours, mins, secs, (100 * us) / AV_TIME_BASE);
		double durationOfVideo = hours*60.0*60.0 + mins*60.0 + secs + (double)us / AV_TIME_BASE;
		return durationOfVideo;
	}
	else
	{
		LOGE("Could not get video duration (N/A).\n");
		return 0.0;
	}

	return 0.0;
}

double CVideoEditerAny::GetAudioDuration()
{
	if (m_pFormatContext == NULL || m_pAudioStream == NULL)
	{
		LOGE("No any audio is open or no have audio stream!");
		return 0.0;
	}

	if (m_pAudioStream->duration != AV_NOPTS_VALUE)
	{
		double divideFactor;
		divideFactor = (double)1 / av_q2d(m_pAudioStream->time_base);
		double durationOfAudio = (double)m_pAudioStream->duration / divideFactor;
		return durationOfAudio;
	}
	else if (m_pFormatContext->duration != AV_NOPTS_VALUE)
	{
		int hours, mins, secs, us;
		int64_t duration = m_pFormatContext->duration;
		secs = duration / AV_TIME_BASE;
		us = duration % AV_TIME_BASE;
		mins = secs / 60;
		secs %= 60;
		hours = mins / 60;
		mins %= 60;
		LOGE("  Duration: %02d:%02d:%02d.%02d\n", hours, mins, secs, (100 * us) / AV_TIME_BASE);
		double durationOfAudio = hours*60.0*60.0 + mins*60.0 + secs + (double)us / AV_TIME_BASE;
		return durationOfAudio;
	}
	else
	{
		LOGE("Could not get video duration (N/A).\n");
		return 0.0;
	}
	return 0.0;
}


int CVideoEditerAny::Open( const char* file )
{
	int ret = -1;
	av_register_all();
	avcodec_register_all();
	if (m_pFormatContext)
	{
		avformat_close_input(&m_pFormatContext);
		m_pFormatContext = NULL;
	}


	if ((ret = avformat_open_input(&m_pFormatContext, file, NULL, NULL)) < 0)
	{
		LOGE("Error: Could not open %s (%s)\n", file, makeErrorStr(ret));
		avformat_close_input(&m_pFormatContext);
		m_pFormatContext = NULL;
		return -1;
	}

	/* ªÒ»°“Ù ”∆µ¡˜–≈œ¢ */
	if ((ret = avformat_find_stream_info(m_pFormatContext, NULL)) < 0)
	{
		LOGE("Could not find stream information (%s)\n", makeErrorStr(ret));
		avformat_close_input(&m_pFormatContext);
		m_pFormatContext = NULL;
		return -1;
	}

	//open codec context.
	if ((ret = open_codec_context(&m_video_stream_idx, m_pFormatContext, &m_pVideoCodec, AVMEDIA_TYPE_VIDEO)) < 0)
	{
		LOGE("No exit video.\n");
		avformat_close_input(&m_pFormatContext);
		return -2;
	}
	if (m_video_stream_idx >= 0)
	{
		m_pVideoStream = m_pFormatContext->streams[m_video_stream_idx];
	}

	AVDictionaryEntry *tag = NULL;
	tag = av_dict_get(m_pVideoStream->metadata, "rotate", tag, 0);
	if (tag == NULL)
	{
		m_Rotate = MT_VIDEO_ROTATE_0;
	}
	else
	{
		int angle = atoi(tag->value);
		angle %= 360;
		if (angle == 90)
		{
			m_Rotate = MT_VIDEO_ROTATE_90;
		}
		else if (angle == 180)
		{
			m_Rotate = MT_VIDEO_ROTATE_180;
		}
		else if (angle == 270)
		{
			m_Rotate = MT_VIDEO_ROTATE_270;
		}
		else
		{
			m_Rotate = MT_VIDEO_ROTATE_0;
		}
	}

	if ((ret = open_codec_context(&m_audio_stream_idx, m_pFormatContext, &m_pAudioCodec, AVMEDIA_TYPE_AUDIO)) < 0)
	{
		LOGE("No exit audio.\n");
		ret = 0;
	}
	if (m_audio_stream_idx >= 0)
	{
		m_pAudioStream = m_pFormatContext->streams[m_audio_stream_idx];
		m_ExitAudio = true;
	}

	m_VideoWidth = m_pVideoCodec->width;
	m_VideoHeight = m_pVideoCodec->height;

    // set default clip params
    clipX = clipY = 0;

	if(m_Rotate == MT_VIDEO_ROTATE_0 || m_Rotate == MT_VIDEO_ROTATE_180)
	{
        clipWidth = m_VideoWidth;
        clipHeight = m_VideoHeight;
	}
	else
	{
		//rotate 90 ,270
        clipWidth = m_VideoHeight;
        clipHeight = m_VideoWidth;
	}
    

	if (m_VideoWidth < 1 || m_VideoHeight < 1)
	{
		LOGE("warning : %s has error width :%d,height:%d", file, m_VideoWidth, m_VideoHeight);
		m_VideoWidth = m_VideoHeight = 1;
	}

	if (m_pSrcFrame)
	{
		av_frame_free(&m_pSrcFrame);
		m_pSrcFrame = NULL;
	}
	m_pSrcFrame = av_frame_alloc();

	//default set size;
	this->SetImportMode(MT_IMPORT_MIN_SIZE, DEFAULE_SIZE);

	return 0;
}

bool CVideoEditerAny::SetCropRegion(int left, int top, int width, int height) 
{
    this->clipX = left;
    this->clipY = top;
    this->clipWidth = width;
    this->clipHeight = height;
    return true;
}

bool CVideoEditerAny::SetImportMode( MTVideoImportMode mode,int size )
{
	if(size & 1)
	{
		LOGE("error size %d",size);
		return false;
	}

    LOGI("%s, import mode to %d, size %d", __func__, mode, size);

	m_ImportMode = mode;
	m_StandSize = size;
	//º∆À„Àı∑≈∫ÛµƒøÌ∏ﬂ;
	if(m_ImportMode == MT_IMPORT_MAX_SIZE)
	{
		//◊Ó¥Û±ﬂ « size
		if(m_VideoWidth > m_VideoHeight)
		{
			//∫·ππ;
			m_DstVideoWidth = m_StandSize;
			m_DstVideoHeight = m_VideoHeight*(m_DstVideoWidth + 0.0f)/m_VideoWidth;
			if(m_DstVideoHeight & 1)
			{
				//≈≈≥˝≈º ˝∏ﬂ;
				m_DstVideoHeight ++;
			}
		}
		else
		{
			// ˙ππ;
			m_DstVideoHeight = m_StandSize;
			m_DstVideoWidth = m_VideoWidth*(m_DstVideoHeight + 0.0f)/m_VideoHeight;
			if(m_DstVideoWidth & 1)
			{
				//≈≈≥˝≈º ˝øÌ;
				m_DstVideoWidth ++;
			}
		}

	}
	else if(m_ImportMode == MT_IMPORT_MIN_SIZE)
	{
		//◊Ó–°±ﬂ « size;
		if(m_VideoWidth > m_VideoHeight)
		{
			//∫·ππ;
			m_DstVideoHeight = m_StandSize;
			m_DstVideoWidth = m_VideoWidth*(m_DstVideoHeight + 0.0f)/m_VideoHeight;
			if(m_DstVideoWidth & 1)
			{
				//≈≈≥˝≈º ˝øÌ;
				m_DstVideoWidth ++;
			}
		}
		else
		{
			// ˙ππ;
			m_DstVideoWidth = m_StandSize;
			m_DstVideoHeight = m_VideoHeight*(m_DstVideoWidth + 0.0f)/m_VideoWidth;
			if(m_DstVideoHeight & 1)
			{
				//≈≈≥˝≈º ˝∏ﬂ;
				m_DstVideoHeight ++;
			}
		}
	} else if (m_ImportMode == MT_IMPORT_MIN_SIZE_MULTIPLE_16) {
        if(m_VideoWidth > m_VideoHeight)
        {
            //∫·ππ;
            m_DstVideoHeight = m_StandSize;
            m_DstVideoWidth = m_VideoWidth*(m_DstVideoHeight)/m_VideoHeight;
        }
        else
        {
            // ˙ππ;
            m_DstVideoWidth = m_StandSize;
            m_DstVideoHeight = m_VideoHeight*(m_DstVideoWidth)/m_VideoWidth;
        }

        m_DstVideoWidth =  (m_DstVideoWidth + 15)/16*16;
        m_DstVideoHeight =  (m_DstVideoHeight + 15)/16*16;
    } else if (MT_IMPORT_FREE == m_ImportMode) {
        // 自由模式, 目标尺寸等于默认值
        m_DstVideoWidth = m_VideoWidth;
        m_DstVideoHeight = m_VideoHeight;
    }
	else
	{
		LOGE("invalid import mode");
		return false;
	}

	if(m_Rotate == MT_VIDEO_ROTATE_0 || m_Rotate == MT_VIDEO_ROTATE_180)
	{
		m_RealEncodeWidth = m_DstVideoWidth;
		m_RealEncodeHeight = m_DstVideoHeight;
	}
	else
	{
		//rotate 90 ,270
		m_RealEncodeWidth = m_DstVideoHeight;
		m_RealEncodeHeight = m_DstVideoWidth;
    }

    if (MT_IMPORT_FREE != m_ImportMode) {
        // 不是自由模式的时候裁剪区域的宽度高度变成 real 的数值
        clipWidth = m_RealEncodeWidth;
        clipHeight = m_RealEncodeHeight;
        LOGI("Change clip region to real %d, %d", clipWidth, clipHeight);
    }
    
    LOGE("--- m_RealEncodeWidth %d, m_RealEncodeHeight %d", m_RealEncodeWidth, m_RealEncodeHeight);

	if (yuv_sws_ctx)
	{
		sws_freeContext(yuv_sws_ctx);
		yuv_sws_ctx = NULL;
	}
	if (m_pVideoCodec->pix_fmt != AV_PIX_FMT_NONE && (m_pVideoCodec->pix_fmt != AV_PIX_FMT_YUV420P ||
		m_VideoWidth != m_DstVideoWidth || m_VideoHeight != m_DstVideoHeight))
	{
		yuv_sws_ctx = sws_getContext(
			m_VideoWidth,
			m_VideoHeight,
			m_pVideoCodec->pix_fmt,
			m_DstVideoWidth,
			m_DstVideoHeight,
			AV_PIX_FMT_YUV420P,
			SWS_AREA, NULL, NULL, NULL);
        
        if (m_YuvFrame)
        {
            if (m_YuvFrame->data[0])
            {
                av_free(m_YuvFrame->data[0]);
                m_YuvFrame->data[0] = NULL;
            }
            av_frame_free(&m_YuvFrame);
            m_YuvFrame = NULL;
        }
        
		//…Í«Î“ª’≈YUV÷°(Àı∑≈”√Õæ)
		m_YuvFrame = av_frame_alloc();
		{
			int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, m_DstVideoWidth, m_DstVideoHeight, 1);
			uint8_t *buffer = (uint8_t *)av_malloc_array(1, numBytes);
            int len = av_image_fill_arrays(m_YuvFrame->data, m_YuvFrame->linesize, buffer, AV_PIX_FMT_YUV420P, m_DstVideoWidth, m_DstVideoHeight, 1);
			m_YuvFrame->width = m_DstVideoWidth;
			m_YuvFrame->height = m_DstVideoHeight;
		}
	}
	
	return true;
}

int CVideoEditerAny::CutVideoWithTime( const char* dstFile, double startTime, double endTime )
{
	if (m_pFormatContext == NULL)
	{
		LOGE("No any video is open!");
		return -1;
	}
	m_ProcessBar = 0.0;
	m_bInterrupt = false;

	double duration = this->GetVideoDuration() + 0.01;

    // Fix: end time is bigger then video duration, just use duration as end time.  Javan 2016.7.7.
    if(endTime > duration)
    {
        endTime = duration;
    }

    if (startTime >= endTime || startTime > duration || endTime > duration)
    {
        LOGE("invalid parameter. start:%f,end:%f,duration:%f", startTime, endTime, duration);
        return -1;
    }

	double record_duration = endTime - startTime + 0.01;
	CFrameRecorder recorder;
	//ƒø±ÍÕº¥Û–°;
    
    LOGD("Open Recorder with size %d, %d,", clipWidth, clipHeight);

	recorder.Open(dstFile, clipWidth, clipHeight);
	if (m_pVideoStream->avg_frame_rate.den && m_pVideoStream->avg_frame_rate.num)
	{
		double fps = av_q2d(m_pVideoStream->avg_frame_rate);
		if(fps > 30.1f)
		{
			recorder.SetMaxFrameRate30(true);
		}
	}
    // 设置对输入的参数的裁剪
	recorder.SetupCropRegion(clipX, clipY, clipWidth, clipHeight, 0);
	recorder.SetWatermark(m_WatermarkPath,m_WatermarkType);
	recorder.SetEndingWatermark(m_EndingWatermarkPath,m_EndingWatermarkType,endTime - startTime);
	recorder.AdjustVideoDuration(true,endTime - startTime);
	if (m_ExitAudio)
	{
		recorder.SetupAudio(m_pAudioCodec->channels, m_pAudioCodec->sample_rate, m_pAudioCodec->sample_fmt);
	}
	else
	{
		recorder.SetupAudio(1, 44100, AV_SAMPLE_FMT_S16);
	}

	recorder.SetupAudioAligen(true);

	recorder.Start();

	double video_pts_base = m_pVideoStream->time_base.num / (double)m_pVideoStream->time_base.den;
	int ret = -1;
	int frameFinished = 0;
	int64_t video_start_pts = (startTime*(m_pVideoStream->time_base.den)) / (m_pVideoStream->time_base.num) + 0.5;
	int64_t video_end_pts = (endTime*(m_pVideoStream->time_base.den)) / (m_pVideoStream->time_base.num) + 0.5;
	int64_t audio_start_pts = 0;
	int64_t audio_end_pts = 0;
	if (m_ExitAudio)
	{
		audio_start_pts = (startTime*(m_pAudioStream->time_base.den)) / (m_pAudioStream->time_base.num) + 0.5;
		audio_end_pts = (endTime*(m_pAudioStream->time_base.den)) / (m_pAudioStream->time_base.num) + 0.5;
	}

	bool nVideoFlag = false;// ”∆µ «∑Ò“—æ≠Ωÿ»°ÕÍ
	bool nAudioFlag = false;//“Ù∆µ «∑Ò“—æ≠Ωÿ»°ÕÍ
	double last_time_stamp = 0.0;
	//---------------seek-------------------------------;
	int64_t seekTime = startTime*TIME_MICROSECOND;
	ret = avformat_seek_file(m_pFormatContext, -1, INT64_MIN, seekTime, INT64_MAX, 0);
	avcodec_flush_buffers(m_pVideoCodec);
	/*if (av_seek_frame(m_pFormatContext, m_video_stream_idx, video_start_pts, AVSEEK_FLAG_BACKWARD) < 0)
	{
		LOGE("Failed to seek Video \n");
	}

	if (m_ExitAudio)
	{
		if (av_seek_frame(m_pFormatContext, m_audio_stream_idx, audio_start_pts, AVSEEK_FLAG_BACKWARD) < 0)
		{
			LOGE("Failed to seek Audio \n");
		}
	}
	avcodec_flush_buffers(m_pVideoCodec);*/
	//------------------end seek----------------------;

    if (listener) {
        listener->videoEditerAnyProgressBegan(this);
    }
    
	unsigned char* pResYuvData = NULL;
	if (m_Rotate != MT_VIDEO_ROTATE_0)
	{
		pResYuvData = new unsigned char[m_RealEncodeWidth*m_RealEncodeHeight * 3 / 2];
	}
	AVPacket packet = { 0 };
	av_init_packet(&packet);
	RotationMode nYuvRotateMode = kRotate0;
	//convert rotate
	switch(m_Rotate)
	{
	case MT_VIDEO_ROTATE_0:
		nYuvRotateMode = kRotate0;
		break;
	case MT_VIDEO_ROTATE_90:
		nYuvRotateMode = kRotate90;
		break;
	case MT_VIDEO_ROTATE_180:
		nYuvRotateMode = kRotate180;
		break;
	case MT_VIDEO_ROTATE_270:
		nYuvRotateMode = kRotate270;
		break;
	}


	while (av_read_frame(m_pFormatContext, &packet) >= 0)
	{
		if(m_bInterrupt)
		{
			//÷–∂œµº»Î
			nVideoFlag = true;
			LOGE("wfcan videoediter interrupt CutVideoWithTime finish.");
			break;
		}
		if (packet.stream_index == m_video_stream_idx && nVideoFlag == false)
		{
			ret = avcodec_decode_video2(m_pVideoCodec, m_pSrcFrame, &frameFinished, &packet);
			if (frameFinished)
			{
				int64_t nPTS = av_frame_get_best_effort_timestamp(m_pSrcFrame);

				if (nPTS < video_start_pts)
				{
					av_packet_unref(&packet);
					av_init_packet(&packet);
					continue;
				}
				if (nPTS > video_end_pts)
				{
					nVideoFlag = true;
					if (nAudioFlag || m_ExitAudio == false)
					{
						break;
					}
					av_packet_unref(&packet);
					av_init_packet(&packet);
					continue;
				}
				double time_stemp = (nPTS - video_start_pts)* av_q2d(m_pVideoStream->time_base);
				last_time_stamp = time_stemp;
				m_ProcessBar = min(100.0,time_stemp/record_duration * 100.0);
                
                
                if (listener) {
                    listener->videoEditerAnyProgressChanged(this, time_stemp, record_duration);
                }
                
				byte* pResY = NULL;
				byte* pResU = NULL;
				byte* pResV = NULL;
				int nResStride_y = 0;
				int nResStride_u = 0;
				int nResStride_v = 0;
				if (yuv_sws_ctx)
				{
					//–Ë“™◊™≥…YUV420P
					ret = sws_scale(yuv_sws_ctx, m_pSrcFrame->data, m_pSrcFrame->linesize, 0, m_pSrcFrame->height, m_YuvFrame->data, m_YuvFrame->linesize);
					if (ret < 0)
					{
						LOGE("error in sws_scale.");
						return -1;
					}
					pResY = m_YuvFrame->data[0];
					pResU = m_YuvFrame->data[1];
					pResV = m_YuvFrame->data[2];
					nResStride_y = m_YuvFrame->linesize[0];
					nResStride_u = m_YuvFrame->linesize[1];
					nResStride_v = m_YuvFrame->linesize[2];

					if(m_Rotate != MT_VIDEO_ROTATE_0)
					{
						pResY = pResYuvData;
						pResU = pResYuvData + m_RealEncodeWidth*m_RealEncodeHeight;
						pResV = pResU + m_RealEncodeWidth*m_RealEncodeHeight / 4;

						I420Rotate(m_YuvFrame->data[0], m_YuvFrame->linesize[0],
							m_YuvFrame->data[1], m_YuvFrame->linesize[1],
							m_YuvFrame->data[2], m_YuvFrame->linesize[2], 
							pResY, m_RealEncodeWidth,
							pResU, m_RealEncodeWidth / 2, pResV, m_RealEncodeWidth / 2,
							m_YuvFrame->width, m_YuvFrame->height, nYuvRotateMode);
						
						nResStride_y = m_RealEncodeWidth;
						nResStride_u = m_RealEncodeWidth/2;
						nResStride_v = m_RealEncodeWidth/2;

					}
				}
				else
				{
					pResY = m_pSrcFrame->data[0];
					pResU = m_pSrcFrame->data[1];
					pResV = m_pSrcFrame->data[2];
					nResStride_y = m_pSrcFrame->linesize[0];
					nResStride_u = m_pSrcFrame->linesize[1];
					nResStride_v = m_pSrcFrame->linesize[2];

					if(m_Rotate != MT_VIDEO_ROTATE_0)
					{
						pResY = pResYuvData;
						pResU = pResYuvData + m_RealEncodeWidth*m_RealEncodeHeight;
						pResV = pResU + m_RealEncodeWidth*m_RealEncodeHeight / 4;

						I420Rotate(m_pSrcFrame->data[0], m_pSrcFrame->linesize[0],
							m_pSrcFrame->data[1], m_pSrcFrame->linesize[1],
							m_pSrcFrame->data[2], m_pSrcFrame->linesize[2], 
							pResY, m_RealEncodeWidth,
							pResU, m_RealEncodeWidth / 2, pResV, m_RealEncodeWidth / 2,
							m_pSrcFrame->width, m_pSrcFrame->height, nYuvRotateMode);
						
						nResStride_y = m_RealEncodeWidth;
						nResStride_u = m_RealEncodeWidth/2;
						nResStride_v = m_RealEncodeWidth/2;

					}
				}
				ret = recorder.RecordI420(pResY, nResStride_y, 
					pResU, nResStride_u, 
					pResV, nResStride_v, m_RealEncodeWidth, m_RealEncodeHeight, time_stemp);
				if(ret < 0)
				{
					av_packet_unref(&packet);
					goto finish;
				}
			}
		}
		else if (packet.stream_index == m_audio_stream_idx && nAudioFlag == false)
		{
			AVPacket tmpPack;
			tmpPack.data = packet.data;
			tmpPack.size = packet.size;
			while (packet.size > 0)
			{
				ret = avcodec_decode_audio4(m_pAudioCodec, m_pSrcFrame, &frameFinished, &packet);
				packet.size -= ret;
				packet.data += ret;
				if (frameFinished)
				{
					int64_t nPTS = av_frame_get_best_effort_timestamp(m_pSrcFrame);
					double time_stemp = (nPTS - audio_start_pts)* av_q2d(m_pAudioStream->time_base);

					if (nPTS < audio_start_pts)
					{
						break;
					}
					if (nPTS > audio_end_pts)
					{
						nAudioFlag = true;
						break;
					}

					recorder.RecordPCM(m_pSrcFrame->extended_data, m_pSrcFrame->nb_samples);
				}
			}
			packet.data = tmpPack.data;
			packet.size = tmpPack.size;

		}
		av_packet_unref(&packet);
		av_init_packet(&packet);
		if (nAudioFlag && nVideoFlag)
		{
			break;
		}

	}
	av_packet_unref(&packet);


	if (m_pVideoCodec && !nVideoFlag)
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
			if (frameFinished) 
			{
				int64_t nPTS = av_frame_get_best_effort_timestamp(m_pSrcFrame);
				if (nPTS < video_start_pts)
				{
					av_packet_unref(&packet);
					continue;
				}
				if (nPTS > video_end_pts)
				{
					nVideoFlag = true;
					av_packet_unref(&packet);
					if (nAudioFlag || m_ExitAudio == false)
					{
						break;
					}
					continue;
				}
				double time_stemp = (nPTS - video_start_pts)* av_q2d(m_pVideoStream->time_base);
				last_time_stamp = time_stemp;
				m_ProcessBar = min(100.0,time_stemp/record_duration * 100.0);
				byte* pResY = NULL;
				byte* pResU = NULL;
				byte* pResV = NULL;
				int nResStride_y = 0;
				int nResStride_u = 0;
				int nResStride_v = 0;
				if (yuv_sws_ctx)
				{
					//–Ë“™◊™≥…YUV420P
					ret = sws_scale(yuv_sws_ctx, m_pSrcFrame->data, m_pSrcFrame->linesize, 0, m_pSrcFrame->height, m_YuvFrame->data, m_YuvFrame->linesize);
					if (ret < 0)
					{
						LOGE("error in sws_scale.");
						return -1;
					}
					pResY = m_YuvFrame->data[0];
					pResU = m_YuvFrame->data[1];
					pResV = m_YuvFrame->data[2];
					nResStride_y = m_YuvFrame->linesize[0];
					nResStride_u = m_YuvFrame->linesize[1];
					nResStride_v = m_YuvFrame->linesize[2];

					if(m_Rotate != MT_VIDEO_ROTATE_0)
					{
						pResY = pResYuvData;
						pResU = pResYuvData + m_RealEncodeWidth*m_RealEncodeHeight;
						pResV = pResU + m_RealEncodeWidth*m_RealEncodeHeight / 4;

						I420Rotate(m_YuvFrame->data[0], m_YuvFrame->linesize[0],
							m_YuvFrame->data[1], m_YuvFrame->linesize[1],
							m_YuvFrame->data[2], m_YuvFrame->linesize[2], 
							pResY, m_RealEncodeWidth,
							pResU, m_RealEncodeWidth / 2, pResV, m_RealEncodeWidth / 2,
							m_YuvFrame->width, m_YuvFrame->height, nYuvRotateMode);

						nResStride_y = m_RealEncodeWidth;
						nResStride_u = m_RealEncodeWidth/2;
						nResStride_v = m_RealEncodeWidth/2;

					}
				}
				else
				{
					pResY = m_pSrcFrame->data[0];
					pResU = m_pSrcFrame->data[1];
					pResV = m_pSrcFrame->data[2];
					nResStride_y = m_pSrcFrame->linesize[0];
					nResStride_u = m_pSrcFrame->linesize[1];
					nResStride_v = m_pSrcFrame->linesize[2];

					if(m_Rotate != MT_VIDEO_ROTATE_0)
					{
						pResY = pResYuvData;
						pResU = pResYuvData + m_RealEncodeWidth*m_RealEncodeHeight;
						pResV = pResU + m_RealEncodeWidth*m_RealEncodeHeight / 4;

						I420Rotate(m_pSrcFrame->data[0], m_pSrcFrame->linesize[0],
							m_pSrcFrame->data[1], m_pSrcFrame->linesize[1],
							m_pSrcFrame->data[2], m_pSrcFrame->linesize[2], 
							pResY, m_RealEncodeWidth,
							pResU, m_RealEncodeWidth / 2, pResV, m_RealEncodeWidth / 2,
							m_pSrcFrame->width, m_pSrcFrame->height, nYuvRotateMode);

						nResStride_y = m_RealEncodeWidth;
						nResStride_u = m_RealEncodeWidth/2;
						nResStride_v = m_RealEncodeWidth/2;

					}
				}
				ret = recorder.RecordI420(pResY, nResStride_y, 
					pResU, nResStride_u, 
					pResV, nResStride_v, m_RealEncodeWidth, m_RealEncodeHeight, time_stemp);
				if(ret < 0)
				{
					av_packet_unref(&packet);
					goto finish;
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

	if (m_ExitAudio == false)
	{
		//≤ª¥Ê‘⁄“Ù∆µ£¨ÃÌº”ø’“Ù∆µ
		int DataSize = (endTime - startTime + 0.1) * 44100 * 2;
		const int MaxDataSize = 100000;
		if(DataSize < MaxDataSize)
		{
			byte* pData = new byte[DataSize];
			memset(pData, 0, DataSize);
			recorder.RecordPCM(&pData, DataSize / 2);
			SAFE_DELETE_ARRAY(pData);
		}
		else
		{
			//“ª∂Œ“ª∂Œ–¥
			byte* pData = new byte[MaxDataSize];
			while(DataSize > 0)
			{
				int readSize = min(DataSize,MaxDataSize);
				memset(pData, 0, readSize);
				recorder.RecordPCM(&pData, readSize / 2);
				DataSize -= readSize;
			}
			SAFE_DELETE_ARRAY(pData);
		}
	}

finish:
	recorder.Finish();
	recorder.Close();
	SAFE_DELETE_ARRAY(pResYuvData);
	m_ProcessBar = 100.0;
	//ÕÍ≥…£¨»°œ˚÷–∂œ
    
    if (listener) {
        if (m_bInterrupt) {
            listener->videoEditerAnyProgressCanceled(this);
        } else {
            listener->videoEditerAnyProgressEnded(this);
        }
    }
	m_bInterrupt = false;
    
	return ret;
}

int CVideoEditerAny::CutVideoWithFrame(const char* dstFile, double startTime, double endTime,
                                       unsigned char r,unsigned char g,unsigned char b,MTTargetVideoSize sizeMode)
{
    if (m_pFormatContext == NULL)
    {
        LOGE("No any video is open!");
        return -1;
    }
    
    //展示到屏幕上的宽高尺寸(如果视频是带旋转角度的，需要旋转)
    int showTargetVideoWidth = m_StandSize;
    int showTargetVideoHeight = m_StandSize;
    switch (sizeMode) {
        case MT_TARGET_VIDEO_1_1:
            break;
        case MT_TARGET_VIDEO_3_4:
            showTargetVideoHeight = m_StandSize;
            showTargetVideoWidth = m_StandSize / 4 * 3;
            break;
        case MT_TARGET_VIDEO_4_3:
            showTargetVideoWidth = m_StandSize;
            showTargetVideoHeight = m_StandSize / 4 * 3;
            break;
        case MT_TARGET_VIDEO_9_16:
            showTargetVideoHeight = m_StandSize;
            showTargetVideoWidth = m_StandSize / 16 * 9;
            break;
        case MT_TARGET_VIDEO_16_9:
            showTargetVideoWidth = m_StandSize;
            showTargetVideoHeight = m_StandSize / 16 * 9;
            break;
        default:
            break;
    }
    //保证宽高为偶数
    while (showTargetVideoWidth & 1) {
        -- showTargetVideoWidth;
    }
    while (showTargetVideoHeight & 1) {
        -- showTargetVideoHeight;
    }
    
    //视频实际的宽高尺寸（选择前视频的宽高）
    int realTargetVideoWidth = showTargetVideoWidth;
    int realTargetVideoHeight = showTargetVideoHeight;
    if (m_Rotate == MT_VIDEO_ROTATE_90 || m_Rotate == MT_VIDEO_ROTATE_270)
    {
        realTargetVideoHeight = showTargetVideoWidth;
        realTargetVideoWidth = showTargetVideoHeight;
    }

    //转成yuv
    unsigned char colorY = RGBToY(r,g,b);//r *  0.299000f + g *  0.587000f + b *  0.114000f;
    unsigned char colorU = RGBToU(r,g,b);;//r * -0.168736f + g * -0.331264f + b *  0.500000f + 128;
    unsigned char colorV = RGBToV(r,g,b);;//r *  0.500000f + g * -.418688f  + b *  -0.081312f + 128;
    
    m_ProcessBar = 0.0;
    m_bInterrupt = false;
    
    double duration = this->GetVideoDuration() + 0.01;
    
    //增加容错
    if(endTime > duration)
    {
        endTime = duration;
    }
    if (startTime >= endTime || startTime > duration || endTime > duration)
    {
        LOGE("invalid parameter. start:%f,end:%f,duration:%f", startTime, endTime, duration);
        return -1;
    }
    double record_duration = endTime - startTime + 0.01;
    CFrameRecorder recorder;
    //目标图大小;
    recorder.Open(dstFile, showTargetVideoWidth, showTargetVideoHeight);
    if (m_pVideoStream->avg_frame_rate.den && m_pVideoStream->avg_frame_rate.num)
    {
        double fps = av_q2d(m_pVideoStream->avg_frame_rate);
        if(fps > 30.1f)
        {
            recorder.SetMaxFrameRate30(true);
        }
    }
    recorder.SetupCropRegion(0, 0, showTargetVideoWidth, showTargetVideoHeight, 0);
    recorder.SetWatermark(m_WatermarkPath,m_WatermarkType);
    recorder.SetEndingWatermark(m_EndingWatermarkPath,m_EndingWatermarkType,endTime - startTime);
    recorder.AdjustVideoDuration(true,endTime - startTime);
    if (m_ExitAudio)
    {
        recorder.SetupAudio(m_pAudioCodec->channels, m_pAudioCodec->sample_rate, m_pAudioCodec->sample_fmt);
    }
    else
    {
        recorder.SetupAudio(1, 44100, AV_SAMPLE_FMT_S16);
    }
    
    recorder.SetupAudioAligen(true);
    
    recorder.Start();
    
    double video_pts_base = m_pVideoStream->time_base.num / (double)m_pVideoStream->time_base.den;
    int ret = -1;
    int frameFinished = 0;
    int64_t video_start_pts = (startTime*(m_pVideoStream->time_base.den)) / (m_pVideoStream->time_base.num) + 0.5;
    int64_t video_end_pts = (endTime*(m_pVideoStream->time_base.den)) / (m_pVideoStream->time_base.num) + 0.5;
    int64_t audio_start_pts = 0;
    int64_t audio_end_pts = 0;
    if (m_ExitAudio)
    {
        audio_start_pts = (startTime*(m_pAudioStream->time_base.den)) / (m_pAudioStream->time_base.num) + 0.5;
        audio_end_pts = (endTime*(m_pAudioStream->time_base.den)) / (m_pAudioStream->time_base.num) + 0.5;
    }
    
    bool nVideoFlag = false;//视频是否已经截取完
    bool nAudioFlag = false;//音频是否已经截取完
    double last_time_stamp = 0.0;
    //---------------seek-------------------------------;
    int64_t seekTime = startTime*TIME_MICROSECOND;
    ret = avformat_seek_file(m_pFormatContext, -1, INT64_MIN, seekTime, INT64_MAX, 0);
    avcodec_flush_buffers(m_pVideoCodec);
    /*if (av_seek_frame(m_pFormatContext, m_video_stream_idx, video_start_pts, AVSEEK_FLAG_BACKWARD) < 0)
     {
     LOGE("Failed to seek Video \n");
     }
     
     if (m_ExitAudio)
     {
     if (av_seek_frame(m_pFormatContext, m_audio_stream_idx, audio_start_pts, AVSEEK_FLAG_BACKWARD) < 0)
     {
     LOGE("Failed to seek Audio \n");
     }
     }
     avcodec_flush_buffers(m_pVideoCodec);*/
    //------------------end seek----------------------;
    
    //------------------begin fill config--------------;
    int targetVideoMaxSide = max(realTargetVideoWidth, realTargetVideoHeight);
    int nDstVideoWidth = 0,nDstVideoHeight = 0;
    if(m_VideoWidth > m_VideoHeight)
    {
        nDstVideoWidth = targetVideoMaxSide;
        nDstVideoHeight = (float)nDstVideoWidth/m_VideoWidth * m_VideoHeight;
        //保证要填充的黑白边能够被4整除
        while((realTargetVideoHeight - nDstVideoHeight)%4 != 0)
        {
            nDstVideoHeight--;
        }
    }
    else
    {
        nDstVideoHeight = targetVideoMaxSide;
        nDstVideoWidth = (float)nDstVideoHeight/m_VideoHeight * m_VideoWidth;
        //保证要填充的黑白边能够被4整除
        while((realTargetVideoWidth - nDstVideoWidth)%4 != 0)
        {
            nDstVideoWidth--;
        }
    }
    
    unsigned char* pFillData = NULL;
    struct SwsContext* fill_sws_ctx = NULL;
    AVFrame* fillFrame = NULL;
    {
        //need scale
        fill_sws_ctx = sws_getContext(
                                     m_VideoWidth,
                                     m_VideoHeight,
                                     m_pVideoCodec->pix_fmt,
                                     nDstVideoWidth,
                                     nDstVideoHeight,
                                     AV_PIX_FMT_YUV420P,
                                     SWS_AREA, NULL, NULL, NULL);
        
        pFillData = new unsigned char[realTargetVideoWidth*realTargetVideoHeight*3/2];
        if(pFillData == NULL)
        {
            LOGE("pFillData memory error!");
            return -1;
        }
    }
    
    //申请一张YUV帧(缩放用途)
    fillFrame = av_frame_alloc();
    {
        int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, nDstVideoWidth, nDstVideoHeight, 1);
        uint8_t *buffer = (uint8_t *)av_malloc_array(1, numBytes);
        int len = av_image_fill_arrays(fillFrame->data, fillFrame->linesize, buffer, AV_PIX_FMT_YUV420P, nDstVideoWidth, nDstVideoHeight, 1);
        fillFrame->width = nDstVideoWidth;
        fillFrame->height = nDstVideoHeight;
    }
    if (fillFrame == NULL)
    {
        LOGE("yuv frame memory error!");
        return -1;
    }
    
    //--------------end fill config----------------;
    
    unsigned char* pResYuvData = NULL;
    if (m_Rotate != MT_VIDEO_ROTATE_0)
    {
        pResYuvData = new unsigned char[showTargetVideoWidth * showTargetVideoHeight * 3 / 2];
        if(pResYuvData == NULL)
        {
            LOGE("pResYuvData memory error!");
            return -1;
        }
    }
    LOGE("wfc rotate = %d", m_Rotate);
    
    //无帧的方案
    bool bDecodeFrame = false;
    bool bEncodeFrame = false;
    AVFrame* pZeroFrame = NULL;
    AVPacket packet = { 0 };
    av_init_packet(&packet);
    RotationMode nYuvRotateMode = kRotate0;
    //convert rotate
    switch(m_Rotate)
    {
        case MT_VIDEO_ROTATE_0:
            nYuvRotateMode = kRotate0;
            break;
        case MT_VIDEO_ROTATE_90:
            nYuvRotateMode = kRotate90;
            break;
        case MT_VIDEO_ROTATE_180:
            nYuvRotateMode = kRotate180;
            break;
        case MT_VIDEO_ROTATE_270:
            nYuvRotateMode = kRotate270;
            break;
    }
    if (listener) {
        listener->videoEditerAnyProgressBegan(this);
    }
    
    while (av_read_frame(m_pFormatContext, &packet) >= 0)
    {
        if(m_bInterrupt)
        {
            //中断导入
            nVideoFlag = true;
            LOGE("wfcan videoediter interrupt CutVideoWithTime finish.");
            break;
        }
        if (packet.stream_index == m_video_stream_idx && nVideoFlag == false)
        {
            ret = avcodec_decode_video2(m_pVideoCodec, m_pSrcFrame, &frameFinished, &packet);
            if (frameFinished)
            {
                int64_t nPTS = av_frame_get_best_effort_timestamp(m_pSrcFrame);
                if(pZeroFrame == NULL)
                {
                    pZeroFrame = av_frame_clone(m_pSrcFrame);
                }
                bDecodeFrame = true;
                if (nPTS < video_start_pts)
                {
                    av_packet_unref(&packet);
                    av_init_packet(&packet);
                    continue;
                }
                if (nPTS > video_end_pts)
                {
                    nVideoFlag = true;
                    if (nAudioFlag || m_ExitAudio == false)
                    {
                        break;
                    }
                    av_packet_unref(&packet);
                    av_init_packet(&packet);
                    continue;
                }
                double time_stemp = (nPTS - video_start_pts)* av_q2d(m_pVideoStream->time_base);
                if(!bEncodeFrame)
                {
                    //首帧0.0
                    time_stemp = 0.0;
                    bEncodeFrame = true;
                }
                last_time_stamp = time_stemp;
                m_ProcessBar = min(100.0,time_stemp/record_duration * 100.0);
                if (listener) {
                    listener->videoEditerAnyProgressChanged(this, time_stemp, record_duration);
                }
                byte* pResY = NULL;
                byte* pResU = NULL;
                byte* pResV = NULL;
                int nResStride_y = 0;
                int nResStride_u = 0;
                int nResStride_v = 0;
                
                //需要转成YUV420P
                ret = sws_scale(fill_sws_ctx, m_pSrcFrame->data, m_pSrcFrame->linesize, 0, m_pSrcFrame->height, fillFrame->data, fillFrame->linesize);
                if (ret < 0)
                {
                    LOGE("error in sws_scale.");
                    SAFE_DELETE_ARRAY(pResYuvData);
                    return -1;
                }
                
                FillVideoFrame(pFillData,fillFrame,colorY,colorU,colorV,realTargetVideoWidth,realTargetVideoHeight);
                
                byte* pDstY = pFillData;
                byte* pDstU = pFillData + realTargetVideoWidth * realTargetVideoHeight;
                byte* pDstV = pDstU + realTargetVideoWidth * realTargetVideoHeight / 4;
                pResY = pDstY;
                pResU = pDstU;
                pResV = pDstV;
                
                nResStride_y = realTargetVideoWidth;
                nResStride_u = realTargetVideoWidth / 2;
                nResStride_v = realTargetVideoWidth / 2;
                
                if(m_Rotate != MT_VIDEO_ROTATE_0)
                {
                    pResY = pResYuvData;
                    pResU = pResYuvData + showTargetVideoWidth * showTargetVideoHeight;
                    pResV = pResU + showTargetVideoWidth * showTargetVideoHeight / 4;
                    
                    
                    I420Rotate(pDstY, nResStride_y,
                               pDstU, nResStride_u,
                               pDstV, nResStride_v,
                               pResY, showTargetVideoWidth,
                               pResU, showTargetVideoWidth / 2,
                               pResV, showTargetVideoWidth / 2,
                               realTargetVideoWidth, realTargetVideoHeight, nYuvRotateMode);
                    
                    nResStride_y = showTargetVideoWidth;
                    nResStride_u = showTargetVideoWidth / 2;
                    nResStride_v = showTargetVideoWidth / 2;
                }
                ret = recorder.RecordI420(pResY, nResStride_y,
                                          pResU, nResStride_u,
                                          pResV, nResStride_v, showTargetVideoWidth, showTargetVideoHeight, time_stemp);
                if(ret < 0)
                {
                    av_packet_unref(&packet);
                    goto finish;
                }
            }
        }
        else if (packet.stream_index == m_audio_stream_idx && nAudioFlag == false)
        {
            AVPacket tmpPack;
            tmpPack.data = packet.data;
            tmpPack.size = packet.size;
            while (packet.size > 0)
            {
                ret = avcodec_decode_audio4(m_pAudioCodec, m_pSrcFrame, &frameFinished, &packet);
                packet.size -= ret;
                packet.data += ret;
                if (frameFinished)
                {
                    int64_t nPTS = av_frame_get_best_effort_timestamp(m_pSrcFrame);
                    double time_stemp = (nPTS - audio_start_pts)* av_q2d(m_pAudioStream->time_base);
                    
                    if (nPTS < audio_start_pts)
                    {
                        break;
                    }
                    if (nPTS > audio_end_pts)
                    {
                        nAudioFlag = true;
                        break;
                    }
                    
                    recorder.RecordPCM(m_pSrcFrame->extended_data, m_pSrcFrame->nb_samples);
                }
            }
            packet.data = tmpPack.data;
            packet.size = tmpPack.size;
            
        }
        av_packet_unref(&packet);
        av_init_packet(&packet);
        if (nAudioFlag && nVideoFlag)
        {
            break;
        }
        
    }
    av_packet_unref(&packet);
    
    
    if (m_pVideoCodec && !nVideoFlag)
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
            if (frameFinished)
            {
                int64_t nPTS = av_frame_get_best_effort_timestamp(m_pSrcFrame);
                if (nPTS < video_start_pts)
                {
                    av_packet_unref(&packet);
                    continue;
                }
                if (nPTS > video_end_pts)
                {
                    nVideoFlag = true;
                    av_packet_unref(&packet);
                    if (nAudioFlag || m_ExitAudio == false)
                    {
                        break;
                    }
                    continue;
                }
                double time_stemp = (nPTS - video_start_pts)* av_q2d(m_pVideoStream->time_base);
                last_time_stamp = time_stemp;
                m_ProcessBar = min(100.0,time_stemp/record_duration * 100.0);
                byte* pResY = NULL;
                byte* pResU = NULL;
                byte* pResV = NULL;
                int nResStride_y = 0;
                int nResStride_u = 0;
                int nResStride_v = 0;
                
                //需要转成YUV420P
                ret = sws_scale(fill_sws_ctx, m_pSrcFrame->data, m_pSrcFrame->linesize, 0, m_pSrcFrame->height, fillFrame->data, fillFrame->linesize);
                if (ret < 0)
                {
                    LOGE("error in sws_scale.");
                    return -1;
                }
                FillVideoFrame(pFillData,fillFrame,colorY,colorU,colorV,realTargetVideoWidth,realTargetVideoHeight);
                
                byte* pDstY = pFillData;
                byte* pDstU = pFillData + realTargetVideoWidth * realTargetVideoHeight;
                byte* pDstV = pDstU + realTargetVideoWidth * realTargetVideoHeight / 4;
                pResY = pDstY;
                pResU = pDstU;
                pResV = pDstV;
                
                nResStride_y = realTargetVideoWidth;
                nResStride_u = realTargetVideoWidth / 2;
                nResStride_v = realTargetVideoWidth / 2;
                
                if(m_Rotate != MT_VIDEO_ROTATE_0)
                {
                    pResY = pResYuvData;
                    pResU = pResYuvData + showTargetVideoWidth * showTargetVideoHeight;
                    pResV = pResU + showTargetVideoWidth * showTargetVideoHeight / 4;
                    
                    
                    I420Rotate(pDstY, nResStride_y,
                               pDstU, nResStride_u,
                               pDstV, nResStride_v,
                               pResY, showTargetVideoWidth,
                               pResU, showTargetVideoWidth / 2,
                               pResV, showTargetVideoWidth / 2,
                               realTargetVideoWidth, realTargetVideoHeight, nYuvRotateMode);
                    
                    nResStride_y = showTargetVideoWidth;
                    nResStride_u = showTargetVideoWidth / 2;
                    nResStride_v = showTargetVideoWidth / 2;
                }
                ret = recorder.RecordI420(pResY, nResStride_y,
                                          pResU, nResStride_u,
                                          pResV, nResStride_v, showTargetVideoWidth, showTargetVideoHeight, time_stemp);
                if(ret < 0)
                {
                    av_packet_unref(&packet);
                    goto finish;
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
    //一帧都没有
    if(bEncodeFrame == false && bDecodeFrame == false)
    {
        LOGE("no any frame in video");
        ret = -1;
        goto finish;
    }
    
    if(bDecodeFrame == true && !bEncodeFrame && pZeroFrame)
    {
        double time_stemp = 0.00;
        last_time_stamp = time_stemp;
        m_ProcessBar = min(100.0,time_stemp/record_duration * 100.0);
        byte* pResY = NULL;
        byte* pResU = NULL;
        byte* pResV = NULL;
        int nResStride_y = 0;
        int nResStride_u = 0;
        int nResStride_v = 0;
        
        //需要转成YUV420P
        ret = sws_scale(fill_sws_ctx, pZeroFrame->data, pZeroFrame->linesize, 0, pZeroFrame->height, fillFrame->data, fillFrame->linesize);
        if (ret < 0)
        {
            LOGE("error in sws_scale.");
            return -1;
        }
        
        FillVideoFrame(pFillData,fillFrame,colorY,colorU,colorV,realTargetVideoWidth,realTargetVideoHeight);
        
        byte* pDstY = pFillData;
        byte* pDstU = pFillData + realTargetVideoWidth * realTargetVideoHeight;
        byte* pDstV = pDstU + realTargetVideoWidth * realTargetVideoHeight / 4;
        pResY = pDstY;
        pResU = pDstU;
        pResV = pDstV;
        
        nResStride_y = realTargetVideoWidth;
        nResStride_u = realTargetVideoWidth / 2;
        nResStride_v = realTargetVideoWidth / 2;
        
        if(m_Rotate != MT_VIDEO_ROTATE_0)
        {
            pResY = pResYuvData;
            pResU = pResYuvData + showTargetVideoWidth * showTargetVideoHeight;
            pResV = pResU + showTargetVideoWidth * showTargetVideoHeight / 4;

            I420Rotate(pDstY, nResStride_y,
                       pDstU, nResStride_u,
                       pDstV, nResStride_v,
                       pResY, showTargetVideoWidth,
                       pResU, showTargetVideoWidth / 2,
                       pResV, showTargetVideoWidth / 2,
                       realTargetVideoWidth, realTargetVideoHeight, nYuvRotateMode);
            
            nResStride_y = showTargetVideoWidth;
            nResStride_u = showTargetVideoWidth / 2;
            nResStride_v = showTargetVideoWidth / 2;
        }
        ret = recorder.RecordI420(pResY, nResStride_y,
                                  pResU, nResStride_u,
                                  pResV, nResStride_v, showTargetVideoWidth, showTargetVideoHeight, time_stemp);
    }
    if(pZeroFrame)
    {
        av_frame_free(&pZeroFrame);
        pZeroFrame = NULL;
    }
    if (m_ExitAudio == false)
    {
        //不存在音频，添加空音频
        int DataSize = (endTime - startTime + 0.1) * 44100 * 2;
        const int MaxDataSize = 100000;
        if(DataSize < MaxDataSize)
        {
            byte* pData = new byte[DataSize];
            memset(pData, 0, DataSize);
            recorder.RecordPCM(&pData, DataSize / 2);
            SAFE_DELETE_ARRAY(pData);
        }
        else
        {
            //一段一段写
            byte* pData = new byte[MaxDataSize];
            while(DataSize > 0)
            {
                int readSize = min(DataSize,MaxDataSize);
                memset(pData, 0, readSize);
                recorder.RecordPCM(&pData, readSize / 2);
                DataSize -= readSize;
            }
            SAFE_DELETE_ARRAY(pData);
        }
    }
    
finish:
    recorder.Finish();
    recorder.Close();
    SAFE_DELETE_ARRAY(pResYuvData);
    SAFE_DELETE_ARRAY(pFillData);
    if (fillFrame)
    {
        if (fillFrame->data[0])
        {
            av_free(fillFrame->data[0]);
            fillFrame->data[0] = NULL;
        }
        av_frame_free(&fillFrame);
        fillFrame = NULL;
    }
    if (fill_sws_ctx)
    {
        sws_freeContext(fill_sws_ctx);
        fill_sws_ctx = NULL;
    }
    
    m_ProcessBar = 100.0;
    if (listener) {
        if (m_bInterrupt) {
            listener->videoEditerAnyProgressCanceled(this);
        } else {
            listener->videoEditerAnyProgressEnded(this);
        }
    }
    //完成，取消中断
    m_bInterrupt = false;
    return ret;
}


void CVideoEditerAny::SetWaterMark( const char* watermark_path, MeipaiWatermarkType type )
{
	if(watermark_path)
	{
		
		//º«¬º¬∑æ∂
		SAFE_DELETE_ARRAY(m_WatermarkPath);
		int len = strlen(watermark_path);
		m_WatermarkPath = new char[len + 1];
		strcpy(m_WatermarkPath,watermark_path);
		m_WatermarkType = type;
	}
	else
	{
		m_WatermarkType = WATERMARK_NONE;
	}
}

void CVideoEditerAny::SetEndingWaterMark( const char* watermark_path,MeipaiWatermarkType type /*= WATERMARK_CENTER*/ )
{
	if(watermark_path)
	{

		//º«¬º¬∑æ∂
		SAFE_DELETE_ARRAY(m_EndingWatermarkPath);
		int len = strlen(watermark_path);
		m_EndingWatermarkPath = new char[len + 1];
		strcpy(m_EndingWatermarkPath,watermark_path);
		m_EndingWatermarkType = type;
	}
	else
	{
		m_EndingWatermarkType = WATERMARK_NONE;
	}
}

double CVideoEditerAny::GetProgressbarValue()
{
	return m_ProcessBar;
}

void CVideoEditerAny::ClearProgressBarValue()
{
	m_ProcessBar = 0.0;
}

void CVideoEditerAny::Interrupt()
{
	m_bInterrupt = true;
}

//============================================

int CVideoEditerAny::seekTo(int64_t timeUs) {
   	if (m_pFormatContext == NULL)
    {
        LOGE("No any video is open!");
        return -1;
    }
    m_ProcessBar = 0.0;
    m_bInterrupt = false;
    
    double duration = this->GetVideoDuration() + 0.01;
    double startTime = timeUs/TIME_MICROSECOND;
    
    if (startTime > duration)
    {
        LOGE("invalid parameter. start:%f,duration:%f", startTime, duration);
        return -1;
    }
    
    //---------------seek-------------------------------;
    int64_t seekTime = timeUs;
    int ret = avformat_seek_file(m_pFormatContext, -1, INT64_MIN, seekTime, INT64_MAX, 0);
    
    if (ret >= 0) {
        avcodec_flush_buffers(m_pVideoCodec);
        
        if (m_Rotate != MT_VIDEO_ROTATE_0)
        {
            pResYuvData = new unsigned char[m_RealEncodeWidth*m_RealEncodeHeight * 3 / 2];
            resYuvDataSize = m_RealEncodeWidth*m_RealEncodeHeight * 3 / 2;
        }
        
        //convert rotate
        switch(m_Rotate)
        {
            case MT_VIDEO_ROTATE_0:
                nYuvRotateMode = kRotate0;
                break;
            case MT_VIDEO_ROTATE_90:
                nYuvRotateMode = kRotate90;
                break;
            case MT_VIDEO_ROTATE_180:
                nYuvRotateMode = kRotate180;
                break;
            case MT_VIDEO_ROTATE_270:
                nYuvRotateMode = kRotate270;
                break;
        }
    }
    return ret;
}

int CVideoEditerAny::getAudioTrackIndex() {
    return m_audio_stream_idx;
}

int CVideoEditerAny::getVideoTrakIndex() {
    return m_video_stream_idx;
}

int CVideoEditerAny::getSampleTrackIndex() {
    
    if (packet.data != NULL) {
        return packet.stream_index;
    }
    
    int ret = 0;

    while ((ret = av_read_frame(m_pFormatContext, &packet)) >= 0)
    {
        if (packet.stream_index == m_video_stream_idx )
        {
            break;
        }
        else if (packet.stream_index == m_audio_stream_idx)
        {
            break;
        }
    }

    return ret >= 0 ? packet.stream_index : -1;
}

bool CVideoEditerAny::advance() {
    av_packet_unref(&packet);
    av_init_packet(&packet);

    auto index = getSampleTrackIndex();
    
    return index >= 0;
}

int CVideoEditerAny::readSample(uint8 *buffer) {
    // copy buffer to buffer.
    if (buffer) {
        memcpy(buffer, packet.data, packet.size);
    }
    
    return packet.size;
}

int CVideoEditerAny::decodeVideo(uint8 *buffer, int32_t size, int64_t &timeSample, int32_t &outSize) {
    int frameFinished = 0;
    int ret = 0;
    
    ret = avcodec_decode_video2(m_pVideoCodec, m_pSrcFrame, &frameFinished, &packet);
    
    if (frameFinished) {
        int64_t nPTS = av_frame_get_best_effort_timestamp(m_pSrcFrame);
        
        double time_stemp = (nPTS)* av_q2d(m_pVideoStream->time_base);
        timeSample = time_stemp * TIME_MICROSECOND;
        byte* pResY = NULL;
        byte* pResU = NULL;
        byte* pResV = NULL;
        int nResStride_y = 0;
        int nResStride_u = 0;
        int nResStride_v = 0;
        
        if (yuv_sws_ctx) {
            //–Ë“™◊™≥…YUV420P
            ret = sws_scale(yuv_sws_ctx, m_pSrcFrame->data, m_pSrcFrame->linesize, 0, m_pSrcFrame->height, m_YuvFrame->data, m_YuvFrame->linesize);
            if (ret < 0) {
                LOGE("error in sws_scale.");
                return -1;
            }
            pResY = m_YuvFrame->data[0];
            pResU = m_YuvFrame->data[1];
            pResV = m_YuvFrame->data[2];
            nResStride_y = m_YuvFrame->linesize[0];
            nResStride_u = m_YuvFrame->linesize[1];
            nResStride_v = m_YuvFrame->linesize[2];
            
            if(m_Rotate != MT_VIDEO_ROTATE_0) {
                pResY = pResYuvData;
                pResU = pResYuvData + m_RealEncodeWidth*m_RealEncodeHeight;
                pResV = pResU + m_RealEncodeWidth*m_RealEncodeHeight / 4;
                
                I420Rotate(m_YuvFrame->data[0], m_YuvFrame->linesize[0],
                           m_YuvFrame->data[1], m_YuvFrame->linesize[1],
                           m_YuvFrame->data[2], m_YuvFrame->linesize[2],
                           pResY, m_RealEncodeWidth,
                           pResU, m_RealEncodeWidth / 2, pResV, m_RealEncodeWidth / 2,
                           m_YuvFrame->width, m_YuvFrame->height, nYuvRotateMode);
                
                nResStride_y = m_RealEncodeWidth;
                nResStride_u = m_RealEncodeWidth/2;
                nResStride_v = m_RealEncodeWidth/2;
                
            }
        }
        else {
            pResY = m_pSrcFrame->data[0];
            pResU = m_pSrcFrame->data[1];
            pResV = m_pSrcFrame->data[2];
            nResStride_y = m_pSrcFrame->linesize[0];
            nResStride_u = m_pSrcFrame->linesize[1];
            nResStride_v = m_pSrcFrame->linesize[2];
            
            if(m_Rotate != MT_VIDEO_ROTATE_0) {
                pResY = pResYuvData;
                pResU = pResYuvData + m_RealEncodeWidth*m_RealEncodeHeight;
                pResV = pResU + m_RealEncodeWidth*m_RealEncodeHeight / 4;
                
                I420Rotate(m_pSrcFrame->data[0], m_pSrcFrame->linesize[0],
                           m_pSrcFrame->data[1], m_pSrcFrame->linesize[1],
                           m_pSrcFrame->data[2], m_pSrcFrame->linesize[2],
                           pResY, m_RealEncodeWidth,
                           pResU, m_RealEncodeWidth / 2, pResV, m_RealEncodeWidth / 2,
                           m_pSrcFrame->width, m_pSrcFrame->height, nYuvRotateMode);
                
                nResStride_y = m_RealEncodeWidth;
                nResStride_u = m_RealEncodeWidth/2;
                nResStride_v = m_RealEncodeWidth/2;
            }
            
        }
        
        // copy data from YUV
        byte* pDstY = buffer;
        byte* pDstU = buffer + m_RealEncodeWidth*m_RealEncodeHeight;
        byte* pDstV = pDstU + m_RealEncodeWidth*m_RealEncodeHeight / 4;
        
        int dst_stride_y = m_RealEncodeWidth;
        int dst_stride_u = m_RealEncodeWidth / 2;
        int dst_stride_v = m_RealEncodeWidth / 2;
        
        unsigned char* pSrcY = pResY;
        unsigned char* pSrcU = pResU;
        unsigned char* pSrcV = pResV;
        
        int src_stride_y = nResStride_y;
        int src_stride_u = nResStride_u;
        int src_stride_v = nResStride_v;
        
        for (int i = 0; i < m_RealEncodeHeight; i++) {
            memcpy(pDstY, pSrcY, dst_stride_y);
            pSrcY += src_stride_y;
            pDstY += dst_stride_y;
            
            if (i & 1) {
                memcpy(pDstU, pSrcU, dst_stride_u);
                memcpy(pDstV, pSrcV, dst_stride_v);
                pDstU += dst_stride_u;
                pDstV += dst_stride_v;
                pSrcU += src_stride_u;
                pSrcV += src_stride_v;
            }
        }
        
        outSize = resYuvDataSize;
        
    }
    
    av_packet_unref(&packet);
    av_init_packet(&packet);
    
    return frameFinished;
}

int32_t CVideoEditerAny::getOutputBufferSize() {
    return m_RealEncodeWidth*m_RealEncodeHeight * 3 / 2;
}

int64_t CVideoEditerAny::getSampleTime() {
    int64_t timeSampeUS = -1;
    if (packet.data != NULL) {
        double time_stemp = (packet.pts)* av_q2d(m_pAudioStream->time_base);
        timeSampeUS = time_stemp*TIME_MICROSECOND;
    }
    
    return timeSampeUS;
}

int CVideoEditerAny::getSampleFlags() {
    return packet.flags;
}

const std::pair<int, int> CVideoEditerAny::getRealOutputSize() {
    return std::make_pair(m_RealEncodeWidth, m_RealEncodeHeight);
}

void CVideoEditerAny::setProgressListener(VideoEditerAnyProgressListener *listener) {
    SAFE_DELETE(this->listener);
    this->listener = listener;
}

VideoEditerAnyProgressListener* CVideoEditerAny::getProgressListener() {
    return this->listener;
}
