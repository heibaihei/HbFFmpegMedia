#include <stdio.h>

#include "HBAudio.h"

#define __STDC_CONSTANT_MACROS


#define TARGET_AUDIO_ENCODE_FORMAT  AV_CODEC_ID_AAC

int flushAudioDecodeCache(AVCodecContext* pOutputCodecCtx, AVStream* audioOutputStream, AVFormatContext* pOutputFormatCtx);

int initialOutputFrame(AVFrame** frame, AudioParams *pAudioParam, int AudioSamples) {
    if (!frame)
        return HB_ERROR;
    
    AVFrame* newOutputFrame = *frame;
    if (!newOutputFrame) {
        newOutputFrame = av_frame_alloc();
        if (!newOutputFrame)
            return HB_ERROR;
    }
    
    newOutputFrame->nb_samples = AudioSamples;
    newOutputFrame->format = getAudioInnerFormat(pAudioParam->pri_sample_fmt);
    newOutputFrame->sample_rate = pAudioParam->sample_rate;
    newOutputFrame->channels = pAudioParam->channels;
    newOutputFrame->channel_layout = (uint64_t)av_get_default_channel_layout(pAudioParam->channels);
    
    if ((av_frame_get_buffer(newOutputFrame, 1)) < 0) {
        LOGE("Get frame buffer error !");
        return HB_ERROR;
    }
    
    /** 将本地的对象指针传递出去 */
    *frame = newOutputFrame;
    
    return HB_OK;
}

int audioComponentInitial(AVFormatContext** pOutputFormatCtx, AVStream** pOutputStream, AVCodecContext** pOutputCodecCtx, AVCodec** pOutputCodec, AudioParams* targetAudioParam, char *strInputFileName, char *strOutputFileName)
{
    if (!strInputFileName || !pOutputFormatCtx || !pOutputCodecCtx || !pOutputCodec || !targetAudioParam) {
        LOGE("Input audio component is failed, file:%s", strInputFileName);
        return HB_ERROR;
    }
    
    /** media format */
    AVFormatContext* audioFmtCtx = avformat_alloc_context();
    audioFmtCtx->oformat = av_guess_format(NULL, strOutputFileName, NULL);
    if (avio_open(&audioFmtCtx->pb, strOutputFileName, AVIO_FLAG_READ_WRITE) < 0) {
        LOGE("Failed to open output file: %s!\n", strOutputFileName);
        return HB_ERROR;
    }
    
    /** media stream */
    AVStream* audioStream = avformat_new_stream(audioFmtCtx, NULL);
    if (audioStream == NULL) {
        LOGE("Create media stream failed !\n");
        return HB_ERROR;
    }
    
    AVCodec* audioCodec = avcodec_find_encoder(TARGET_AUDIO_ENCODE_FORMAT);
    if (audioCodec == NULL) {
        LOGE("Can not find audio encoder! %d\n", audioCodec->id);
        return HB_ERROR;
    }
    
    /** media codecCtx */
    AVCodecContext* audioCodecCtx = avcodec_alloc_context3(audioCodec);
    audioCodecCtx->codec_id = audioCodec->id;
    audioCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
    audioCodecCtx->sample_fmt = getAudioInnerFormat(targetAudioParam->pri_sample_fmt);
    audioCodecCtx->sample_rate = targetAudioParam->sample_rate;
    audioCodecCtx->channels = targetAudioParam->channels;
    audioCodecCtx->channel_layout = av_get_default_channel_layout(audioCodecCtx->channels);
    audioCodecCtx->bit_rate = targetAudioParam->mbitRate;
    
    /** 拷贝音频参数到 stream 中 */
    avcodec_parameters_from_context(audioStream->codecpar, audioCodecCtx);
    
    *pOutputFormatCtx = audioFmtCtx;
    *pOutputStream = audioStream;
    *pOutputCodecCtx = audioCodecCtx;
    *pOutputCodec = audioCodec;
    
    av_dump_format(*pOutputFormatCtx, 0, strOutputFileName, 1);
    
    return HB_OK;
}

