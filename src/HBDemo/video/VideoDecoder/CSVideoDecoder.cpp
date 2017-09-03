//
//  CSVideoDecoder.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/1.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSVideoDecoder.h"

namespace HBMedia {

CSVideoDecoder::CSVideoDecoder() {
    mVideoOutputFile = nullptr;
    mVideoOutputFileHandle = nullptr;
    mVideoInputFile = nullptr;
    mVideoInputFileHandle = nullptr;
    memset(&mDecodeStateFlag, 0x00, sizeof(mDecodeStateFlag));
}

CSVideoDecoder::~CSVideoDecoder() {
    if (mVideoInputFile)
        av_freep(mVideoInputFile);
    if (mVideoOutputFile)
        av_freep(mVideoOutputFile);
}

int  CSVideoDecoder::videoDecoderInitial() {
    memset(&mDecodeStateFlag, 0x00, sizeof(mDecodeStateFlag));
    globalInitial();
    
    int HBError = -1;
    if (!mVideoInputFile) {
        LOGE("Audio decoder args file is invalid !");
        return HB_ERROR;
    }
    
    if (_checkVideoParamValid() != HB_OK) {
        LOGE("Audio decoder param failed !");
        return HB_ERROR;
    }
    
    mPInputVideoFormatCtx = avformat_alloc_context();
    HBError = avformat_open_input(&mPInputVideoFormatCtx, mVideoInputFile, NULL, NULL);
    if (HBError != 0) {
        LOGE("Audio decoder couldn't open input file. <%d> <%s>", HBError, av_err2str(HBError));
        return HB_ERROR;
    }
    
    HBError = avformat_find_stream_info(mPInputVideoFormatCtx, NULL);
    if (HBError < 0) {
        LOGE("Audio decoder couldn't find stream information. <%s>", av_err2str(HBError));
        return HB_ERROR;
    }
    
    mVideoStreamIndex = INVALID_STREAM_INDEX;
    for (int i=0; i<mPInputVideoFormatCtx->nb_streams; i++) {
        if (mPInputVideoFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            mVideoStreamIndex = i;
            break;
        }
    }
    if (mVideoStreamIndex == INVALID_STREAM_INDEX) {
        LOGW("Audio decoder counldn't find valid audio stream !");
        return HB_ERROR;
    }
    
    AVStream* pVideoStream = mPInputVideoFormatCtx->streams[mVideoStreamIndex];
    
    mPInputVideoCodec = avcodec_find_decoder(pVideoStream->codecpar->codec_id);
    if (!mPInputVideoCodec) {
        LOGE("Codec <%d> not found !", pVideoStream->codecpar->codec_id);
        return HB_ERROR;
    }
    
    mPInputVideoCodecCtx = avcodec_alloc_context3(mPInputVideoCodec);
    if (!mPInputVideoCodecCtx) {
        LOGE("Codec ctx <%d> not found !", pVideoStream->codecpar->codec_id);
        return HB_ERROR;
    }
    
    avcodec_parameters_to_context(mPInputVideoCodecCtx, pVideoStream->codecpar);
    
    /** 初始化视频转码器 */
    /** 初始化数据缓冲区： TODO: huangcl */
    
    /** 初始化包的缓冲队列 */
    packet_queue_init(&mPacketCacheList);
    packet_queue_start(&mPacketCacheList);
    
    /** 初始化音频数据缓冲管道 */
    /** 初始化数据缓冲区： TODO: huangcl */
    
    av_dump_format(mPInputVideoFormatCtx, mVideoStreamIndex, mVideoInputFile, false);
    return HB_OK;
}
    
int  CSVideoDecoder::videoDecoderOpen() {
    int HBError = -1;
    HBError = avcodec_open2(mPInputVideoCodecCtx, mPInputVideoCodec, NULL);
    if (HBError < 0) {
        LOGE("Could not open codec. <%s>", av_err2str(HBError));
        return HB_ERROR;
    }
    
    if (mVideoOutputFile) {
        mVideoOutputFileHandle = fopen(mVideoOutputFile, "wb");
        if (!mVideoOutputFileHandle) {
            LOGE("Audio decoder couldn't open output file.");
            return HB_ERROR;
        }
    }
    
    packet_queue_flush(&mPacketCacheList);
    packet_queue_put_flush_pkt(&mPacketCacheList);
    return HB_OK;
}

int  CSVideoDecoder::videoDecoderClose() {
    return HB_OK;
}
int  CSVideoDecoder::videoDecoderRelease() {
    return HB_OK;
}
    
int  CSVideoDecoder::readVideoPacket() {
    int HBError = -1;
    
    if ((mDecodeStateFlag & DECODE_STATE_READPKT_END) \
        || (mDecodeStateFlag & DECODE_STATE_READPKT_ABORT)) {
        LOGW("Video read packet finished !");
        return HB_ERROR;
    }
    
    AVPacket *pNewPacket = av_packet_alloc();
    while (true) {
        HBError = av_read_frame(mPInputVideoFormatCtx, pNewPacket);
        if (HBError == 0) {
            if (pNewPacket->stream_index == mVideoStreamIndex) {
                packet_queue_put(&mPacketCacheList, pNewPacket);
            }
            av_packet_unref(pNewPacket);
#if DECODE_MODE_SEPERATE_AUDIO_WITH_VIDEO
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
    
int  CSVideoDecoder::selectVideoFrame() {
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
            if (pNewPacket->stream_index != mVideoStreamIndex) {
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
#if DECODE_MODE_SEPERATE_AUDIO_WITH_VIDEO
                continue;
#else
                break;
#endif
            }
        }
        
        HBError = avcodec_send_packet(mPInputVideoCodecCtx, pNewPacket);
        if (HBError == 0)
        {
            AVFrame* pNewFrame = av_frame_alloc();
            while (true)
            {
                HBError = avcodec_receive_frame(mPInputVideoCodecCtx, pNewFrame);
                if (HBError == 0) {
                    /** TODO: huangcl 得到数据，则将数据存入到缓冲区中 */
//                    uint8_t* pAudioResampleBuffer = nullptr;
//                    int samplesOfConvert = mAudioResample->doResample(&pAudioResampleBuffer, pNewFrame->data, pNewFrame->nb_samples);
//                    if (samplesOfConvert > 0) /** 将转码后数据写入缓冲区 */
//                        CSIOPushDataBuffer(pAudioResampleBuffer, samplesOfConvert);
//                    if (pAudioResampleBuffer)
//                        av_freep(pAudioResampleBuffer);
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
    
int  CSVideoDecoder::CSIOPushDataBuffer(uint8_t* data, int samples) {
    return HB_OK;
}
int  CSVideoDecoder::_checkVideoParamValid() {
    return HB_OK;
}
    
void CSVideoDecoder::setInputVideoMediaFile(char *file) {
    if (mVideoInputFile)
        av_freep(mVideoInputFile);
    mVideoInputFile = av_strdup(file);
}
char *CSVideoDecoder::getInputVideoMediaFile() {
    return mVideoInputFile;
}
void CSVideoDecoder::setOutputVideoMediaFile(char *file) {
    if (mVideoOutputFile)
        av_freep(mVideoOutputFile);
    mVideoOutputFile = av_strdup(file);
}

char *CSVideoDecoder::getOutputVideoMediaFile() {
    return mVideoOutputFile;
}

}
