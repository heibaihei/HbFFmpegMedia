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

static int audioSwrConvertComponentInitial(struct SwrContext** pAudioConvertCtx, AVCodecContext *audioCodeCtx, AudioParams *targetAudioParams) {
    
    if (!pAudioConvertCtx || !targetAudioParams || !audioCodeCtx) {
        LOGE("Audio swreample convert args invalid !");
        return HB_ERROR;
    }
    
    struct SwrContext* audioConvertCtx = nullptr;
    if (targetAudioParams->channel_layout != audioCodeCtx->channel_layout \
        || targetAudioParams->channels != audioCodeCtx->channels \
        || targetAudioParams->sample_rate != audioCodeCtx->sample_rate \
        || targetAudioParams->sample_fmt != audioCodeCtx->sample_fmt) {
        audioConvertCtx = swr_alloc();
        audioConvertCtx = swr_alloc_set_opts(audioConvertCtx, targetAudioParams->channel_layout, targetAudioParams->sample_fmt, targetAudioParams->sample_rate,
                                           av_get_default_channel_layout(audioCodeCtx->channels), audioCodeCtx->sample_fmt, audioCodeCtx->sample_rate, 0, NULL);

        swr_init(audioConvertCtx);
    }
    *pAudioConvertCtx = audioConvertCtx;
    
    return HB_OK;
}

static int audioSwrConvertComponentClose(struct SwrContext** pAudioConvertCtx) {
    
    if (pAudioConvertCtx) {
        if (*pAudioConvertCtx)
            swr_free(pAudioConvertCtx);
        *pAudioConvertCtx = NULL;
    }
    return HB_OK;
}

static int audioDecoderComponentInitial(AudioParams *targetAudioParams, AVFormatContext	**inputAudioFormatCtx, AVCodecContext** inputAudioCodecCtx, AVCodec** inputAudioCodec, char *strInputFileName, char*strOutputFileName, int& streamIndex) {

    int hbError = 0;
    if (!strInputFileName || !strOutputFileName ) {
        LOGE("Audio decoder args file is Null, invalid !  InputFile: %s | OutputFile:%s ", strInputFileName, strOutputFileName);
        return HB_ERROR;
    }
    
    if (targetAudioParams && targetAudioParams->channels != av_get_channel_layout_nb_channels(targetAudioParams->channel_layout)) {
        LOGE("Check audio param failed !");
        return HB_ERROR;
    }
    
    AVFormatContext	*audioFormatCtx = avformat_alloc_context();
    if((hbError = avformat_open_input(&audioFormatCtx, strInputFileName, NULL, NULL)) != 0){
        LOGE("Audio decoder couldn't open input file. <%d> <%s>", hbError, av_err2str(hbError));
        return HB_ERROR;
    }
    if((hbError = avformat_find_stream_info(audioFormatCtx, NULL)) < 0){
        LOGE("Audio decoder couldn't find stream information. <%s>", av_err2str(hbError));
        return HB_ERROR;
    }
    
    streamIndex = -1;
    for(int i=0; i<audioFormatCtx->nb_streams; i++) {
        if (audioFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            streamIndex = i;
            break;
        }
    }
    
    if(streamIndex == -1){
        LOGW("Audio decoder counldn't find valid audio stream !");
        return HB_ERROR;
    }
    
    AVCodecContext* audioCodecCtx = avcodec_alloc_context3(avcodec_find_decoder(audioFormatCtx->streams[streamIndex]->codecpar->codec_id));
    avcodec_parameters_to_context(audioCodecCtx, audioFormatCtx->streams[streamIndex]->codecpar);
    
    AVCodec* audioCodec=avcodec_find_decoder(audioCodecCtx->codec_id);
    if(audioCodec == NULL){
        LOGE("Codec <%d> not found !", audioCodecCtx->codec_id);
        return HB_ERROR;
    }
    
    *inputAudioFormatCtx = audioFormatCtx;
    *inputAudioCodecCtx = audioCodecCtx;
    *inputAudioCodec = audioCodec;
    
    av_dump_format(audioFormatCtx, 0, strInputFileName, false);
    
    return HB_OK;
}