int HBAudioEncoder(char *strInputFileName, char*strOutputFileName, AudioDataType dataType, AudioParams *outputAudioParams)
{
	int ret=0;
    AVPacket outputPacket;
    av_init_packet(&outputPacket);
    AVFrame* pOutputFrame = nullptr;
    AVFormatContext* pOutputFormatCtx = nullptr;
    AVStream* audioOutputStream = nullptr;
    AVCodecContext* pOutputCodecCtx = nullptr;
    AVCodec* pOutputCodec = nullptr;

    globalInitial();

    if (HB_OK != audioComponentInitial(&pOutputFormatCtx, &audioOutputStream, &pOutputCodecCtx, &pOutputCodec, outputAudioParams, strInputFileName, strOutputFileName)) {
        LOGE("Audio component initial failed !\n");
        return HB_ERROR;
    }
    
	if ((ret = avcodec_open2(pOutputCodecCtx, pOutputCodec, NULL)) < 0) {
		LOGE("Failed to open encoder !%s\n", makeErrorStr(ret));
		return HB_ERROR;
	}
    
	/** Write Header */
    if (avformat_write_header(pOutputFormatCtx,NULL) < 0) {
        LOGE("Avformat write header failed !");
        return HB_ERROR;
    }

    int wantOutputSamplePerChannelOfFrame = pOutputCodecCtx->frame_size;
    
    int outputFrameBufferSize = av_get_bytes_per_sample(pOutputCodecCtx->sample_fmt) * wantOutputSamplePerChannelOfFrame * pOutputCodecCtx->channels;
    
    uint8_t* outputFrameBuffer = (uint8_t *)av_malloc(outputFrameBufferSize);
    
    AVAudioFifo *audioFifo = av_audio_fifo_alloc(pOutputCodecCtx->sample_fmt, pOutputCodecCtx->channels, pOutputCodecCtx->frame_size);
    
    FILE * inputFileHandle = fopen(strInputFileName, "rb");
    
    int realAudioReadSize = 0;
    bool bAudioDataEof = false;
    while (!bAudioDataEof)
    {
        if (realAudioReadSize <= 0) {
            realAudioReadSize = (int)fread(outputFrameBuffer, 1, outputFrameBufferSize, inputFileHandle);
            if (realAudioReadSize <= 0) {
                LOGF("Read audio media abort !\n");
                bAudioDataEof = true;
            }
        }
        
        if (realAudioReadSize > 0) {
            int audioFrameDataLineSize[AV_NUM_DATA_POINTERS] = {0, 0, 0, 0, 0, 0, 0, 0};
            uint8_t *audioFrameData[AV_NUM_DATA_POINTERS] = {NULL};
            
            int realAudioReadSample = realAudioReadSize / (av_get_bytes_per_sample(pOutputCodecCtx->sample_fmt) * pOutputCodecCtx->channels);
            ret = av_samples_fill_arrays(audioFrameData, audioFrameDataLineSize, outputFrameBuffer, pOutputCodecCtx->channels, realAudioReadSample, pOutputCodecCtx->sample_fmt, 1);
            if (ret < 0) {
                LOGE("Audio samples fill arrays failed <%s>!", makeErrorStr(ret));
                return HB_ERROR;
            }
            
            ret =av_audio_fifo_write(audioFifo, (void **)audioFrameData, realAudioReadSample);
            if ((ret < 0) || (ret < realAudioReadSample)) {
                LOGE("Audio fifo write sample:%d failed !", realAudioReadSample);
                return HB_ERROR;
            }
            
            realAudioReadSize -= (ret * (av_get_bytes_per_sample(pOutputCodecCtx->sample_fmt) * pOutputCodecCtx->channels));
            while ((av_audio_fifo_size(audioFifo)) >= wantOutputSamplePerChannelOfFrame || (bAudioDataEof && av_audio_fifo_size(audioFifo) > 0))
            {
                static int64_t lastAudioFramePts = 0;
                ret = initialOutputFrame(&pOutputFrame, outputAudioParams, wantOutputSamplePerChannelOfFrame);
                if (ret != HB_OK) {
                    LOGE("initial output frame failed !");
                    return HB_ERROR;
                }
                
                ret = av_audio_fifo_read(audioFifo, (void **)pOutputFrame->data, wantOutputSamplePerChannelOfFrame);
                if (ret < 0) {
                    LOGE("Audio fifo read data failed !");
                    return HB_ERROR;
                }
                
                lastAudioFramePts += ret;
                pOutputFrame->pts = lastAudioFramePts;
                
                ret = avcodec_send_frame(pOutputCodecCtx, pOutputFrame);
                if (ret<0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                    av_frame_unref(pOutputFrame);
                    LOGE("Send packet failed: %s!\n", makeErrorStr(ret));
                    return HB_ERROR;
                }
                
                av_frame_unref(pOutputFrame);
                
                while (true) {
                    ret = avcodec_receive_packet(pOutputCodecCtx, &outputPacket);
                    if (ret == 0) {
                        av_packet_rescale_ts(&outputPacket, pOutputCodecCtx->time_base, audioOutputStream->time_base);
                        
                        outputPacket.stream_index = audioOutputStream->index;
                        av_write_frame(pOutputFormatCtx, &outputPacket);
                        av_packet_unref(&outputPacket);
                    }
                    else if (ret == AVERROR(EAGAIN))
                        break;
                    else if (ret<0 && ret!=AVERROR_EOF)
                        return HB_ERROR;
                }
            }
        }
    }

ENCODE_END_LABLE:
    flushAudioDecodeCache(pOutputCodecCtx, audioOutputStream, pOutputFormatCtx);
    
    if (0 != av_write_trailer(pOutputFormatCtx)) {
        LOGE("Audio write tailer failed !");
        return HB_ERROR;
    }

	if (audioOutputStream){
		avcodec_close(pOutputCodecCtx);
		av_free(pOutputFrame);
		av_free(outputFrameBuffer);
	}
    av_packet_unref(&outputPacket);
	avio_close(pOutputFormatCtx->pb);
	avformat_free_context(pOutputFormatCtx);

	fclose(inputFileHandle);

	return HB_OK;
}

