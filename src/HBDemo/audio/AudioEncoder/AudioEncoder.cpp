#include <stdio.h>

#include "HBAudio.h"

#define __STDC_CONSTANT_MACROS

static int _audioDecoderInitial()
{
    av_register_all();
    avformat_network_init();
    
    return HB_OK;
}

int audio_flush_encoder(AVFormatContext *fmt_ctx,unsigned int stream_index){
	int ret;
	int got_frame;
	AVPacket enc_pkt;
	if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities &
		CODEC_CAP_DELAY))
		return 0;
	while (1) {
		enc_pkt.data = NULL;
		enc_pkt.size = 0;
		av_init_packet(&enc_pkt);
		ret = avcodec_encode_audio2 (fmt_ctx->streams[stream_index]->codec, &enc_pkt,
			NULL, &got_frame);
		av_frame_free(NULL);
		if (ret < 0)
			break;
		if (!got_frame){
			ret=0;
			break;
		}
		printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n",enc_pkt.size);
		/* mux encoded frame */
		ret = av_write_frame(fmt_ctx, &enc_pkt);
		if (ret < 0)
			break;
	}
	return ret;
}

int HBAudioEncoder(char *strInputFileName, char*strOutputFileName, AudioDataType dataType, AudioParams &outputAudioParams)
{
	uint8_t* frame_buf;
	AVFrame* pFrame;
	AVPacket pkt;

	int got_frame=0;
	int ret=0;
	int size=0;

	int framenum=1000;                          //Audio frame number
    int i;

    _audioDecoderInitial();

    FILE * inputFileHandle= fopen(strInputFileName, "rb");
    
	AVFormatContext* pOutputFormatCtx = avformat_alloc_context();
	AVOutputFormat* pOutputFormat = av_guess_format(NULL, strOutputFileName, NULL);
	pOutputFormatCtx->oformat = pOutputFormat;

	if (avio_open(&pOutputFormatCtx->pb, strOutputFileName, AVIO_FLAG_READ_WRITE) < 0){
		LOGE("Failed to open output file!\n");
		return HB_ERROR;
	}

    AVCodecContext* pOutputCodecCtx = avcodec_alloc_context3(NULL);
	pOutputCodecCtx->codec_id = pOutputFormat->audio_codec;
	pOutputCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
	pOutputCodecCtx->sample_fmt = outputAudioParams.fmt;
	pOutputCodecCtx->sample_rate = outputAudioParams.freq;
	pOutputCodecCtx->channel_layout = outputAudioParams.channel_layout;
	pOutputCodecCtx->channels = av_get_channel_layout_nb_channels(pOutputCodecCtx->channel_layout);
	pOutputCodecCtx->bit_rate = outputAudioParams.mbitRate;

    AVStream* audioOutputStream = avformat_new_stream(pOutputFormatCtx, avcodec_find_encoder(pOutputCodecCtx->codec_id));
    if (audioOutputStream == NULL){
        return HB_ERROR;
    }
    
	av_dump_format(pOutputFormatCtx, 0, strOutputFileName, 1);

	AVCodec* pOutputCodec = avcodec_find_encoder(pOutputCodecCtx->codec_id);
	if (!pOutputCodec){
		LOGE("Can not find audio encoder! %d\n", pOutputCodecCtx->codec_id);
		return HB_ERROR;
	}
	if ((ret = avcodec_open2(pOutputCodecCtx, pOutputCodec, NULL)) < 0) {
		LOGE("Failed to open encoder !%s\n", makeErrorStr(ret));
		return HB_ERROR;
	}
    
	pFrame = av_frame_alloc();
	pFrame->nb_samples= pOutputCodecCtx->frame_size;
	pFrame->format= pOutputCodecCtx->sample_fmt;
	
	size = av_samples_get_buffer_size(NULL, pOutputCodecCtx->channels,pOutputCodecCtx->frame_size,pOutputCodecCtx->sample_fmt, 1);
	frame_buf = (uint8_t *)av_malloc(size);
	avcodec_fill_audio_frame(pFrame, pOutputCodecCtx->channels, pOutputCodecCtx->sample_fmt,(const uint8_t*)frame_buf, size, 1);
	
	//Write Header
	ret = avformat_write_header(pOutputFormatCtx,NULL);

	av_new_packet(&pkt,size);

	for (i=0; i<framenum; i++){
		//Read PCM
		if (fread(frame_buf, 1, size, inputFileHandle) <= 0){
			LOGE("Failed to read raw data! \n");
			return HB_ERROR;
		}else if(feof(inputFileHandle)){
			break;
		}
		pFrame->data[0] = frame_buf;  //PCM Data

		pFrame->pts=i*100;
		got_frame=0;
		//Encode
		ret = avcodec_encode_audio2(pOutputCodecCtx, &pkt,pFrame, &got_frame);
		if(ret < 0){
			LOGE("Failed to encode!\n");
			return HB_ERROR;
		}
		if (got_frame==1){
			LOGE("Succeed to encode 1 frame! \tsize:%5d\n",pkt.size);
			pkt.stream_index = audioOutputStream->index;
			ret = av_write_frame(pOutputFormatCtx, &pkt);
			av_free_packet(&pkt);
		}
	}
	
	//Flush Encoder
	ret = audio_flush_encoder(pOutputFormatCtx, 0);
	if (ret < 0) {
		LOGE("Flushing encoder failed\n");
		return HB_ERROR;
	}

	//Write Trailer
	av_write_trailer(pOutputFormatCtx);

	//Clean
	if (audioOutputStream){
		avcodec_close(pOutputCodecCtx);
		av_free(pFrame);
		av_free(frame_buf);
	}
	avio_close(pOutputFormatCtx->pb);
	avformat_free_context(pOutputFormatCtx);

	fclose(inputFileHandle);

	return HB_OK;
}


