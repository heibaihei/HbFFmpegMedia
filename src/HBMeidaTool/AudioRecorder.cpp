#include "AudioRecorder.h"

#include "LogHelper.h"


#ifndef makeErrorStr
static char errorStr[AV_ERROR_MAX_STRING_SIZE];
#define makeErrorStr(errorCode) av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, errorCode)
#endif


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

CAudioRecorder::CAudioRecorder()
{
	m_pOutputFormatContext = NULL;
	m_pOutputFormat = NULL;
	m_pDstAudioStream = NULL;
	m_oAcc = NULL;
	m_pDstAudioCodec = NULL;
	m_pAudioFrame = NULL;
	m_avFIFO = NULL;
	swr_ctx = NULL;
	dst_samples_data = NULL;
}


CAudioRecorder::~CAudioRecorder()
{
	Close();
}


int CAudioRecorder::Open(const char* dstFile)
{
	int ret = -1;
	av_register_all();
	avcodec_register_all();
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

    return 0;
}


int CAudioRecorder::SetupAudio(int channle_count, int sample_rate, AVSampleFormat sample_fmt)
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
	m_oAcc->bit_rate = 16000;
	m_oAcc->sample_rate = 16000;
	m_oAcc->channel_layout = AV_CH_LAYOUT_MONO;// select_channel_layout(m_pDstAudioCodec);//AV_CH_LAYOUT_STEREO;// 
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

int CAudioRecorder::Start()
{
	if (m_pOutputFormatContext == NULL)
	{
		return -1;
	}
	av_dump_format(m_pOutputFormatContext, 0, m_FilePath, 1);
	int ret;
	/* open the output file, if needed */
	if (!(m_pOutputFormatContext->oformat->flags & AVFMT_NOFILE)) {
		ret = avio_open(&m_pOutputFormatContext->pb, (const char*)m_FilePath, AVIO_FLAG_WRITE);
		if (ret < 0) {
			LOGE("Could not open '%s': %s\n", (const char*)m_FilePath,
				makeErrorStr(ret));
			return ret;
		}
	}
	/*д����ͷ*/

	if ((ret = avformat_write_header(m_pOutputFormatContext, NULL)) < 0)
	{
		LOGE("write header error.(%s)\n", makeErrorStr(ret));
		return ret;
	}
	return ret;
}

int CAudioRecorder::RecordPCM(unsigned char** pData, int length)
{
	if (m_pDstAudioStream == NULL)
	{
		LOGE("No open audio stream!\n");
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
			return -1;
		}
		ret = av_audio_fifo_write(m_avFIFO, (void**)dst_samples_data, ret);
	}
	else
	{
		ret = av_audio_fifo_write(m_avFIFO, (void**)pData, length);
	}


	if (m_oAcc && m_avFIFO)
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
				if (ret < 0)
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

	return 1;
}

int CAudioRecorder::Finish()
{
	int ret = -1;
	if (m_oAcc)
	{
		AVPacket packet = { 0 };
		int got_packet = 0;
		if (m_avFIFO)
		{
			av_init_packet(&packet);
			ret = av_audio_fifo_read(m_avFIFO, (void**)dst_samples_data, m_oAcc->frame_size);

			if (ret > 0)
			{
				m_pAudioFrame->nb_samples = m_oAcc->frame_size;

				memset(dst_samples_data[0] + ret * 2, 0, m_oAcc->frame_size * 2 - ret * 2);
				LOGE("nb_samples= %d", ret);
				int bufsize = av_samples_get_buffer_size(NULL, m_oAcc->channels, m_pAudioFrame->nb_samples,
					m_oAcc->sample_fmt, 0);
				LOGE("buff size = %d", bufsize);
				ret = avcodec_fill_audio_frame(m_pAudioFrame, m_oAcc->channels, m_oAcc->sample_fmt, dst_samples_data[0], bufsize, 0);
				LOGE("ret = %d", ret);



				ret = avcodec_encode_audio2(m_oAcc, &packet, m_pAudioFrame, &got_packet);
				LOGE("got_packet = %d", got_packet);
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
	return 1;
}

int CAudioRecorder::Close()
{

	if (m_oAcc)
	{
		avcodec_close(m_oAcc);
		m_oAcc = NULL;
	}
	if (m_pOutputFormatContext)
	{
		avformat_free_context(m_pOutputFormatContext);
		m_pOutputFormatContext = NULL;
	}
	m_pDstAudioStream = NULL;
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
	m_pOutputFormat = NULL;
	m_pDstAudioCodec = NULL;
	dst_samples_linesize = 0;
	src_nb_samples = 0;
	dst_samples_size = 0;
	max_dst_nb_samples = 0;
	return 1;
}