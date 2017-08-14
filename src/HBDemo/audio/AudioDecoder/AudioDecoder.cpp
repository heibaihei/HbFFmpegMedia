/**
 * ◊ÓºÚµ•µƒª˘”⁄FFmpegµƒ“Ù∆µΩ‚¬Î∆˜
 * Simplest FFmpeg Audio Decoder
 *
 * ¿◊œˆÊË Lei Xiaohua
 * leixiaohua1020@126.com
 * ÷–π˙¥´√Ω¥Û—ß/ ˝◊÷µÁ ”ºº ı
 * Communication University of China / Digital TV Technology
 * http://blog.csdn.net/leixiaohua1020
 *
 * ±æ≥Ã–Úø…“‘Ω´“Ù∆µ¬Î¡˜£®mp3£¨AACµ»£©Ω‚¬ÎŒ™PCM≤…—˘ ˝æ›°£
 *  «◊ÓºÚµ•µƒFFmpeg“Ù∆µΩ‚¬Î∑Ω√ÊµƒΩÃ≥Ã°£
 * Õ®π˝—ßœ∞±æ¿˝◊”ø…“‘¡ÀΩ‚FFmpegµƒΩ‚¬Î¡˜≥Ã°£
 *
 * This software decode audio streams (AAC,MP3 ...) to PCM data.
 * Suitable for beginner of FFmpeg.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define __STDC_CONSTANT_MACROS

#include "AudioDecoder.h"

/** 1 second of 48khz 32bit audio: 48000*4=192000 */
#define MAX_AUDIO_FRAME_SIZE 192000

static int _audioDecoderInitial()
{
    av_register_all();
    avformat_network_init();
    
    return HB_OK;
}

static int _audioCheckParamsIsValid(AudioParams &outputAudioParams) {

    if (outputAudioParams.channels != av_get_channel_layout_nb_channels(outputAudioParams.channel_layout)) {
        LOGE("Check audio param failed !");
        return HB_ERROR;
    }
    
    return HB_OK;
}

