//
//  CSVideoDecoder.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/1.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSVideoDecoder.h"

namespace HBMedia {

CSVideoDecoder::CSVideoDecoder(ImageParams& params) {
    memset(&mDecodeStateFlag, 0x00, sizeof(mDecodeStateFlag));
    imageParamInit(&mSrcVideoParams);
    mTargetVideoParams = params;

    mVideoStreamIndex = INVALID_STREAM_INDEX;
    mPKTSerial = 0;
    mPInVideoFormatCtx = nullptr;

    mPInputVideoCodecCtx = nullptr;
    mPInputVideoCodec = nullptr;
    mPVideoConvertCtx = nullptr;

    mTargetVideoFrameBufferSize = 0;
    mTargetVideoFrameBuffer = nullptr;
}

CSVideoDecoder::~CSVideoDecoder() {

}

int CSVideoDecoder::prepare() {

    if (baseInitial() != HB_OK) {
        LOGE("Video base initial failed !");
        return HB_ERROR;
    }
    
    if (_checkVideoParamValid() != HB_OK) {
        LOGE("Check Video decoder param failed !");
        return HB_ERROR;
    }
    
    if (videoDecoderInitial() != HB_OK) {
        LOGE("Video decoder initial failed !");
        return HB_ERROR;
    }
    
    if (videoDecoderOpen() != HB_OK) {
        LOGE("Video decoder open failed !");
        return HB_ERROR;
    }
    
    if (videoSwscalePrepare() != HB_OK) {
        LOGE("Video swscale initail failed !");
        return HB_ERROR;
    }
    
    return HB_OK;
}
    
int CSVideoDecoder::stop() {
    if (videoDecoderClose() != HB_OK) {
        LOGE("Video decoder close failed !");
        return HB_ERROR;
    }
    
    if (videoDecoderRelease() !=HB_OK) {
        LOGE("Video decoder release failed !");
        return HB_ERROR;
    }
    
    return HB_OK;
}

int  CSVideoDecoder::videoDoSwscale(uint8_t** inData, int*inDataSize) {
    int pictureSrcDataLineSize[4] = {0, 0, 0, 0};
    int pictureDstDataLineSize[4] = {0, 0, 0, 0};
    uint8_t *pictureSrcData[4] = {NULL};
    uint8_t *pictureDstData[4] = {NULL};
    
    av_image_fill_arrays(pictureSrcData, pictureSrcDataLineSize, *inData, getImageInnerFormat(mSrcVideoParams.mPixFmt), mSrcVideoParams.mWidth, mSrcVideoParams.mHeight, mSrcVideoParams.mAlign);
    
    av_image_fill_arrays(pictureDstData, pictureDstDataLineSize, mTargetVideoFrameBuffer, getImageInnerFormat(mTargetVideoParams.mPixFmt), mTargetVideoParams.mWidth, mTargetVideoParams.mHeight, mTargetVideoParams.mAlign);
    
    if (sws_scale(mPVideoConvertCtx, (const uint8_t* const*)pictureSrcData, pictureSrcDataLineSize, 0, mSrcVideoParams.mHeight, pictureDstData, pictureDstDataLineSize) <= 0) {
        LOGE("Picture sws scale failed !");
        return HB_ERROR;
    }
    
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
        HBError = av_read_frame(mPInVideoFormatCtx, pNewPacket);
        if (HBError == 0) {
            if (pNewPacket->stream_index == mVideoStreamIndex) {
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
    
int  CSVideoDecoder::selectVideoFrame() {
    int HBError = -1;
    AVPacket* pNewPacket = av_packet_alloc();
    
    while (true) {
        
        if ((mDecodeStateFlag & DECODE_STATE_FLUSH_MODE) \
            || (mDecodeStateFlag & DECODE_STATE_DECODE_END) \
            || (mDecodeStateFlag & DECODE_STATE_DECODE_ABORT)) {
            /** 音频解码模块状态检测 */
            break;
        }
        
        if (mPacketCacheList.nb_packets > 0) {
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
#if DECODE_WITH_MULTI_THREAD_MODE
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
                    
                    if (videoDoSwscale(pNewFrame->data, &(mSrcVideoParams.mPreImagePixBufferSize)) != HB_OK) {
                        LOGE("Video do swscale failed !");
                        av_frame_unref(pNewFrame);
                        return HB_ERROR;
                    }
                    else {
                        /** 此处得到转换后的视频数据：mTargetVideoFrameBuffer */
                        if (mTrgPicFileHandle) {
                            /** 说明以视频裸文件的数据方式输出 */
                        }
                        else {
                            /** 进行数据拷贝 */
                            AVFrame* pTmpFrame = av_frame_alloc();
                            uint8_t* pTargetData = (uint8_t *)av_mallocz(mTargetVideoFrameBufferSize);
                            memcpy(pTargetData, mTargetVideoFrameBuffer, mTargetVideoFrameBufferSize);
                            
                            av_image_fill_arrays(pTmpFrame->data, pTmpFrame->linesize, pTargetData, getImageInnerFormat(mTargetVideoParams.mPixFmt), mTargetVideoParams.mWidth, mTargetVideoParams.mHeight, mTargetVideoParams.mAlign);
                            pTmpFrame->width = mTargetVideoParams.mWidth;
                            pTmpFrame->height = mTargetVideoParams.mHeight;
                            pTmpFrame->format = mTargetVideoParams.mPixFmt;
                            
                            /** TODO: Huangcl 将帧插入帧缓冲区 */
                        }
                        av_frame_unref(pNewFrame);
                    }
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
    
int  CSVideoDecoder::videoDecoderInitial() {
    int HBError = -1;
    
    memset(&mDecodeStateFlag, 0x00, sizeof(mDecodeStateFlag));
    
    mPInVideoFormatCtx = avformat_alloc_context();
    HBError = avformat_open_input(&mPInVideoFormatCtx, mSrcPicMediaFile, NULL, NULL);
    if (HBError != 0) {
        LOGE("Video decoder couldn't open input file. <%d> <%s>", HBError, av_err2str(HBError));
        return HB_ERROR;
    }
    
    HBError = avformat_find_stream_info(mPInVideoFormatCtx, NULL);
    if (HBError < 0) {
        LOGE("Video decoder couldn't find stream information. <%s>", av_err2str(HBError));
        return HB_ERROR;
    }
    
    mVideoStreamIndex = INVALID_STREAM_INDEX;
    for (int i=0; i<mPInVideoFormatCtx->nb_streams; i++) {
        if (mPInVideoFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            mVideoStreamIndex = i;
            break;
        }
    }
    if (mVideoStreamIndex == INVALID_STREAM_INDEX) {
        LOGW("Video decoder counldn't find valid audio stream !");
        return HB_ERROR;
    }
    
    AVStream* pVideoStream = mPInVideoFormatCtx->streams[mVideoStreamIndex];
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
    
    /** 初始化包的缓冲队列 */
    packet_queue_init(&mPacketCacheList);
    
    /** 初始化音频数据缓冲管道 */
    packet_queue_init(&mFrameCacheList);
    
    av_dump_format(mPInVideoFormatCtx, mVideoStreamIndex, mSrcPicMediaFile, false);
    return HB_OK;
}

int  CSVideoDecoder::videoDecoderClose() {
    return HB_OK;
}

int  CSVideoDecoder::videoDecoderRelease() {
    return HB_OK;
}

int CSVideoDecoder::videoDecoderOpen() {
    int HBError = -1;
    
    packet_queue_start(&mPacketCacheList);
    packet_queue_start(&mFrameCacheList);
    
    HBError = avcodec_open2(mPInputVideoCodecCtx, mPInputVideoCodec, NULL);
    if (HBError < 0) {
        LOGE("Could not open codec. <%s>", av_err2str(HBError));
        return HB_ERROR;
    }
    
    packet_queue_flush(&mPacketCacheList);
    packet_queue_put_flush_pkt(&mPacketCacheList);
    packet_queue_flush(&mFrameCacheList);
    packet_queue_put_flush_pkt(&mFrameCacheList);
    
    /** 初始化原视频参数 : mSrcVideoParams */
    mSrcVideoParams.mPreImagePixBufferSize = av_image_get_buffer_size(getImageInnerFormat(mSrcVideoParams.mPixFmt), \
                                                         mSrcVideoParams.mWidth, mSrcVideoParams.mHeight, mSrcVideoParams.mAlign);
    return HB_OK;
}

int CSVideoDecoder::videoSwscalePrepare() {
    mPVideoConvertCtx = sws_getContext(mSrcVideoParams.mWidth, mSrcVideoParams.mHeight, getImageInnerFormat(mSrcVideoParams.mPixFmt), mTargetVideoParams.mWidth, mTargetVideoParams.mHeight, getImageInnerFormat(mTargetVideoParams.mPixFmt), SWS_BICUBIC, NULL, NULL, NULL);
    if (!mPVideoConvertCtx) {
        LOGE("Create video sws context failed !");
        return HB_ERROR;
    }
    
    mTargetVideoFrameBufferSize = av_image_get_buffer_size(getImageInnerFormat(mTargetVideoParams.mPixFmt), \
        mTargetVideoParams.mWidth, mTargetVideoParams.mHeight, mTargetVideoParams.mAlign);
    if (!mTargetVideoFrameBufferSize) {
        LOGE("Video get Sws target frame buffer size failed<%d> !", mTargetVideoFrameBufferSize);
        return HB_ERROR;
    }
    
    mTargetVideoFrameBuffer = (uint8_t *)av_mallocz(mTargetVideoFrameBufferSize);
    if (!mTargetVideoFrameBuffer) {
        LOGE("Video get Sws target frame buffer failed !");
        return HB_ERROR;
    }
    
    return HB_OK;
}

int  CSVideoDecoder::_checkVideoParamValid() {
    
    if (!mSrcPicMediaFile) {
        LOGE("Audio decoder input file is invalid !");
        return HB_ERROR;
    }
    
    if (mTrgPicMediaFile) {
        mTrgPicFileHandle = fopen(mTrgPicMediaFile, "wb");
        if (!mTrgPicFileHandle) {
            LOGE("Audio decoder couldn't open output file.");
            return HB_ERROR;
        }
    }
    return HB_OK;
}

}