int flushAudioDecodeCache(AVCodecContext* pOutputCodecCtx, AVStream* audioOutputStream, AVFormatContext* pOutputFormatCtx)
{
    /**
     *  结束编解码： 排水模式
     */
    if (avcodec_send_frame(pOutputCodecCtx, NULL) == 0)
    {
        AVPacket outputPacket;
        av_init_packet(&outputPacket);
        
        while (true) {
            int ret = avcodec_receive_packet(pOutputCodecCtx, &outputPacket);
            if (ret == 0) {
                outputPacket.stream_index = audioOutputStream->index;
                
                if (outputPacket.pts != AV_NOPTS_VALUE)
                    outputPacket.pts = av_rescale_q(outputPacket.pts, pOutputCodecCtx->time_base, audioOutputStream->time_base);
                if (outputPacket.dts != AV_NOPTS_VALUE)
                    outputPacket.dts = av_rescale_q(outputPacket.dts, pOutputCodecCtx->time_base, audioOutputStream->time_base);
                
                ret = av_write_frame(pOutputFormatCtx, &outputPacket);
                av_packet_unref(&outputPacket);
            }
            else if (ret == AVERROR_EOF) {
                av_packet_unref(&outputPacket);
                break;
            }
            else if (ret < 0) {
                LOGE("Flush audio decode failed <%d>!", ret);
                break;
            }
        }
    }
    else
        LOGE("Flush audio decode failed !");

    return HB_OK;
}