int HBAudioDecoder(char *strInputFileName, char*strOutputFileName, AudioDataType dataType, AudioParams &outputAudioParams)
{
    int hbError = 0;
	int	iAudioStream = -1;

    if (!strInputFileName || !strOutputFileName || dataType >= AUDIO_DATA_TYPE_OF_MAX || dataType <= AUDIO_DATA_TYPE_OF_UNKNOWN) {
        LOGE("Audio decoder args file is Null, invalid ! type:%d | InputFile: %s | OutputFile:%s ", dataType, strInputFileName, strOutputFileName);
        return HB_ERROR;
    }
    
    if (_audioCheckParamsIsValid(outputAudioParams)) {
        LOGF("Audio decoder params is invalid !");
        return HB_ERROR;
    }
    
	FILE *pAudioOutputFileHandle=fopen(strOutputFileName, "wb");
    if (!pAudioOutputFileHandle) {
        LOGE("Audio decoder couldn't open output file.");
        return HB_ERROR;
    }
	
    _audioDecoderInitial();
    
	AVFormatContext	*pInputAudioFormatCtx = avformat_alloc_context();
	if((hbError = avformat_open_input(&pInputAudioFormatCtx, strInputFileName, NULL, NULL)) != 0){
		LOGE("Audio decoder couldn't open input file. <%d> <%s>", hbError, av_err2str(hbError));
		return HB_ERROR;
	}
	if((hbError = avformat_find_stream_info(pInputAudioFormatCtx, NULL)) < 0){
		LOGE("Audio decoder couldn't find stream information. <%s>", av_err2str(hbError));
		return HB_ERROR;
	}

	av_dump_format(pInputAudioFormatCtx, 0, strInputFileName, false);

    
    for(int i=0; i<pInputAudioFormatCtx->nb_streams; i++) {
		if (pInputAudioFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			iAudioStream = i;
			break;
		}
    }
    
	if(iAudioStream == -1){
		LOGW("Audio decoder counldn't find valid audio stream !");
		return HB_ERROR;
	}

    AVCodecContext* pInputAudioCodecCtx = avcodec_alloc_context3(avcodec_find_decoder(pInputAudioFormatCtx->streams[iAudioStream]->codecpar->codec_id));
    avcodec_parameters_to_context(pInputAudioCodecCtx, pInputAudioFormatCtx->streams[iAudioStream]->codecpar);
	AVCodec* pInputAudioCodec=avcodec_find_decoder(pInputAudioCodecCtx->codec_id);
	if(pInputAudioCodec == NULL){
		LOGE("Codec <%d> not found !", pInputAudioCodecCtx->codec_id);
		return HB_ERROR;
	}

	if((hbError = avcodec_open2(pInputAudioCodecCtx, pInputAudioCodec,NULL))<0){
		LOGE("Could not open codec. <%s>", av_err2str(hbError));
		return HB_ERROR;
	}

	AVPacket* packet=(AVPacket *)av_malloc(sizeof(AVPacket));
    if (!packet) {
        LOGE("Create avpacket failed !");
        return HB_ERROR;
    }
    
    av_init_packet(packet);
    AVFrame* pFrame=av_frame_alloc();
    
    /** Swr Initial */
    struct SwrContext* audioConvertCtx = swr_alloc();
    audioConvertCtx=swr_alloc_set_opts(audioConvertCtx, outputAudioParams.channel_layout, outputAudioParams.sample_fmt, outputAudioParams.sample_rate,
                                       av_get_default_channel_layout(pInputAudioCodecCtx->channels), pInputAudioCodecCtx->sample_fmt, pInputAudioCodecCtx->sample_rate, 0, NULL);
    swr_init(audioConvertCtx);
    
    uint8_t* swrOutputAudioBuffer = NULL;
    unsigned int swrOutputAudioBufferSize = 0;
    while(av_read_frame(pInputAudioFormatCtx, packet) >= 0)
    {
        if(packet->stream_index == iAudioStream)
        {
            hbError = avcodec_send_packet(pInputAudioCodecCtx, packet);
            if (hbError < 0 && hbError != AVERROR(EAGAIN) && hbError != AVERROR_EOF) {
                LOGE("Avcodec send packet failed <%d> !\n", hbError);
                return HB_ERROR;
            }
            
            hbError = avcodec_receive_frame(pInputAudioCodecCtx, pFrame);
            switch (hbError) {
                case 0:
                    {
                        int outputAudioSamplePerChannle = ((pFrame->nb_samples * outputAudioParams.sample_rate) / pFrame->sample_rate + 256);
                        int outputAudioSampleSize = av_samples_get_buffer_size(NULL, outputAudioParams.channels, outputAudioSamplePerChannle, outputAudioParams.sample_fmt, 0);
                        if (outputAudioSampleSize < 0) {
                            LOGE("Calculate ouput sample size failed !");
                            return HB_ERROR;
                        }
                        
                        av_fast_malloc(&swrOutputAudioBuffer, &swrOutputAudioBufferSize, outputAudioSampleSize);
                        if (!swrOutputAudioBuffer)
                            return AVERROR(ENOMEM);
                        
                        
                        int audioSwrPerChannelSamples = swr_convert(audioConvertCtx, &swrOutputAudioBuffer, outputAudioSamplePerChannle, (const uint8_t **)pFrame->data, pFrame->nb_samples);
                        if (audioSwrPerChannelSamples < 0) {
                            LOGE("Swr convert audio sample failed !");
                            return HB_ERROR;
                        }
                        else {
                            /** 输出转换后的音频数据到输出文件中 */
                            unsigned int resampled_data_size = audioSwrPerChannelSamples * outputAudioParams.channels * av_get_bytes_per_sample(outputAudioParams.sample_fmt);
                            
                            fwrite(swrOutputAudioBuffer, 1, resampled_data_size, pAudioOutputFileHandle);
                        }
                        av_frame_unref(pFrame);
                    }
                    break;
            
                case AVERROR_EOF:
                    printf("the decoder has been fully flushed,\
                           and there will be no more output frames.\n");
                    break;
                    
                case AVERROR(EAGAIN):
                    printf("Resource temporarily unavailable\n");
                    break;
                    
                case AVERROR(EINVAL):
                    printf("Invalid argument\n");
                    break;
                default:
                    break;
            }
        }
        
        av_packet_unref(packet);
    }
    
    av_free(swrOutputAudioBuffer);
	fclose(pAudioOutputFileHandle);

	swr_free(&audioConvertCtx);
	avcodec_close(pInputAudioCodecCtx);
	avformat_close_input(&pInputAudioFormatCtx);

	return HB_OK;
}


