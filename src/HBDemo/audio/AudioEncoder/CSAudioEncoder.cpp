//
//  CSAudioEncoder.c
//  FFmpeg
//
//  Created by zj-db0519 on 2017/8/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSAudioEncoder.h"

namespace HBMedia {
    
enum AVCodecID CSAudioEncoder::mTargetCodecID = AV_CODEC_ID_AAC;
    
CSAudioEncoder::CSAudioEncoder(AudioParams* targetAudioParam)
{
    if (!targetAudioParam) {
        LOGE("Initial audio decoder param failed !");
        return;
    }
    mTargetAudioParams = *targetAudioParam;
    mPOutputAudioFormatCtx = nullptr;
    mPOutputAudioStream = nullptr;
    mInputAudioMediaFileHandle = nullptr;
    mAudioFifo = nullptr;
}

CSAudioEncoder::~CSAudioEncoder()
{
    if (mPOutputAudioFormatCtx) {
        avformat_free_context(mPOutputAudioFormatCtx);
        mPOutputAudioFormatCtx = nullptr;
    }
}

int  CSAudioEncoder::audioEncoderInitial()
{
    audioGlobalInitial();
    mPOutputAudioFormatCtx = avformat_alloc_context();
    mPOutputAudioFormatCtx->oformat = av_guess_format(NULL, mOutputAudioMediaFile, NULL);
    if (avio_open(&mPOutputAudioFormatCtx->pb, mOutputAudioMediaFile, AVIO_FLAG_READ_WRITE) < 0) {
        LOGE("Failed to open output file: %s!\n", mOutputAudioMediaFile);
        return HB_ERROR;
    }
    
    mPOutputAudioStream = avformat_new_stream(mPOutputAudioFormatCtx, NULL);
    if (mPOutputAudioStream == NULL) {
        LOGE("Create media stream failed !\n");
        return HB_ERROR;
    }
    
    mPOutputAudioCodec = avcodec_find_encoder(mTargetCodecID);
    if (mPOutputAudioCodec == NULL) {
        LOGE("Can not find audio encoder! %d\n", mPOutputAudioCodec->id);
        return HB_ERROR;
    }
    
    mPOutputAudioCodecCtx = avcodec_alloc_context3(mPOutputAudioCodec);
    mPOutputAudioCodecCtx->codec_id = mPOutputAudioCodec->id;
    mPOutputAudioCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
    mPOutputAudioCodecCtx->sample_fmt = mTargetAudioParams.sample_fmt;
    mPOutputAudioCodecCtx->sample_rate = mTargetAudioParams.sample_rate;
    mPOutputAudioCodecCtx->channels = mTargetAudioParams.channels;
    mPOutputAudioCodecCtx->channel_layout = av_get_default_channel_layout(mPOutputAudioCodecCtx->channels);
    mPOutputAudioCodecCtx->bit_rate = mTargetAudioParams.mbitRate;
    
    avcodec_parameters_from_context(mPOutputAudioStream->codecpar, mPOutputAudioCodecCtx);
    
    av_dump_format(mPOutputAudioFormatCtx, 0, mOutputAudioMediaFile, 1);
    
    mEncodeStateFlag = 0x00;
    return HB_OK;
}

int  CSAudioEncoder::audioEncoderOpen()
{
    int HbError = -1;
    
    HbError = avcodec_open2(mPOutputAudioCodecCtx, mPOutputAudioCodec, NULL);
    if (HbError < 0) {
        LOGE("Failed to open encoder !%s\n", makeErrorStr(HbError));
        return HB_ERROR;
    }
    
    HbError = avformat_write_header(mPOutputAudioFormatCtx, NULL);
    if (HbError < 0) {
        LOGE("Avformat write header failed !");
        return HB_ERROR;
    }
    
    mLastAudioFramePts = 0;
    mAudioFifo = av_audio_fifo_alloc(mPOutputAudioCodecCtx->sample_fmt, mPOutputAudioCodecCtx->channels, mPOutputAudioCodecCtx->frame_size);
    mPerFrameBufferSizes = av_get_bytes_per_sample(mPOutputAudioCodecCtx->sample_fmt) *mPOutputAudioCodecCtx->frame_size * mPOutputAudioCodecCtx->channels;
    
    /** 临时测试代码，表示以本地文件的输出方式来获取数据 */
    mInputAudioMediaFileHandle = fopen(mInputAudioMediaFile, "rb");
    if(!mInputAudioMediaFileHandle) {
        LOGE("Open input audio file failed !");
        return HB_ERROR;
    }
    
    return HB_OK;
}

int CSAudioEncoder::getPcmData(uint8_t** pData, int* dataSizes) {
    
    if ((mEncodeStateFlag & ENCODE_STATE_READ_DATA_END) || !pData || dataSizes) {
        LOGE("Audio encoder get pcm data failed !");
        return HB_ERROR;
    }
    
    *pData = (uint8_t*) av_mallocz(mPerFrameBufferSizes);
    if (*pData) {
        LOGE("Audio encoder malloc audio buffer failed !");
        return HB_ERROR;
    }
    
    *dataSizes = (int)fread(*pData, 1, mPerFrameBufferSizes, mInputAudioMediaFileHandle);
    if (*dataSizes <= 0) {
        LOGF("Read audio media abort !\n");
        mEncodeStateFlag |= ENCODE_STATE_READ_DATA_END;
    }
    return HB_OK;
}

int CSAudioEncoder::pushPcmDataToAudioBuffer(uint8_t* pData, int dataSizes)
{
    int HbError = -1;
    int audioFrameDataLineSize[AV_NUM_DATA_POINTERS] = {0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t *audioFrameData[AV_NUM_DATA_POINTERS] = {NULL};
    
    int samplesPerChannel = dataSizes / (av_get_bytes_per_sample(mPOutputAudioCodecCtx->sample_fmt) * mPOutputAudioCodecCtx->channels);
    HbError = av_samples_fill_arrays(audioFrameData, audioFrameDataLineSize, pData, mPOutputAudioCodecCtx->channels, samplesPerChannel, mPOutputAudioCodecCtx->sample_fmt, 1);
    if (HbError < 0) {
        LOGE("Audio samples fill arrays failed <%s>!", makeErrorStr(HbError));
        return HB_ERROR;
    }
    
    HbError = av_audio_fifo_write(mAudioFifo, (void **)audioFrameData, samplesPerChannel);
    if ((HbError < 0) || (HbError < samplesPerChannel)) {
        LOGE("Audio fifo write sample:%d failed !", samplesPerChannel);
        return HB_ERROR;
    }
    
    return HbError;
}
    
int CSAudioEncoder::selectAudioFrame()
{
    int HbError = -1;
    AVFrame* pNewFrame = nullptr;
    AVPacket newPacket;
    av_init_packet(&newPacket);
    
    while (true) {
        if ((mEncodeStateFlag & ENCODE_STATE_FLUSH_MODE) || (mEncodeStateFlag & ENCODE_STATE_ENCODE_END) \
            || (mEncodeStateFlag & ENCODE_STATE_ENCODE_ABORT)){
            /** 音频解码模块状态检测 */
            break;
        }
        
        if ((av_audio_fifo_size(mAudioFifo) >= mPOutputAudioCodecCtx->frame_size) || ((mEncodeStateFlag & ENCODE_STATE_READ_DATA_END) && (av_audio_fifo_size(mAudioFifo) > 0))) {
            
            HbError = _initialOutputFrame(&pNewFrame, &mTargetAudioParams, mPOutputAudioCodecCtx->frame_size);
            if (HbError != HB_OK) {
                LOGE("initial output frame failed !");
                return HB_ERROR;
            }
            
            HbError = av_audio_fifo_read(mAudioFifo, (void **)pNewFrame->data, mPOutputAudioCodecCtx->frame_size);
            if (HbError < 0) {
                LOGE("Audio fifo read data failed !");
                return HB_ERROR;
            }
            
            mLastAudioFramePts += HbError;
            pNewFrame->pts = mLastAudioFramePts;
            
        }
        else if ((mEncodeStateFlag & ENCODE_STATE_READ_DATA_END) \
            || (mEncodeStateFlag & ENCODE_STATE_READ_DATA_ABORT)) {
            pNewFrame = nullptr;
            mEncodeStateFlag |= ENCODE_STATE_FLUSH_MODE;
        }
        else
            goto AUDIO_ENCODE_END_LABEL;
        
        HbError = avcodec_send_frame(mPOutputAudioCodecCtx, pNewFrame);
        if (HbError<0 && HbError != AVERROR(EAGAIN) && HbError != AVERROR_EOF) {
            av_frame_unref(pNewFrame);
            LOGE("Send packet failed: %s!\n", makeErrorStr(HbError));
            return HB_ERROR;
        }
        
        if (pNewFrame)
            av_frame_unref(pNewFrame);
        
        while (true) {
            HbError = avcodec_receive_packet(mPOutputAudioCodecCtx, &newPacket);
            if (HbError == 0) {
                av_packet_rescale_ts(&newPacket, mPOutputAudioCodecCtx->time_base, mPOutputAudioStream->time_base);
                
                newPacket.stream_index = mPOutputAudioStream->index;
                av_write_frame(mPOutputAudioFormatCtx, &newPacket);
                av_packet_unref(&newPacket);
            }
            else {
                if (HbError<0 && HbError!=AVERROR_EOF)
                    mEncodeStateFlag |= ENCODE_STATE_ENCODE_ABORT;
                else if (HbError == AVERROR_EOF && !pNewFrame)
                    mEncodeStateFlag |= ENCODE_STATE_ENCODE_END;
                break;
            }
        }
    }

AUDIO_ENCODE_END_LABEL:
    return HB_OK;
}
    
int  CSAudioEncoder::audioEncoderClose()
{
    if (0 != av_write_trailer(mPOutputAudioFormatCtx)) {
        LOGE("Audio write tailer failed !");
        return HB_ERROR;
    }
    
    if (mInputAudioMediaFileHandle) {
        fclose(mInputAudioMediaFileHandle);
        mInputAudioMediaFileHandle = nullptr;
    }
    if (avcodec_is_open(mPOutputAudioCodecCtx)) {
        avcodec_close(mPOutputAudioCodecCtx);
    }
    return HB_OK;
}

int  CSAudioEncoder::audioEncoderRelease()
{
    return HB_OK;
}
    
void CSAudioEncoder::setInputAudioMediaFile(char *file)
{
    if (mInputAudioMediaFile)
        av_freep(mInputAudioMediaFile);
    av_strdup(mInputAudioMediaFile);
}

char *CSAudioEncoder::getInputAudioMediaFile()
{
    return mInputAudioMediaFile;
}

void CSAudioEncoder::setOutputAudioMediaFile(char *file)
{
    if (mOutputAudioMediaFile)
        av_freep(mOutputAudioMediaFile);
    av_strdup(mOutputAudioMediaFile);
}

int CSAudioEncoder::_initialOutputFrame(AVFrame** frame, AudioParams *pAudioParam, int AudioSamples) {
    if (!frame)
        return HB_ERROR;
    
    AVFrame* newOutputFrame = *frame;
    if (!newOutputFrame) {
        newOutputFrame = av_frame_alloc();
        if (!newOutputFrame)
            return HB_ERROR;
    }
    
    newOutputFrame->nb_samples = AudioSamples;
    newOutputFrame->format = pAudioParam->sample_fmt;
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
    
char *CSAudioEncoder::getOutputAudioMediaFile()
{
    return mOutputAudioMediaFile;
}

}
