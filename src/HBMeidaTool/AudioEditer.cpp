#include "AudioEditer.h"
#include "Error.h"

#ifndef LOGE
#define LOGE printf
#endif
#ifndef makeErrorStr
static char errorStr[AV_ERROR_MAX_STRING_SIZE];
#define makeErrorStr(errorCode) av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, errorCode)
#endif

#ifndef SWR_CH_MAX
#define SWR_CH_MAX 32
#endif

#define IS_RESAMPLER 1


const enum AVSampleFormat dstFormat = AV_SAMPLE_FMT_S16;
const int dstChannel = 1;
const int dstSampleRate = 8000;


CAudioEditer::CAudioEditer()
{
	m_pSrcFrame = NULL;
	m_audio_stream_idx = -1;
    audioDecoder = NULL;
    resample = NULL;
    fileCtx = NULL;
}


CAudioEditer::~CAudioEditer()
{
	this->Close();
}

int CAudioEditer::Open(const char* file)
{
	int ret = -1;
	av_register_all();
	avcodec_register_all();
    
    if (fileCtx != NULL) {
        fileCtx->close();
        delete fileCtx;
        fileCtx = NULL;
    }
    
    fileCtx = new InFileContext();
    if (fileCtx == NULL) {
        av_log(NULL, AV_LOG_ERROR, "New file context error!\n");
        ret = AV_MALLOC_ERR;
        goto TAR_OUT;
    }
    ret = fileCtx->open(file);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Open file err!\n");
        goto TAR_OUT;
    }
    
    audioDecoder = fileCtx->getAudioDecoder();
    if (audioDecoder == NULL) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find audio decoder\n");
        ret = AV_DECODE_ERR;
        goto TAR_OUT;
	}
    
    
    channels = audioDecoder->getChannels();
    sampleRate = audioDecoder->getSampleRate();
    m_audio_stream_idx = fileCtx->getAudioIndex();
    duration = fileCtx->getDuration();
    
    m_pSrcFrame = av_frame_alloc();
    if (m_pSrcFrame == NULL) {
        ret = AV_MALLOC_ERR;
        goto TAR_OUT;
    }
    
TAR_OUT:
    
    return ret;
}


static bool checkNeedResampler(int64_t outChannelsLayout, int outFormat, int outSampleRate,
                               int64_t inChannelsLayout, int inFormat, int inSampleRate)
{
    if (outChannelsLayout == inChannelsLayout &&
        outFormat == inFormat &&
        outSampleRate && inSampleRate) {
        return false;
    }
    
    return true;
}

/*
 * @func 将音频文件里面的数据进行解码并拷贝到指定的buffer
 * @args audioData： 上层指定缓存，该缓存需要上层开辟传入 dataSize：缓存区的大小
 */
int CAudioEditer::CopyDecodeFrameDataFromFile(short *audioData, size_t dataSize)
{
    int ret = -1;
    uint8_t *p_data;
    int readSample = 0;
    uint8_t *outData = NULL;
    int64_t nPTS;
    int outCnt;
    int outSize;
    int outSample;
    int wantedSamples;
    bool needResample = true;
    int64_t decChannelLayout = 0;
    
    p_data = (uint8_t *)audioData;
    if (p_data == NULL || dataSize <= 0) {
        LOGE("Buffer pool is null");
        return -1;
    }

    AVPacket packet = { 0 };
    av_init_packet(&packet);

    while (true) {
        ret = fileCtx->readPacket(&packet);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Read exit [%s]\n", makeErrorStr(ret));
            break;
        }

        if (packet.stream_index == m_audio_stream_idx) {
            ret = audioDecoder->pushPacket(&packet);
            if (ret < 0) {
                break;
            }
            while (ret >= 0) {
                ret = audioDecoder->popFrame(m_pSrcFrame);
                if (ret < 0) {
                    break;
                }
                
                nPTS = av_frame_get_best_effort_timestamp(m_pSrcFrame);

                if (needResample) {
                    decChannelLayout = (m_pSrcFrame->channel_layout && av_frame_get_channels(m_pSrcFrame) == av_get_channel_layout_nb_channels(m_pSrcFrame->channel_layout)) ?
                    m_pSrcFrame->channel_layout : av_get_default_channel_layout(av_frame_get_channels(m_pSrcFrame));
                    
                    needResample = checkNeedResampler(av_get_default_channel_layout(dstChannel),
                                                      dstFormat, dstSampleRate, decChannelLayout, m_pSrcFrame->format, m_pSrcFrame->sample_rate);
                    av_log(NULL, AV_LOG_VERBOSE, "Audio need resampler %d\n", needResample);
                }
                
                if (needResample && resample == NULL) {
                    resample = new Resampler();
                    if (resample == NULL) {
                        ret = AV_MALLOC_ERR;
                        goto TAR_OUT;
                    }
                    

                    ret = resample->initResampler(av_get_default_channel_layout(dstChannel), dstFormat, dstSampleRate,
                                            decChannelLayout, m_pSrcFrame->format, m_pSrcFrame->sample_rate);
                    if (ret < 0) {
                        av_log(NULL, AV_LOG_ERROR, "Init resampler error!\n");
                        goto TAR_OUT;
                    }
                }
                
                if (resample) {
                    wantedSamples = m_pSrcFrame->nb_samples;
                    outCnt = wantedSamples * dstSampleRate / sampleRate + 256;
                    outSize = av_samples_get_buffer_size(NULL, dstChannel, outCnt, dstFormat, 0);
                    if (outSize < 0) {
                        av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size() failed\n");
                        goto TAR_OUT;
                    }

                    outSample = resample->convert(&outData, outCnt, (const uint8_t **)m_pSrcFrame->extended_data, m_pSrcFrame->nb_samples);
                    
                    outSize = outSample * 2;

                } else {
                    outSample = m_pSrcFrame->nb_samples;
                    outSize = m_pSrcFrame->linesize[0];
                    outData = m_pSrcFrame->extended_data[0];
                }

                readSample += outSample;
                
                if (readSample * sizeof(short) > dataSize) {
                    LOGE("Read frame size more than buffer size!\n");
                    ret = readSample - outSample;
                    goto TAR_OUT;
                }

                memcpy(p_data, outData, outSize);
                p_data += outSize;

                av_frame_unref(m_pSrcFrame);
            }
        }
        av_packet_unref(&packet);
    }

TAR_OUT:
    
    if (fileCtx) {
        fileCtx->close();
        delete fileCtx;
        fileCtx = NULL;
    }
    if (resample) {
        resample->deinitResampler();
        delete resample;
    }
    av_packet_unref(&packet);

    if (outData) {
        av_freep(&outData);
    }
    return readSample;
}

double CAudioEditer::GetAudioDuration()
{
	return duration;
}

int CAudioEditer::GetAudioChannels()
{
    if (IS_RESAMPLER) {
        return dstChannel;
    }
    return channels;
}

int CAudioEditer::GetAudioSampleRate()
{
    if (IS_RESAMPLER) {
        return dstSampleRate;
    }
    return sampleRate;
}


void CAudioEditer::Close()
{
	if (m_pSrcFrame)
	{
		av_frame_free(&m_pSrcFrame);
		m_pSrcFrame = NULL;
	}

	m_audio_stream_idx = -1;
}