int HBAudioDecoder(char *strInputFileName, char*strOutputFileName, AudioDataType dataType, AudioParams *outputAudioParams)
{
    int hbError = 0;
	int	iAudioStreamIndex = -1;

    if (!strInputFileName || !strOutputFileName || dataType >= AUDIO_DATA_TYPE_OF_MAX || dataType <= AUDIO_DATA_TYPE_OF_UNKNOWN) {
        LOGE("Audio decoder args file is Null, invalid ! type:%d | InputFile: %s | OutputFile:%s ", dataType, strInputFileName, strOutputFileName);
        return HB_ERROR;
    }
    
    if (globalInitial() != HB_OK) {
        LOGE("Audio global initial failed !");
        return HB_ERROR;
    }
    
    AVFormatContext	*pInputAudioFormatCtx = nullptr;
    AVCodecContext  *pInputAudioCodecCtx = nullptr;
    AVCodec         *pInputAudioCodec = nullptr;
    if (audioDecoderComponentInitial(outputAudioParams, &pInputAudioFormatCtx, &pInputAudioCodecCtx, &pInputAudioCodec, strInputFileName, strOutputFileName, iAudioStreamIndex) != HB_OK) {
        LOGE("Audio decoder component initial failed !");
        return HB_ERROR;
    }
    
    struct SwrContext* audioConvertCtx = nullptr;
    if (audioSwrConvertComponentInitial(&audioConvertCtx, pInputAudioCodecCtx, outputAudioParams) != HB_OK) {
        LOGE("Audio swreample component initial failed !");
        return HB_ERROR;
    }
    
	if((hbError = avcodec_open2(pInputAudioCodecCtx, pInputAudioCodec,NULL))<0){
		LOGE("Could not open codec. <%s>", av_err2str(hbError));
		return HB_ERROR;
	}

    FILE *pAudioOutputFileHandle=fopen(strOutputFileName, "wb");
    if (!pAudioOutputFileHandle) {
        LOGE("Audio decoder couldn't open output file.");
        return HB_ERROR;
    }
    
    AVPacket* pPacket = av_packet_alloc();
    AVFrame* pFrame = av_frame_alloc();
    
    uint8_t* swrOutputAudioBuffer = NULL;
    unsigned int swrOutputAudioBufferSize = 0;
    while(av_read_frame(pInputAudioFormatCtx, pPacket) == 0)
    {
        if(pPacket->stream_index == iAudioStreamIndex)
        {
            hbError = avcodec_send_packet(pInputAudioCodecCtx, pPacket);
            if (hbError < 0 && hbError != AVERROR(EAGAIN) && hbError != AVERROR_EOF) {
                LOGE("Avcodec send packet failed <%d> !\n", hbError);
                goto DECODE_END_LABLE;
            }
            
            
            while (true) {/** 参照： AudioResampler::audioConvert */
                hbError = avcodec_receive_frame(pInputAudioCodecCtx, pFrame);
                if (hbError == 0) {
                    unsigned int iOutputAudioDataByteSizes = 0;
                    int audioSwrPerChannelSamples = pFrame->nb_samples;
                    AVFrame *pOutputFrame = nullptr;
                    if (audioConvertCtx) {
                        
                        /** 创建音频数据输出缓冲区 */
                        pOutputFrame = av_frame_alloc();
                        pOutputFrame->nb_samples = (int)av_rescale_rnd((pFrame->nb_samples + swr_get_delay(audioConvertCtx, outputAudioParams->sample_rate)), \
                                                           outputAudioParams->sample_rate, pFrame->sample_rate, AV_ROUND_UP);
                        hbError = av_samples_alloc(pOutputFrame->data, &(pOutputFrame->linesize[0]), \
                                           outputAudioParams->channels, pOutputFrame->nb_samples, outputAudioParams->sample_fmt, 1);
                        if (hbError < 0) {
                            LOGE("av samples alloc failed !");
                            return -1;
                        }
                        audioSwrPerChannelSamples = swr_convert(audioConvertCtx, \
                                                        pOutputFrame->data, pOutputFrame->nb_samples, (const uint8_t **)pFrame->data, pFrame->nb_samples);
                        iOutputAudioDataByteSizes = audioSwrPerChannelSamples * outputAudioParams->channels * av_get_bytes_per_sample(outputAudioParams->sample_fmt);
                    }
                    else {
                        pOutputFrame = pFrame;
                        iOutputAudioDataByteSizes = audioSwrPerChannelSamples * pOutputFrame->channels * av_get_bytes_per_sample((enum AVSampleFormat)pOutputFrame->format);
                    }
                    
                    if (audioSwrPerChannelSamples < 0) {
                        LOGE("Swr convert audio sample failed !");
                        return HB_ERROR;
                    }
                    else if (pOutputFrame) { /** 输出转换后的音频数据到输出文件中 */
                        fwrite(pOutputFrame->data[0], iOutputAudioDataByteSizes, 1, pAudioOutputFileHandle);
                    }
                    
                    if (pOutputFrame && (pOutputFrame != pFrame)) {
                        av_frame_unref(pOutputFrame);
                        av_frame_free(&pOutputFrame);
                    }
                    av_frame_unref(pFrame);
                }
                else if (hbError == AVERROR(EAGAIN))
                    break;
                else
                    goto DECODE_END_LABLE;
            }
        }
        
        av_packet_unref(pPacket);
    }
	
DECODE_END_LABLE:
    audioSwrConvertComponentClose(&audioConvertCtx);
    
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);
    
    if (pInputAudioCodecCtx && avcodec_is_open(pInputAudioCodecCtx)) {
        avcodec_close(pInputAudioCodecCtx);
        avcodec_free_context(&pInputAudioCodecCtx);
    }
    
    avformat_close_input(&pInputAudioFormatCtx);
    
    if (pAudioOutputFileHandle)
        fclose(pAudioOutputFileHandle);
    
    if (swrOutputAudioBuffer)
        av_free(swrOutputAudioBuffer);
    
	return HB_OK;
}


