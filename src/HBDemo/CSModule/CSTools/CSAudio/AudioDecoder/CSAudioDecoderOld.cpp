//
//  CSAudioDecoder.c
//  FFmpeg
//
//  Created by zj-db0519 on 2017/8/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#if 0
#include "CSAudioDecoder.h"

namespace HBMedia {

CSAudioDecoder::CSAudioDecoder(AudioParams* targetAudioParam)
{
    if (!targetAudioParam) {
        LOGE("Initial audio decoder param failed !");
        return;
    }

    mAudioStreamIndex = INVALID_STREAM_INDEX;
    mTargetAudioParams = *targetAudioParam;
    mInputAudioMediaFile = nullptr;
    mOutputAudioMediaFile = nullptr;
    mPInputAudioFormatCtx = nullptr;
    mPInputAudioCodecCtx = nullptr;
    mPInputAudioCodec = nullptr;
    mAudioOutputFileHandle = nullptr;
    mAudioFifo = nullptr;
    mAudioResample = nullptr;
    mDecodeStateFlag = 0x00;
    mPKTSerial = 0;
}

CSAudioDecoder::~CSAudioDecoder()
{
    if (mInputAudioMediaFile)
        av_freep(mInputAudioMediaFile);
    if (mOutputAudioMediaFile)
        av_freep(mOutputAudioMediaFile);
    if (mAudioOutputFileHandle) {
        fclose(mAudioOutputFileHandle);
        mAudioOutputFileHandle = nullptr;
    }
    if (mAudioFifo) {
        av_audio_fifo_free(mAudioFifo);
        mAudioFifo = nullptr;
    }
    if (mPInputAudioFormatCtx) {
        avformat_free_context(mPInputAudioFormatCtx);
        mPInputAudioFormatCtx = nullptr;
    }
    if (mAudioResample)
        SAFE_DELETE(mAudioResample);
}

int  CSAudioDecoder::audioDecoderInitial()
{
    mDecodeStateFlag = 0x00;
    globalInitial();
    
    int HBError = HB_ERROR;
    if (!mInputAudioMediaFile) {
        LOGE("Audio decoder args file is invalid !");
        return HB_ERROR;
    }
    
    if (_checkAudioParamValid() != HB_OK) {
        LOGE("Audio decoder param failed !");
        return HB_ERROR;
    }
    
    mPInputAudioFormatCtx = avformat_alloc_context();
    HBError = avformat_open_input(&mPInputAudioFormatCtx, mInputAudioMediaFile, NULL, NULL);
    if (HBError != 0) {
        LOGE("Audio decoder couldn't open input file. <%d> <%s>", HBError, av_err2str(HBError));
        return HB_ERROR;
    }
    
    HBError = avformat_find_stream_info(mPInputAudioFormatCtx, NULL);
    if (HBError < 0) {
        LOGE("Audio decoder couldn't find stream information. <%s>", av_err2str(HBError));
        return HB_ERROR;
    }
    
    mAudioStreamIndex = INVALID_STREAM_INDEX;
    for (int i=0; i<mPInputAudioFormatCtx->nb_streams; i++) {
        if (mPInputAudioFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            mAudioStreamIndex = i;
            break;
        }
    }
    if (mAudioStreamIndex == INVALID_STREAM_INDEX) {
        LOGW("Audio decoder counldn't find valid audio stream !");
        return HB_ERROR;
    }
    
    AVStream* pAudioStream = mPInputAudioFormatCtx->streams[mAudioStreamIndex];
    
    mPInputAudioCodec = avcodec_find_decoder(pAudioStream->codecpar->codec_id);
    if (!mPInputAudioCodec) {
        LOGE("Codec <%d> not found !", pAudioStream->codecpar->codec_id);
        return HB_ERROR;
    }
    
    mPInputAudioCodecCtx = avcodec_alloc_context3(mPInputAudioCodec);
    if (!mPInputAudioCodecCtx) {
        LOGE("Codec ctx <%d> not found !", pAudioStream->codecpar->codec_id);
        return HB_ERROR;
    }
    
    avcodec_parameters_to_context(mPInputAudioCodecCtx, pAudioStream->codecpar);
    
    /** 初始化音频转码器 */
    mAudioResample = new CSAudioResample(&mTargetAudioParams);
    mAudioResample->audioSetSourceParams(mPInputAudioCodecCtx->channels, mPInputAudioCodecCtx->sample_fmt, mPInputAudioCodecCtx->sample_rate);
    mAudioResample->audioSetDestParams(mTargetAudioParams.channels, getAudioInnerFormat(mTargetAudioParams.pri_sample_fmt), mTargetAudioParams.sample_rate);
    
    /** 初始化包的缓冲队列 */
    packet_queue_init(&mPacketCacheList);
    packet_queue_start(&mPacketCacheList);
    
    /** 初始化音频数据缓冲管道 */
    mAudioFifo = av_audio_fifo_alloc(getAudioInnerFormat(mTargetAudioParams.pri_sample_fmt), mTargetAudioParams.channels, mTargetAudioParams.frame_size);
    
    av_dump_format(mPInputAudioFormatCtx, mAudioStreamIndex, mInputAudioMediaFile, false);
    return HB_OK;
}

int  CSAudioDecoder::audioDecoderOpen()
{
    int HBError = HB_ERROR;
    HBError = avcodec_open2(mPInputAudioCodecCtx, mPInputAudioCodec, NULL);
    if (HBError < 0) {
        LOGE("Could not open codec. <%s>", av_err2str(HBError));
        return HB_ERROR;
    }
    
    if (mOutputAudioMediaFile) {
        mAudioOutputFileHandle = fopen(mOutputAudioMediaFile, "wb");
        if (!mAudioOutputFileHandle) {
            LOGE("Audio decoder couldn't open output file.");
            return HB_ERROR;
        }
    }
    
    mAudioResample->audioResampleOpen();
    
    packet_queue_flush(&mPacketCacheList);
    packet_queue_put_flush_pkt(&mPacketCacheList);
    return HB_OK;
}
    
int  CSAudioDecoder::audioDecoderClose()
{
    mAudioResample->audioResampleClose();
    
    return HB_OK;
}

int  CSAudioDecoder::audioDecoderRelease()
{
    return HB_OK;
}

int  CSAudioDecoder::readAudioPacket()
{
    int HBError = HB_ERROR;
    
    if ((mDecodeStateFlag & DECODE_STATE_READPKT_END) \
        || (mDecodeStateFlag & DECODE_STATE_READPKT_ABORT)) {
        LOGW("Audio read packet finished !");
        return HB_ERROR;
    }
    
    AVPacket *pNewPacket = av_packet_alloc();
    while (true) {
        HBError = av_read_frame(mPInputAudioFormatCtx, pNewPacket);
        if (HBError == 0) {
            if (pNewPacket->stream_index == mAudioStreamIndex) {
                packet_queue_put(&mPacketCacheList, pNewPacket);
            }
            av_packet_unref(pNewPacket);
#if DECODE_WITH_MULTI_THREAD_MODE
            goto READ_PKT_END_LABEL;
#endif
        }
        else {
            switch (HBError) {
                case AVERROR_EOF:
                    mDecodeStateFlag |= DECODE_STATE_READPKT_END;
                    break;
                    
                default:
                    mDecodeStateFlag |= DECODE_STATE_READPKT_ABORT;
                    break;
            }
            goto READ_PKT_END_LABEL;
        }
    }
    
READ_PKT_END_LABEL:
    av_packet_free(&pNewPacket);
    return HB_OK;
}

int  CSAudioDecoder::selectAudieoFrame()
{
    int HBError = -1;
    AVPacket* pNewPacket = av_packet_alloc();
    
    while (true) {
        
        if ((mDecodeStateFlag & DECODE_STATE_FLUSH_MODE) || (mDecodeStateFlag & DECODE_STATE_DECODE_END) \
            || (mDecodeStateFlag & DECODE_STATE_DECODE_ABORT)){
            /** 音频解码模块状态检测 */
            break;
        }
        
        if (mPacketCacheList.nb_packets > 0) {
            /** 队列中存在音频包，则取包 */
            packet_queue_get(&mPacketCacheList, pNewPacket, QUEUE_NOT_BLOCK, &mPKTSerial);
            if (pNewPacket->stream_index != mAudioStreamIndex) {
                av_packet_unref(pNewPacket);
                continue;
            }
        }
        else {
            /** 如果队列中没有包，则检查读包状态是否正常，并且进入排水模式 */
            if ((mDecodeStateFlag & DECODE_STATE_READPKT_END) \
                || (mDecodeStateFlag & DECODE_STATE_READPKT_ABORT)) {
                av_packet_free(&pNewPacket);
                pNewPacket = NULL;
                /** 设置标识解码器已经进入排水模式 */
                mDecodeStateFlag |= DECODE_STATE_FLUSH_MODE;
            }
            else {
#if DECODE_WITH_MULTI_THREAD_MODE
                continue;
#else
                break;
#endif
            }
        }
        
        HBError = avcodec_send_packet(mPInputAudioCodecCtx, pNewPacket);
        if (HBError == 0)
        {
            AVFrame* pNewFrame = av_frame_alloc();
            while (true)
            {
                HBError = avcodec_receive_frame(mPInputAudioCodecCtx, pNewFrame);
                if (HBError == 0) {
                    uint8_t* pAudioResampleBuffer = nullptr;
                    int samplesOfConvert = mAudioResample->doResample(&pAudioResampleBuffer, pNewFrame->data, pNewFrame->nb_samples);
                    if (samplesOfConvert > 0) /** 将转码后数据写入缓冲区 */
                        CSIOPushDataBuffer(pAudioResampleBuffer, samplesOfConvert);
                    if (pAudioResampleBuffer)
                        av_freep(pAudioResampleBuffer);
                }
                else if (HBError == AVERROR(EAGAIN))
                    break;
                else {
                    /** avcodec_receive_frame 过程底层发生异常 */
                    mDecodeStateFlag |= DECODE_STATE_DECODE_ABORT;
                }
                
                av_frame_unref(pNewFrame);
            }
        }
        else if (HBError != AVERROR(EAGAIN)) {
            /** avcodec_send_packet 过程底层发生异常 */
            mDecodeStateFlag |= DECODE_STATE_DECODE_ABORT;
        }
        
        av_packet_unref(pNewPacket);
    }
    
DECODE_END_LABEL:
    av_packet_free(&pNewPacket);
    return HB_OK;
}
    
int  CSAudioDecoder::CSIOPushDataBuffer(uint8_t* data, int samples)
{
    int ret = -1;
    if (!data || samples <= 0) {
        LOGE("CS push data buffer failed !");
        return HB_ERROR;
    }
    
    int audioFrameDataLineSize[AV_NUM_DATA_POINTERS] = {0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t *audioFrameData[AV_NUM_DATA_POINTERS] = {NULL};
    
    ret = av_samples_fill_arrays(audioFrameData, audioFrameDataLineSize, data, mTargetAudioParams.channels, samples, getAudioInnerFormat(mTargetAudioParams.pri_sample_fmt), 1);
    if (ret < 0) {
        LOGE("Audio samples fill arrays failed <%s>!", makeErrorStr(ret));
        return HB_ERROR;
    }
    ret = av_audio_fifo_write(mAudioFifo, (void **)audioFrameData, samples);
    if ((ret < 0) || (ret < samples)) {
        LOGE("Audio fifo write sample:%d failed !", samples);
        return HB_ERROR;
    }
    
//    if (mAudioOutputFileHandle) { /** 如果存在输出文件，则将pcm 数据输出到对应的文件中 */
//        unsigned int resampled_data_size = samplesConvert * mTargetAudioParams.channels * av_get_bytes_per_sample(mTargetAudioParams.sample_fmt);
//        fwrite(mTargetAudioSampleBuffer, 1, resampled_data_size, mAudioOutputFileHandle);
//    }
    
    return HB_OK;
}
    
int  CSAudioDecoder::_checkAudioParamValid()
{
    if (mTargetAudioParams.channels != av_get_channel_layout_nb_channels(mTargetAudioParams.channel_layout)) {
        LOGE("Check audio param failed !");
        return HB_ERROR;
    }
    return HB_OK;
}
    
void CSAudioDecoder::setInputAudioMediaFile(char *file)
{
    if (mInputAudioMediaFile)
        av_freep(mInputAudioMediaFile);
    av_strdup(mInputAudioMediaFile);
}

char *CSAudioDecoder::getInputAudioMediaFile()
{
    return mInputAudioMediaFile;
}

void CSAudioDecoder::setOutputAudioMediaFile(char *file)
{
    if (mOutputAudioMediaFile)
        av_freep(mOutputAudioMediaFile);
    av_strdup(mOutputAudioMediaFile);
}

char *CSAudioDecoder::getOutputAudioMediaFile()
{
    return mOutputAudioMediaFile;
}
    
}

#endif
