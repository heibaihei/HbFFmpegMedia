#include "FrameRecorder.h"
#include "rotate_yuv.h"
#include "MTMacros.h"
#include "LogHelper.h"

#ifndef makeErrorStr
static char errorStr[AV_ERROR_MAX_STRING_SIZE];
#define makeErrorStr(errorCode) av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, errorCode)
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

pthread_mutex_t m_mutex;

static int ff_lockmgr_callback(void **mutex, enum AVLockOp op)
{
	switch(op)
	{
	case AV_LOCK_CREATE:///< Create a mutex
		{
			//LOGE("ffmpeg mutex init");
			*mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
//#ifndef WIN32
			pthread_mutex_init((pthread_mutex_t*)*mutex,NULL);
//#endif
			//*mutex = &g_ffmpegmutex;
		}
		break;
	case AV_LOCK_OBTAIN:///< Lock the mutex
		{
			//LOGE("ffmpeg start lock");
//#ifndef WIN32
			pthread_mutex_lock((pthread_mutex_t*)(*mutex));
//#endif // !WIN32
			//LOGE("ffmpeg finished lock");
		}
		break;
	case AV_LOCK_RELEASE:///< Unlock the mutex
		{
			//LOGE("ffmpeg start unlock");
//#ifndef WIN32
			pthread_mutex_unlock((pthread_mutex_t*)(*mutex));
//#endif
			//LOGE("ffmpeg finished unlock");
		}
		break;
	case AV_LOCK_DESTROY:///< Free mutex resources
		{
			if(*mutex)
			{
				//LOGE("ffmpeg mutex destroy");
//#ifndef WIN32
				pthread_mutex_destroy((pthread_mutex_t*)(*mutex));
//#endif
				free(*mutex);
				*mutex = NULL;
			}

		}
		break;
	default:
		break;
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



/* Add an output stream. */
static AVStream *add_stream(AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id)
{
	AVCodecContext *c;
	AVStream *st;

	/* find the encoder */
	*codec = avcodec_find_encoder(codec_id);
	if (!(*codec)) {
		LOGE("Could not find encoder for '%s'\n",
			avcodec_get_name(codec_id));
		return NULL;
	}

	st = avformat_new_stream(oc, *codec);
	if (!st) {
		LOGE("Could not allocate stream\n");
		return NULL;
	}
	st->id = oc->nb_streams - 1;
	c = st->codec;

	/* Some formats want stream headers to be separate. */
	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;

	return st;
}

static AVSampleFormat select_samplefmt_s16(AVCodec *codec)
{
	const AVSampleFormat* p = codec->sample_fmts;
	while (*p != -1)
	{
		if (*p == AV_SAMPLE_FMT_S16P)
		{
			return *p;
		}
		else if (*p == AV_SAMPLE_FMT_S16)
		{
			return *p;
		}
		p++;
	}
	return AV_SAMPLE_FMT_S16;
}

/* select layout with the highest channel count */
static uint64_t select_channel_layout(AVCodec *codec)
{
	const uint64_t *p;
	uint64_t best_ch_layout = 0;
//	int best_nb_channels = 0;

	if (!codec->channel_layouts)
		return AV_CH_LAYOUT_MONO;

	p = codec->channel_layouts;
	while (*p) {
		int nb_channels = av_get_channel_layout_nb_channels(*p);

		if (nb_channels == 1) {
			best_ch_layout = *p;
			return best_ch_layout;
		}
		p++;
	}
	return codec->channel_layouts[0];
}

void my_ffmpeg_log_callback(void* ptr, int level, const char* fmt, va_list vl)
{
	char res[1024];
	vsprintf(res,fmt,vl);
	switch(level)
	{
	case AV_LOG_ERROR:
		LOGE("%s",res);
		break;
	default:
		LOGD("%s",res);
	}
}


CFrameRecorder::CFrameRecorder()
{
    last_frame_data = NULL;
	m_LastFrameData = NULL;
    last_frame_w = last_frame_h = 0;
    last_frame_timesample = -1.0;
	m_LastTimeStamp = -1000.0f;
	m_pOutputFormatContext = NULL;
	m_pOutputFormat = NULL;
	m_pDstVideoStream = NULL;
	m_oVcc = NULL;
	m_pDstVideoCodec = NULL;
	sws_ctx = NULL;
	m_pVideoFrame = NULL;
	m_FrameRate = 0;
	m_LastFrameIndex = -1000000;
	m_VideoDuration = -1.0f;
	m_pDstAudioStream = NULL;
	m_oAcc = NULL;
	m_pDstAudioCodec = NULL;
	m_pAudioFrame = NULL;
	m_avFIFO = NULL;
	swr_ctx = NULL;
	dst_samples_data = NULL;
	dst_samples_linesize = 0;
	src_nb_samples = 0;
    this->startRecorderPrepare = 0;
    audio_pcm_length = 0;
	m_Quality = MEDIARECORDER_QINQMAX;
	m_IsMaxFrameRate30 = false;
	m_FrameInterval = 0.0333333333;
	m_RecordChannle = 1;
	m_AudioAligen = false;
	m_MaxVideoTimeStamp = 0.0;
	m_MaxAudioTimeStamp = 0.0;
	m_FillPixels = 0;
	m_FillMode = FR_FILL_NONE;
	m_WatermarkType = WATERMARK_NONE;
	m_EndingWaterType = WATERMARK_NONE;
	m_IsAdjustDuration = false;
}

CFrameRecorder::~CFrameRecorder()
{
    SAFE_FREE(last_frame_data);
	Close();
}

int CFrameRecorder::Open(const char* dstFile, int width, int height)
{
//#ifndef WIN32
	pthread_mutex_init(&m_mutex, NULL);
//#endif

	int ret = -1;
	if ((width & 1) || (height & 1) || !dstFile)
	{
		LOGE("width or height is odd or dstFile is null %s.\n", dstFile);
		return -1;
	}
	av_register_all();
	avcodec_register_all();
//	av_log_set_callback(my_ffmpeg_log_callback);
//	av_lockmgr_register (ff_lockmgr_callback);

	strcpy(m_FilePath, dstFile);
	m_pOutputFormat = av_guess_format(NULL, dstFile, NULL);
	if (m_pOutputFormat == NULL)
	{
		LOGE("Could guess format from %s.\n", dstFile);
		ret = -1;
		return ret;
	}
	m_pOutputFormatContext = avformat_alloc_context();
	if (m_pOutputFormatContext == NULL)
	{
		LOGE("Alloc format context error.\n");
		ret = -1;
		return ret;
	}
	m_pOutputFormatContext->oformat = m_pOutputFormat;
	m_pOutputFormatContext->max_interleave_delta = 61000000;
	sprintf(m_pOutputFormatContext->filename, "%s", dstFile);

	if (m_pOutputFormat->video_codec != AV_CODEC_ID_H264)
	{
		LOGD("warning: change format from %s to %s.\n", avcodec_get_name(m_pOutputFormat->video_codec), avcodec_get_name(AV_CODEC_ID_H264));
		m_pOutputFormat->video_codec = AV_CODEC_ID_H264;
	}
	if (m_pOutputFormat->audio_codec != AV_CODEC_ID_AAC)
	{
		LOGD("warning: change format from %s to %s.\n", avcodec_get_name(m_pOutputFormat->audio_codec), avcodec_get_name(AV_CODEC_ID_AAC));
		m_pOutputFormat->audio_codec = AV_CODEC_ID_AAC;
	}

	m_pDstVideoStream = add_stream(m_pOutputFormatContext, &m_pDstVideoCodec, m_pOutputFormatContext->oformat->video_codec);
	if (m_pDstVideoStream == NULL)
	{
		LOGE("Could not get video codec.(%s)\n", avcodec_get_name(m_pOutputFormatContext->oformat->video_codec));
		ret = -1;
		return ret;
	}

	m_CropLeft = 0;
	m_CropTop = 0;
	m_CropWidth = width;
	m_CropHeight = height;
	m_CropRight = width;
	m_CropBottom = height;

	return ret;
}

int CFrameRecorder::SetupQuality(MediaRecorderQuality quality)
{
	m_Quality = quality;
    return 0;
}

int CFrameRecorder::SetupCropRegion(int nLeft, int nTop, int nWidth, int nHeight,int rotate)
{
	int val=(rotate%360+360)%360;
	LOGD("Record I420  rotate:%d",val);
	switch(val)
	{
	case 0:
		m_rotateMode=kRotate0;
		break;
	case 90:
		m_rotateMode=kRotate90;
		break;
	case 180:
		m_rotateMode=kRotate180;
		break;
	case 270:
		m_rotateMode=kRotate270;
		break;
	}
	if (nLeft < 0 || nTop < 0 || nWidth <= 0 || nHeight <= 0
		|| (nLeft & 1) || (nTop & 1) || (nWidth & 1) || (nHeight & 1))
	{
		LOGE("Could not access odd parameter.\n");
		return -1;
	}
	if (m_pDstVideoStream == NULL)
	{
		LOGE("No initlized.\n");
		return -1;
	}
	int ret;
	m_CropLeft = nLeft;
	m_CropTop = nTop;
	m_CropWidth = nWidth;
	m_CropHeight = nHeight;
	m_CropRight = nLeft + nWidth;
	m_CropBottom = nTop + nHeight;
	
	if (m_pDstVideoStream)
	{
		m_FrameRate = 500;
		m_oVcc = m_pDstVideoStream->codec;
		m_oVcc->codec_id = m_pOutputFormatContext->oformat->video_codec;
		m_oVcc->codec_type = AVMEDIA_TYPE_VIDEO;
		if(m_Quality == MEDIARECORDER_QINQMAX)
		{
			m_oVcc->bit_rate = nWidth*nHeight*2;
			m_oVcc->qmin = 2;
			m_oVcc->qmax = 25;
		}
		else
		{
			m_oVcc->bit_rate = 35000000;
		}
		
		m_oVcc->width = nWidth;
		m_oVcc->height = nHeight;
		m_oVcc->time_base.num = 1;
		m_oVcc->time_base.den = m_FrameRate;
        
//            m_oVcc->gop_size = m_oVcc->time_base.den * 10;
        // 测试发现该值对保存视频的体积影响不大，但是可以提供更好的seek效果
        m_oVcc->gop_size = 30 * ceilf(mIFrameInternal);
        
		m_oVcc->keyint_min = 20;
		m_oVcc->pix_fmt = AV_PIX_FMT_YUV420P;
		m_oVcc->max_b_frames = 2;
		if (m_oVcc->codec_id == AV_CODEC_ID_H264)
		{
			av_opt_set(m_oVcc->priv_data, "level", "4.1", 0);
			av_opt_set(m_oVcc->priv_data, "preset", "superfast", 0);
			av_opt_set(m_oVcc->priv_data, "tune", "zerolatency", 0);
		}

		m_pDstVideoStream->avg_frame_rate.num = m_oVcc->time_base.den;
		m_pDstVideoStream->avg_frame_rate.den = 1;
	}
	AVDictionary *opts = NULL;  
	av_dict_set(&opts, "profile", "baseline", 0);
	if (m_oVcc && ((ret = avcodec_open2(m_oVcc, m_pDstVideoCodec, &opts)) < 0))
	{
		LOGE("frame recorder w:%d,h:%d,name:%s",m_oVcc->width,m_oVcc->height,m_oVcc->codec->name);
		LOGE("Could not open video codec.(%s)\n", makeErrorStr(ret));
		av_dict_free(&opts);
		return ret;
	}
	av_dict_free(&opts);
	m_pVideoFrame = av_frame_alloc(); //avcodec_alloc_frame();
	m_pVideoFrame->pts = 0;
	return 1;
}

int CFrameRecorder::SetupRecordChannle(int channle)
{
	if(channle == 2)
		m_RecordChannle = 2;
	else
		m_RecordChannle = 1;
	return m_RecordChannle;
}

int CFrameRecorder::SetupAudio(int channle_count, int sample_rate, AVSampleFormat sample_fmt)
{
	int ret;
	m_pDstAudioStream = add_stream(m_pOutputFormatContext, &m_pDstAudioCodec, m_pOutputFormatContext->oformat->audio_codec);
	if (m_pDstAudioStream == NULL)
	{
		LOGE("Could not get audio codec.(%s)\n", avcodec_get_name(m_pOutputFormatContext->oformat->audio_codec));
		ret = -1;
		return ret;
	}
	src_sample_rate = sample_rate;

	m_oAcc = m_pDstAudioStream->codec;

	m_oAcc->sample_fmt = select_samplefmt_s16(m_pDstAudioCodec);
	m_oAcc->codec_id = m_pOutputFormatContext->oformat->audio_codec;
	m_oAcc->codec_type = AVMEDIA_TYPE_AUDIO;
	m_oAcc->bit_rate = 64000;
	m_oAcc->sample_rate = 44100;
	if(m_RecordChannle == 1)
	{
		m_oAcc->channel_layout = select_channel_layout(m_pDstAudioCodec);/*AV_CH_LAYOUT_STEREO;//*/
	}
	else
	{
		m_oAcc->channel_layout = AV_CH_LAYOUT_STEREO;//
	}
	m_oAcc->channels = av_get_channel_layout_nb_channels(m_oAcc->channel_layout);

	if ((ret = avcodec_open2(m_oAcc, m_pDstAudioCodec, NULL) < 0))
	{
		LOGE("Could not open audio codec.(%s)\n", makeErrorStr(ret));
		if ((ret = avcodec_open2(m_oAcc, m_pDstAudioCodec, NULL)) < 0)
		{
			LOGE("error in avcodec_open2");
			return -1;
		}
	}
	if (m_oAcc && (sample_fmt != m_oAcc->sample_fmt ||
		m_oAcc->channels != channle_count ||
		m_oAcc->sample_rate != sample_rate))
	{
		LOGE("swr initlize");
		swr_ctx = swr_alloc();
		if (!swr_ctx) {
			LOGE("Could not allocate resampler context\n");
			return -1;
		}

		/* set options */
		av_opt_set_int(swr_ctx, "in_channel_count", channle_count, 0);
		av_opt_set_int(swr_ctx, "in_sample_rate", sample_rate, 0);
		av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", sample_fmt, 0);
		av_opt_set_int(swr_ctx, "out_channel_count", m_oAcc->channels, 0);
		av_opt_set_int(swr_ctx, "out_sample_rate", m_oAcc->sample_rate, 0);
		av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", m_oAcc->sample_fmt, 0);
		if ((ret = swr_init(swr_ctx)) < 0) {
			LOGE("Failed to initialize the resampling context\n");
			return -1;
		}
	}
//	m_pVideoFrame = avcodec_alloc_frame();

	src_nb_samples = m_oAcc->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE ?
		10000 : m_oAcc->frame_size;
	max_dst_nb_samples = src_nb_samples;
	ret = av_samples_alloc_array_and_samples(&dst_samples_data, &dst_samples_linesize, m_oAcc->channels,
		max_dst_nb_samples, m_oAcc->sample_fmt, 0);
	dst_samples_size = av_samples_get_buffer_size(NULL, m_oAcc->channels, max_dst_nb_samples, m_oAcc->sample_fmt, 0);
	m_avFIFO = av_audio_fifo_alloc(m_oAcc->sample_fmt, m_oAcc->channels, m_oAcc->frame_size);
	m_pAudioFrame = av_frame_alloc();//avcodec_alloc_frame();
	return ret;
}

void CFrameRecorder::SetupAudioAligen(bool aligen )
{
	m_AudioAligen = aligen;
}

void CFrameRecorder::SetFillMode(EncodeFillMode mode,int pixel)
{
	m_FillMode = mode;
	m_FillPixels = min(pixel,m_CropWidth/2);
	if(m_FillPixels&1)
	{
		m_FillPixels--;
	}
}

int CFrameRecorder::Start()
{
	if (m_pOutputFormatContext == NULL)
	{
		return -1;
	}
	LOGE("in start");
	av_dump_format(m_pOutputFormatContext, 0, m_FilePath, 1);
	int ret;
	/* open the output file, if needed */
	if (!(m_pOutputFormatContext->oformat->flags & AVFMT_NOFILE)) {
		ret = avio_open(&m_pOutputFormatContext->pb,(const char*) m_FilePath, AVIO_FLAG_WRITE | AVIO_FLAG_READ);
		if (ret < 0) {
			LOGE("Could not open '%s': %s\n", (const char*)m_FilePath,
				makeErrorStr(ret));
			return ret;
		}
	}
	LOGE("avformat_write_header with faststart %s",m_FilePath);
	/*把信息写头部*/
	AVDictionary * dict = NULL;
	strcpy(m_pOutputFormatContext->filename, m_FilePath);
	av_dict_set(&dict, "movflags", "faststart", 0);
	if ((ret = avformat_write_header(m_pOutputFormatContext, &dict)) < 0)
	{
		LOGE("write header error.(%s)\n", makeErrorStr(ret));
		return ret;
	}

	startRecorderPrepare = 1;

	return ret;
}


int CFrameRecorder::RecordPCM(uint8 *&pData, int length) {
    return RecordPCM(&pData, length);
}

int CFrameRecorder::RecordPCM(unsigned char** pData, int length)
{
//#ifndef WIN32
	pthread_mutex_lock(&m_mutex);
//#endif
	if (m_pDstAudioStream == NULL)
	{
		LOGE("No open audio stream!\n");
//#ifndef WIN32
		pthread_mutex_unlock(&m_mutex);
//#endif // !WIN32
		return -1;
	}
	int ret;
	src_nb_samples = length;
	if (swr_ctx)
	{
		/*���²���*/
		int dst_nb_samples = (int)av_rescale_rnd((int64_t)(swr_get_delay(swr_ctx, src_sample_rate) + src_nb_samples),
			(int64_t)(m_oAcc->sample_rate), (int64_t)(src_sample_rate), AV_ROUND_UP);

		if (dst_nb_samples > max_dst_nb_samples)
		{
			av_freep(&dst_samples_data[0]);
			ret = av_samples_alloc(dst_samples_data, &dst_samples_linesize, m_oAcc->channels,
				dst_nb_samples, m_oAcc->sample_fmt, 1);
			if (ret < 0)
			{
				LOGE("error in resample audio!\n");
//#ifndef WIN32
				pthread_mutex_unlock(&m_mutex);
//#endif
				return -1;
			}
			max_dst_nb_samples = dst_nb_samples;
			dst_samples_size = av_samples_get_buffer_size(NULL, m_oAcc->channels, dst_nb_samples,
				m_oAcc->sample_fmt, 0);
		}

		ret = swr_convert(swr_ctx,
			dst_samples_data, dst_nb_samples,
			(const uint8_t **)pData, src_nb_samples);
		if (ret < 0)
		{
			LOGE("swr convert failed!\n");
//#ifndef WIN32
			pthread_mutex_unlock(&m_mutex);
//#endif
			return -1;
		}
		ret = av_audio_fifo_write(m_avFIFO, (void**)dst_samples_data, ret);
	}
	else
	{
		ret = av_audio_fifo_write(m_avFIFO, (void**)pData, length);
	}
	
    audio_pcm_length += ret;
	m_MaxAudioTimeStamp = (audio_pcm_length)*(1.0f / m_oAcc->sample_rate);
    //LOGE("JAVAN ----Recode audio length %ld", audio_pcm_length);

	if(m_oAcc && m_avFIFO)
		{
			//write all audio
			AVPacket pkt = { 0 };
			av_init_packet(&pkt);
			while (av_audio_fifo_size(m_avFIFO) >= m_oAcc->frame_size)
			{
				ret = av_audio_fifo_read(m_avFIFO, (void**)dst_samples_data, m_oAcc->frame_size);
				m_pAudioFrame->nb_samples = ret;
				int bufsize = av_samples_get_buffer_size(NULL, m_oAcc->channels, m_oAcc->frame_size,
					m_oAcc->sample_fmt, 0);

				ret = avcodec_fill_audio_frame(m_pAudioFrame, m_oAcc->channels, m_oAcc->sample_fmt, dst_samples_data[0], bufsize, 0);
				int got_packet = 0;
				ret = avcodec_encode_audio2(m_oAcc, &pkt, m_pAudioFrame, &got_packet);
				if (got_packet)
				{
					pkt.flags |= AV_PKT_FLAG_KEY;
					pkt.stream_index = m_pDstAudioStream->index;
					if (pkt.pts != AV_NOPTS_VALUE)
						pkt.pts = av_rescale_q(pkt.pts, m_oAcc->time_base, m_pDstAudioStream->time_base);
					if (pkt.dts != AV_NOPTS_VALUE)
						pkt.dts = av_rescale_q(pkt.dts, m_oAcc->time_base, m_pDstAudioStream->time_base);
					if (pkt.duration > 0)
						pkt.duration = (int)av_rescale_q(pkt.duration, m_oAcc->time_base, m_oAcc->time_base);
					ret = av_interleaved_write_frame(m_pOutputFormatContext, &pkt);
					if(ret < 0)
					{
						LOGE("error in Finish audio av_write_frame.(%s)\n", makeErrorStr(ret));
						av_packet_unref(&pkt);
					}
				}
				av_packet_unref(&pkt);
				av_init_packet(&pkt);
			}
			av_packet_unref(&pkt);
		}

//#ifndef WIN32
	pthread_mutex_unlock(&m_mutex);
//#endif
	return 1;
}

unsigned char* CFrameRecorder::CropI420(unsigned char* pY, int stride_y, unsigned char* pU, int stride_u,
											unsigned char* pV, int stride_v, int width, int height)
{
	if (pY == NULL || pV == NULL || pU == NULL || width <= 0 || height <= 0)
	{
		return NULL;
	}
	int src_stride_y = stride_y;
	int src_stride_u = stride_u;
	int src_stride_v = stride_v;

	unsigned char* dst = (unsigned char*)aligned_malloc(3 * m_CropWidth*m_CropHeight / 2, 64);
	unsigned char* src_y = pY + m_CropTop*src_stride_y + m_CropLeft;
	unsigned char* src_u = pU + (m_CropTop / 2)*src_stride_u + m_CropLeft / 2;
	unsigned char* src_v = pV + (m_CropTop / 2)*src_stride_v + m_CropLeft / 2;
	
	int dst_stride_y = m_CropWidth;
	int dst_stride_u = m_CropWidth / 2;
	int dst_stride_v = m_CropWidth / 2;

	unsigned char* dst_y = dst;
	unsigned char* dst_u = dst + m_CropWidth*m_CropHeight;
	unsigned char* dst_v = dst_u + m_CropWidth*m_CropHeight / 4;

	

	for (int i = 0; i < m_CropHeight; i++)
	{
		memcpy(dst_y, src_y, dst_stride_y);
		src_y += src_stride_y;
		dst_y += dst_stride_y;
		if (i & 1)
		{
			memcpy(dst_u, src_u, dst_stride_u);
			memcpy(dst_v, src_v, dst_stride_v); 
			dst_u += dst_stride_u;
			dst_v += dst_stride_v;
			src_u += src_stride_u;
			src_v += src_stride_v;
		}
	}

	return dst;
}

unsigned char* CFrameRecorder::CropYuv420sp(unsigned char* src, int width, int height)
{
	if (src == NULL || width <= 0 || height <= 0)
	{
		return NULL;
	}
	
	int src_stride_y = width;
	int src_stride_uv = width;
	unsigned char* src_y = src + m_CropTop*src_stride_y + m_CropLeft;
	unsigned char* src_uv = src + width*height + (m_CropTop / 2)*src_stride_uv + m_CropLeft;


	unsigned char* dst = (unsigned char*)aligned_malloc(3 * m_CropWidth*m_CropHeight / 2, 64);
	int dst_stride_y = m_CropWidth;

	unsigned char* dst_y = dst;
	unsigned char* dst_u = dst + m_CropWidth*m_CropHeight;
	unsigned char* dst_v = dst_u + m_CropWidth*m_CropHeight / 4;

	for (int i = 0; i < m_CropHeight; i++)
	{
		memcpy(dst_y, src_y, dst_stride_y);
		src_y += src_stride_y;
		dst_y += dst_stride_y;
		if (i & 1)
		{
			for (int j = 0; j < m_CropWidth; j += 2)
			{
				*dst_v++ = src_uv[j];
				*dst_u++ = src_uv[j + 1];
			}
			src_uv += src_stride_uv;
		}
	}
	return dst;

}


void CFrameRecorder::FillTopBottomColor(unsigned char* pI420,int width,int height)
{
	byte* pY = pI420;
	byte* pU = pI420 + width*height;
	byte* pV = pU + width*height/4;

	int fillLen = width*m_FillPixels;
	if(m_FillMode == FR_FILL_BLACK)
	{
		//顶部
		memset(pY,0,fillLen);
		//底部
		pY = pU - fillLen;
		memset(pY,0,fillLen);
	}
	else if(m_FillMode == FR_FILL_WHITE)
	{
		//顶部
		memset(pY,255,fillLen);
		//底部
		pY = pU - fillLen;
		memset(pY,255,fillLen);
	}
	fillLen = width*m_FillPixels/4;
	//顶部
	memset(pU,128,fillLen);
	pU = pV - fillLen;
	//底部
	memset(pU,128,fillLen);
	//顶部
	memset(pV,128,fillLen);
	//底部
	pV = pI420 + width*height*3/2 - fillLen;
	memset(pV,128,fillLen);

}


int CFrameRecorder::Record420SP(unsigned char* pYuv420sp, int width, int height, double time_stemp)
{
//#ifndef WIN32
	pthread_mutex_lock(&m_mutex);
//#endif // !WIN32
	if (pYuv420sp == NULL || m_oVcc == NULL || width <= 0 || height <= 0)
	{
//#ifndef WIN32
		pthread_mutex_unlock(&m_mutex);
//#endif
		return -1;
	}

	if(m_IsMaxFrameRate30)
	{
		double interval = time_stemp - m_LastTimeStamp;
		if( fabs(interval - m_FrameInterval) < m_FrameInterval*0.33f)
		{
			//允许的误差范围;
			time_stemp = m_LastTimeStamp + m_FrameInterval;
		}
		else if(interval < m_FrameInterval)
		{
			LOGE("too close between last time strmp : %f, now :%f", m_LastTimeStamp, time_stemp);
			pthread_mutex_unlock(&m_mutex);
			return 0;
		}
	}

	int got_packet = 0;
	int frame_index = (int)(time_stemp*m_FrameRate + 0.5);
	if (frame_index == m_LastFrameIndex)
	{
		LOGE("too close between last : %d, now :%d", m_LastFrameIndex, frame_index);
// #ifndef WIN32
		pthread_mutex_unlock(&m_mutex);
// #endif
		return 0;
	}
	m_LastFrameIndex = frame_index;
	m_LastTimeStamp = time_stemp;
	if(m_LastFrameData)
	{
		aligned_free(m_LastFrameData);
		m_LastFrameData = NULL;
	}
	m_LastFrameData = CropYuv420sp(pYuv420sp, width, height);
	unsigned char* pY=m_LastFrameData;
	unsigned char* pU=m_LastFrameData+m_CropWidth*m_CropHeight;
	unsigned char* pV=pU+m_CropWidth*m_CropHeight/4;

	if (m_rotateMode!=kRotateNone)
	{
		unsigned char* rotateData = (unsigned char*)aligned_malloc(3 * m_CropWidth*m_CropHeight / 2, 64);
		unsigned char* pDstY=rotateData;
		unsigned char* pDstU=rotateData+m_CropWidth*m_CropHeight;
		unsigned char* pDstV=pDstU+m_CropWidth*m_CropHeight/4;
		I420Rotate(pY,m_CropWidth,pU,m_CropWidth/2,pV,m_CropWidth/2,pDstY,m_CropWidth,pDstU,m_CropWidth/2,pDstV,m_CropWidth/2,m_CropWidth,m_CropHeight,m_rotateMode);
		if (m_LastFrameData != NULL)
			aligned_free(m_LastFrameData);
		m_LastFrameData = rotateData;

	}
	if(m_FillMode != FR_FILL_NONE)
	{
		this->FillTopBottomColor(m_LastFrameData,m_CropWidth,m_CropHeight);
	}
	//填充水印
	if(m_WatermarkType != WATERMARK_NONE)
	{
		m_WaterMark.Add_WaterMark_YUV420(m_WatermarkType,
			m_LastFrameData,m_CropWidth,
			m_LastFrameData + m_CropWidth*m_CropHeight,m_CropWidth/2,
			m_LastFrameData + m_CropWidth*m_CropHeight*5/4,m_CropWidth/2,
			m_CropWidth,m_CropHeight);
	}
	if(m_EndingWaterType != WATERMARK_NONE && m_VideoDuration > 1.0)
	{
		double startWork = m_VideoDuration - 1.0;
		if(time_stemp > startWork && time_stemp < m_VideoDuration + 0.2)
		{
			//有效区间
			double valid_time = min(time_stemp - startWork,1.0);
			int gaussValue = valid_time*100;
			m_EndingWatermark.SetWatermrkParam(gaussValue,gaussValue);
			m_EndingWatermark.Add_WaterMark_YUV420(m_EndingWaterType,
				m_LastFrameData,m_CropWidth,
				m_LastFrameData + m_CropWidth*m_CropHeight,m_CropWidth/2,
				m_LastFrameData + m_CropWidth*m_CropHeight*5/4,m_CropWidth/2,
				m_CropWidth,m_CropHeight);
		}
	}
    int ret = av_image_fill_arrays(m_pVideoFrame->data, m_pVideoFrame->linesize, m_LastFrameData, m_oVcc->pix_fmt, m_oVcc->width, m_oVcc->height, 1);

	if(ret < 0)
	{
		LOGE("error in av_image_fill_arrays (%s). ", makeErrorStr(ret));
		aligned_free(m_LastFrameData);
		m_LastFrameData = NULL;
//#ifndef WIN32
		pthread_mutex_unlock(&m_mutex);
//#endif
		return - 1;
	}

	AVPacket pkt = { 0 };
	av_init_packet(&pkt);
	m_pVideoFrame->pict_type = AV_PICTURE_TYPE_NONE;
	m_pVideoFrame->pts = frame_index * av_rescale_q(1, m_oVcc->time_base, m_pDstVideoStream->time_base);

	ret = avcodec_encode_video2(m_oVcc, &pkt, m_pVideoFrame, &got_packet);
	if (ret < 0)
	{
		LOGE("Record420SP error in encode vide2 (%s). time:%lf", makeErrorStr(ret),time_stemp);
		 av_packet_unref(&pkt);
		aligned_free(m_LastFrameData);
		m_LastFrameData = NULL;
//#ifndef WIN32
		pthread_mutex_unlock(&m_mutex);
//#endif
		return - 1;
	}

	if (!ret && got_packet && pkt.size)
	{
		pkt.stream_index = m_pDstVideoStream->index;
		ret = av_interleaved_write_frame(m_pOutputFormatContext, &pkt);
		if (ret < 0)
		{
			LOGE("Record420SP error in av_write_frame (%s). time:%lf", makeErrorStr(ret),time_stemp);
            
            av_packet_unref(&pkt);
			aligned_free(m_LastFrameData);
			m_LastFrameData = NULL;
//#ifndef WIN32
			pthread_mutex_unlock(&m_mutex);
//#endif
			return -1;
		}
	}
	av_packet_unref(&pkt);
	m_MaxVideoTimeStamp = time_stemp;
//#ifndef WIN32
	pthread_mutex_unlock(&m_mutex);
//#endif
	return 1;
}

#define FRAME_RATE 30 // 期望30帧每秒

int CFrameRecorder::RecordARGB(unsigned char* pARGB,int width,int height, double time_stemp)
{
	if(pARGB == NULL || width <= 0 || height <= 0)
	{
		return -1;
	}

	int frame_index = (int)(time_stemp*m_FrameRate + 0.5);
	if (frame_index == m_LastFrameIndex)
	{
		LOGE("too close between last : %d, now :%d", m_LastFrameIndex, frame_index);
		return 0;
	}

    double timesample_inserted = .0;
    if (last_frame_timesample >= 0.0 && last_frame_data) {
        // 已经获取了一帧后，开始比对
        double tmp_timesample = last_frame_timesample;

        unsigned char* pI420 = (unsigned char*)malloc(width*height*3/2);
        unsigned char* pY = pI420;
        unsigned char* pU = pI420 + width*height;
        unsigned char* pV = pU + width*height/4;

        while((time_stemp - tmp_timesample) > 1.0/FRAME_RATE) {
            tmp_timesample += 1.0f/FRAME_RATE;
            if (tmp_timesample > time_stemp) {
                break;
            }


            ARGBToI420(last_frame_data, width*4,pY,width,pU,width/2,pV,width/2,width,height);
            RecordI420(pY,width,pU,width/2,pV,width/2,width,height,tmp_timesample);

            timesample_inserted = tmp_timesample;
            LOGE("Recorde %lf finished",tmp_timesample);
        }

        free(pI420);
    }

	unsigned char* pI420 = (unsigned char*)malloc(width*height*3/2);
	unsigned char* pY = pI420;
	unsigned char* pU = pI420 + width*height;
	unsigned char* pV = pU + width*height/4;

	ARGBToI420(pARGB, width*4,pY,width,pU,width/2,pV,width/2,width,height);

    int res = 0;
    if (timesample_inserted + 1.0f/FRAME_RATE <= time_stemp || last_frame_data == NULL) {
       res  = RecordI420(pY,width,pU,width/2,pV,width/2,width,height,time_stemp);
       last_frame_timesample = time_stemp;
    } else {
        last_frame_timesample = timesample_inserted;
    }
	
	free(pI420);
	//LOGE("Recorde %lf finished",time_stemp);

    if (last_frame_w != width || last_frame_h != height ) { // 删除数据
        free(last_frame_data);
        last_frame_data = NULL;
    }

    if (last_frame_data == NULL) { // 重新申请
        last_frame_data = (unsigned char*)malloc(width * height * sizeof(int32_t));
        last_frame_w = width;
        last_frame_h = height;
    } else {

    }

    memcpy(last_frame_data, pARGB, sizeof(int32_t) * width * height);

	return res;
}

int CFrameRecorder::RecordI420(unsigned char* pY, int stride_y, unsigned char* pU, int stride_u,
	unsigned char* pV, int stride_v, int width, int height, double time_stemp)
{
//#ifndef WIN32
	pthread_mutex_lock(&m_mutex);
//#endif
	if (pY == NULL || pU == NULL || pV == NULL)
	{
//#ifndef WIN32
		pthread_mutex_unlock(&m_mutex);
//#endif
		return -1;
	}

	if(m_IsMaxFrameRate30)
	{
		double interval = time_stemp - m_LastTimeStamp;
		if( fabs(interval - m_FrameInterval) < m_FrameInterval*0.33f)
		{
			//允许的误差范围;
			time_stemp = m_LastTimeStamp + m_FrameInterval;
		}
		else if(interval < m_FrameInterval)
		{
			LOGE("too close between last time strmp : %f, now :%f", m_LastTimeStamp, time_stemp);
			pthread_mutex_unlock(&m_mutex);
			return 0;
		}
	}

	int got_packet = 0;
	int frame_index = (int)(time_stemp*m_FrameRate + 0.5);
	if (frame_index == m_LastFrameIndex)
	{
		LOGE("too close between last : %d, now :%d", m_LastFrameIndex, frame_index);
// #ifndef WIN32
		pthread_mutex_unlock(&m_mutex);
// #endif
		return 0;
	}
	m_LastFrameIndex = frame_index;
	m_LastTimeStamp = time_stemp;
	if(m_LastFrameData)
	{
		aligned_free(m_LastFrameData);
		m_LastFrameData = NULL;
	}
	m_LastFrameData = CropI420(pY,stride_y,pU,stride_u,pV,stride_v, width, height);
	if(m_FillMode != FR_FILL_NONE)
	{
		this->FillTopBottomColor(m_LastFrameData,m_CropWidth,m_CropHeight);
	}
	//填充水印
	if(m_WatermarkType != WATERMARK_NONE)
	{
		m_WaterMark.Add_WaterMark_YUV420(m_WatermarkType,
			m_LastFrameData,m_CropWidth,
			m_LastFrameData + m_CropWidth*m_CropHeight,m_CropWidth/2,
			m_LastFrameData + m_CropWidth*m_CropHeight*5/4,m_CropWidth/2,
			m_CropWidth,m_CropHeight);
	}
	if(m_EndingWaterType != WATERMARK_NONE && m_VideoDuration > 1.0)
	{
		double startWork = m_VideoDuration - 1.0;
		if(time_stemp > startWork && time_stemp < m_VideoDuration + 0.2)
		{
			//有效区间
			double valid_time = min(time_stemp - startWork,1.0);
			int gaussValue = valid_time*100;
			m_EndingWatermark.SetWatermrkParam(gaussValue,gaussValue);
			m_EndingWatermark.Add_WaterMark_YUV420(m_EndingWaterType,
				m_LastFrameData,m_CropWidth,
				m_LastFrameData + m_CropWidth*m_CropHeight,m_CropWidth/2,
				m_LastFrameData + m_CropWidth*m_CropHeight*5/4,m_CropWidth/2,
				m_CropWidth,m_CropHeight);
		}
	}
    int ret = av_image_fill_arrays(m_pVideoFrame->data, m_pVideoFrame->linesize, m_LastFrameData, m_oVcc->pix_fmt, m_oVcc->width, m_oVcc->height, 1);

	if(ret < 0)
	{
		LOGE("error in av_image_fill_arrays (%s). time:%lf", makeErrorStr(ret),time_stemp);
		aligned_free(m_LastFrameData);
		m_LastFrameData = NULL;
		
//#ifndef WIN32
		pthread_mutex_unlock(&m_mutex);
//#endif
		return - 1;
	}

	AVPacket pkt = { 0 };
	av_init_packet(&pkt);
	m_pVideoFrame->pict_type = AV_PICTURE_TYPE_NONE;
	m_pVideoFrame->pts = frame_index * av_rescale_q(1, m_oVcc->time_base, m_pDstVideoStream->time_base);
	
	ret = avcodec_encode_video2(m_oVcc, &pkt, m_pVideoFrame, &got_packet);
	if (ret < 0)
	{
		LOGE("RecordI420 error in encode vide2 (%s). time:%lf", makeErrorStr(ret),time_stemp);
		 av_packet_unref(&pkt);
		aligned_free(m_LastFrameData);
		m_LastFrameData = NULL;
//#ifndef WIN32
		pthread_mutex_unlock(&m_mutex);
//#endif
		return -1;
	}
	
	if (!ret && got_packet && pkt.size)
	{
		pkt.stream_index = m_pDstVideoStream->index;
//		LOGE("start record %lf, packet size: %d ,pkt_pts:%lld",time_stemp,pkt.size,pkt.pts);
		ret = av_interleaved_write_frame(m_pOutputFormatContext, &pkt);
		if (ret < 0)
		{
			LOGE("RecordI420 error in av_write_frame (%s). time:%lf", makeErrorStr(ret),time_stemp);
            
            av_packet_unref(&pkt);
			aligned_free(m_LastFrameData);
			m_LastFrameData = NULL;
//#ifndef WIN32
			pthread_mutex_unlock(&m_mutex);
//#endif
			return -1;
		}
	}
    
    av_packet_unref(&pkt);
	m_MaxVideoTimeStamp = time_stemp;
//#ifndef WIN32
	pthread_mutex_unlock(&m_mutex);
//#endif
	return 1;
}


int CFrameRecorder::Finish()
{
	if (m_pOutputFormatContext == NULL)
	{
		LOGE("No initlized!\n");
		return -1;
	}
	int ret;
	LOGE("yzh start finished");
	//视频补帧 by yzh @151009
	if(m_IsAdjustDuration && m_VideoDuration - m_LastTimeStamp > 0.01 && m_oVcc && m_pVideoFrame
		&& m_LastFrameIndex >= 0 && m_LastFrameData)
	{

		do 
		{
			m_LastTimeStamp += 0.03;
			int frame_index = (int)(m_LastTimeStamp*m_FrameRate + 0.5);
			m_pVideoFrame->pict_type = AV_PICTURE_TYPE_NONE;
			m_pVideoFrame->pts = frame_index * av_rescale_q(1, m_oVcc->time_base, m_pDstVideoStream->time_base);
			AVPacket pkt = { 0 };
			av_init_packet(&pkt);
			int got_packet = 0;
			ret = avcodec_encode_video2(m_oVcc, &pkt, m_pVideoFrame, &got_packet);
			if (ret < 0)
			{
				LOGE("Finish error in encode vide2 (%s). time:%lf", makeErrorStr(ret),m_LastTimeStamp);
				av_packet_unref(&pkt);
				return -1;
			}

			if (!ret && got_packet && pkt.size)
			{
				pkt.stream_index = m_pDstVideoStream->index;
				ret = av_interleaved_write_frame(m_pOutputFormatContext, &pkt);
				if (ret < 0)
				{
					LOGE("Finish error in av_write_frame (%s). time:%lf", makeErrorStr(ret),m_LastTimeStamp);
					av_packet_unref(&pkt);
					return -1;
				}
			}
			av_packet_unref(&pkt);
			m_MaxVideoTimeStamp = m_LastTimeStamp;
		} while (m_VideoDuration > m_LastTimeStamp);

	}
	//flush video;
	if(m_oVcc)
	{
		AVPacket packet = {0};
		for (int got_packet = 1; got_packet;)
		{
			av_init_packet(&packet);
			if (avcodec_encode_video2(m_oVcc, &packet, NULL, &got_packet) >= 0)
			{
				if (got_packet)
				{
					packet.stream_index = m_pDstVideoStream->index;
					av_interleaved_write_frame(m_pOutputFormatContext, &packet);

				}
				else
				{
					av_packet_unref(&packet);
					break;
				}
			}
			av_packet_unref(&packet);
		}
	}

	if (m_AudioAligen && m_MaxVideoTimeStamp > m_MaxAudioTimeStamp && m_oAcc)
	{
		//还要写这么多数据
		int DataSize = (m_MaxVideoTimeStamp - m_MaxAudioTimeStamp + 0.01) * m_oAcc->sample_rate;
		int frame_size = m_oAcc->frame_size;
		int maxFillSize = av_samples_get_buffer_size(NULL, m_oAcc->channels, frame_size, m_oAcc->sample_fmt, 0);
		memset(dst_samples_data[0], 0, maxFillSize);
		while (DataSize > 0)
		{
			if (DataSize >= frame_size)
			{
				ret = av_audio_fifo_write(m_avFIFO, (void**)dst_samples_data, frame_size);
			}
			else
			{
				ret = av_audio_fifo_write(m_avFIFO, (void**)dst_samples_data, DataSize);
			}
			DataSize -= ret;
		}
	}

	if(m_oAcc && m_avFIFO)
	{
		//write all audio
		AVPacket pkt = { 0 };
		av_init_packet(&pkt);
		while (av_audio_fifo_size(m_avFIFO) >= m_oAcc->frame_size)
		{
			ret = av_audio_fifo_read(m_avFIFO, (void**)dst_samples_data, m_oAcc->frame_size);
			m_pAudioFrame->nb_samples = ret;
			int bufsize = av_samples_get_buffer_size(NULL, m_oAcc->channels, m_oAcc->frame_size,
				m_oAcc->sample_fmt, 0);

			ret = avcodec_fill_audio_frame(m_pAudioFrame, m_oAcc->channels, m_oAcc->sample_fmt, dst_samples_data[0], bufsize, 0);
			int got_packet = 0;
			ret = avcodec_encode_audio2(m_oAcc, &pkt, m_pAudioFrame, &got_packet);
			if (got_packet)
			{
				pkt.flags |= AV_PKT_FLAG_KEY;
				pkt.stream_index = m_pDstAudioStream->index;
				if (pkt.pts != AV_NOPTS_VALUE)
					pkt.pts = av_rescale_q(pkt.pts, m_oAcc->time_base, m_pDstAudioStream->time_base);
				if (pkt.dts != AV_NOPTS_VALUE)
					pkt.dts = av_rescale_q(pkt.dts, m_oAcc->time_base, m_pDstAudioStream->time_base);
				if (pkt.duration > 0)
					pkt.duration = (int)av_rescale_q(pkt.duration, m_oAcc->time_base, m_oAcc->time_base);
				ret = av_interleaved_write_frame(m_pOutputFormatContext, &pkt);
				if(ret < 0)
				{
					LOGE("error in Finish audio av_write_frame.(%s)\n", makeErrorStr(ret));
					av_packet_unref(&pkt);
				}
			}
			av_packet_unref(&pkt);
			av_init_packet(&pkt);
		}
		av_packet_unref(&pkt);
	}


	//flush audio;
	if(m_oAcc)
	{
		AVPacket packet = {0};
		int got_packet = 0;
		if(m_avFIFO)
		{
			av_init_packet(&packet);
			ret = av_audio_fifo_read(m_avFIFO, (void**)dst_samples_data, m_oAcc->frame_size);

			if(ret > 0)
			{
				m_pAudioFrame->nb_samples = m_oAcc->frame_size;

				memset(dst_samples_data[0]+ret*2,0,m_oAcc->frame_size*2-ret*2);
				LOGE("nb_samples= %d",ret);
				int bufsize = av_samples_get_buffer_size(NULL, m_oAcc->channels, m_pAudioFrame->nb_samples,
					m_oAcc->sample_fmt, 0);
				LOGE("buff size = %d",bufsize);
				ret = avcodec_fill_audio_frame(m_pAudioFrame, m_oAcc->channels, m_oAcc->sample_fmt, dst_samples_data[0], bufsize, 0);
				LOGE("ret = %d",ret);

				

				ret = avcodec_encode_audio2(m_oAcc, &packet, m_pAudioFrame, &got_packet);
				LOGE("got_packet = %d",got_packet);
				if (got_packet)
				{
					LOGE("flush data %d",m_pAudioFrame->nb_samples);
					packet.flags |= AV_PKT_FLAG_KEY;
					packet.stream_index = m_pDstAudioStream->index;
					if (packet.pts != AV_NOPTS_VALUE)
						packet.pts = av_rescale_q(packet.pts, m_oAcc->time_base, m_pDstAudioStream->time_base);
					if (packet.dts != AV_NOPTS_VALUE)
						packet.dts = av_rescale_q(packet.dts, m_oAcc->time_base, m_pDstAudioStream->time_base);
					if (packet.duration > 0)
						packet.duration = (int)av_rescale_q(packet.duration, m_oAcc->time_base, m_oAcc->time_base);
					ret = av_interleaved_write_frame(m_pOutputFormatContext, &packet);
					if(ret < 0)
					{
						av_packet_unref(&packet);
						LOGE("error flush audio in av_write_frame.(%s)\n", makeErrorStr(ret));
					}
				}	
			}
			av_packet_unref(&packet);
		}


		for (;;)
		{
			av_init_packet(&packet);
			ret = avcodec_encode_audio2(m_oAcc, &packet, NULL, &got_packet);
			if (ret < 0)
			{
				av_packet_unref(&packet);
				break;
			}
			if (got_packet)
			{
				packet.flags |= AV_PKT_FLAG_KEY;
				packet.stream_index = m_pDstAudioStream->index;
				if (packet.pts != AV_NOPTS_VALUE)
					packet.pts = av_rescale_q(packet.pts, m_oAcc->time_base, m_pDstAudioStream->time_base);
				if (packet.dts != AV_NOPTS_VALUE)
					packet.dts = av_rescale_q(packet.dts, m_oAcc->time_base, m_pDstAudioStream->time_base);
				if (packet.duration > 0)
					packet.duration = (int)av_rescale_q(packet.duration, m_oAcc->time_base, m_oAcc->time_base);
				ret = av_interleaved_write_frame(m_pOutputFormatContext, &packet);
				if (ret < 0)
				{
					av_packet_unref(&packet);
					LOGE("error flush audio in av_write_frame.(%s)\n", makeErrorStr(ret));
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


	ret = av_write_trailer(m_pOutputFormatContext);
	if (ret < 0)
	{
		LOGE("error in write trailer.(%s)\n", makeErrorStr(ret));
		return ret;
	}
	LOGE("Frame Recorder Finished!");
	return 1;
}

int CFrameRecorder::Close()
{
	if (sws_ctx)
		sws_freeContext(sws_ctx);
	sws_ctx = NULL;

	if(m_oVcc)
	{
		avcodec_close(m_oVcc);
		m_oVcc = NULL;
	}

	if(m_oAcc)
	{
		avcodec_close(m_oAcc);
		m_oAcc = NULL;
	}
	if (m_pOutputFormatContext)
    {
		avio_close(m_pOutputFormatContext->pb);
		avformat_free_context(m_pOutputFormatContext);
    }
	m_pDstAudioStream = NULL;
	m_pDstVideoStream = NULL;
    m_pOutputFormatContext = NULL;
	if(m_LastFrameData)
	{
		aligned_free(m_LastFrameData);
		m_LastFrameData = NULL;
	}
	if (m_pVideoFrame)
    {
		//avcodec_free_frame(&m_pVideoFrame);
        av_frame_free(&m_pVideoFrame);
    }
	m_pVideoFrame = NULL;

	if (m_pAudioFrame)
    {
        //avcodec_free_frame(&m_pAudioFrame);
        av_frame_free(&m_pAudioFrame);
    }		
	m_pAudioFrame = NULL;

	if (m_avFIFO)
		av_audio_fifo_free(m_avFIFO);
	m_avFIFO = NULL;

	if (swr_ctx)
		swr_free(&swr_ctx);
	swr_ctx = NULL;

	if (dst_samples_data)
	{
		av_freep(&dst_samples_data[0]);
		av_freep(&dst_samples_data);
	}
	dst_samples_data = NULL;
	m_LastFrameIndex = -1000000;
	startRecorderPrepare = 0;
	m_pOutputFormat = NULL;
	m_pDstVideoCodec = NULL;
	m_pDstAudioCodec = NULL;
	dst_samples_linesize = 0;
	src_nb_samples = 0;
	dst_samples_size = 0;
	max_dst_nb_samples = 0;
	m_FrameRate = 0;
	m_CropLeft = 0;
	m_CropTop = 0;
	m_CropRight = 0;
	m_CropBottom = 0;
	m_CropWidth = 0;
	m_CropHeight = 0;
	m_MaxAudioTimeStamp = 0.0;
	m_MaxVideoTimeStamp = 0.0;
	m_IsMaxFrameRate30 = false;
	m_rotateMode = kRotateNone;
	m_FillPixels = 0;
	m_VideoDuration = -1.0f;
	m_WatermarkType = WATERMARK_NONE;
	m_EndingWaterType = WATERMARK_NONE;
	m_FillMode = FR_FILL_NONE;
	m_IsAdjustDuration = false;
	return 1;
}

int CFrameRecorder::GetEncodeWidth()
{
	return m_CropWidth;
}

int CFrameRecorder::GetEncodeHeight()
{
	return m_CropHeight;
}

void CFrameRecorder::SetMaxFrameRate30( bool isMax30fps )
{
	m_IsMaxFrameRate30 = isMax30fps;
}

void CFrameRecorder::SetWatermark( const char* watermark_path,MeipaiWatermarkType type )
{
	if(m_WaterMark.LoadWatermark(watermark_path))
	{
		m_WatermarkType = type;
	}
	else
	{
		m_WatermarkType = WATERMARK_NONE;
	}
}

void CFrameRecorder::SetEndingWatermark( const char* watermark_path,MeipaiWatermarkType type ,double encode_duration)
{
	if(m_EndingWatermark.LoadWatermark(watermark_path))
	{
		m_EndingWaterType = type;
		m_VideoDuration = encode_duration;
	}
	else
	{
		m_EndingWaterType = WATERMARK_NONE;
		m_VideoDuration = -1.0;
	}
}

void CFrameRecorder::AdjustVideoDuration( bool isEnable,double encode_duration )
{
	m_IsAdjustDuration = isEnable;
	m_VideoDuration = encode_duration;
}

void CFrameRecorder::setIFrameInternal(float internal) {
    mIFrameInternal = internal;
}
