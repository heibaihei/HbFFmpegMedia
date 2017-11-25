//
//  CSVideoDecoder.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/1.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSVideoDecoder.h"

namespace HBMedia {

int CSVideoDecoder::S_MAX_BUFFER_CACHE = 8;
void* CSVideoDecoder::ThreadFunc_Video_Decoder(void *arg) {
    if (!arg) {
        LOGE("Video decoder args is invalid !");
        return nullptr;
    }
    int HBError = HB_OK;
    CSVideoDecoder* pDecoder = (CSVideoDecoder *)arg;
    AVPacket *pNewPacket = av_packet_alloc();
    AVFrame  *pNewFrame = av_frame_alloc();
    while (!(pDecoder->mDecodeStateFlag & DECODE_STATE_DECODE_END)) {
        
        if (!(pDecoder->mDecodeStateFlag & DECODE_STATE_READPKT_END)) {
            HBError = av_read_frame(pDecoder->mPInVideoFormatCtx, pNewPacket);
            if (HBError != 0) {
                if (HBError != AVERROR_EOF)
                    pDecoder->mDecodeStateFlag |= DECODE_STATE_READPKT_ABORT;
                pDecoder->mDecodeStateFlag |= DECODE_STATE_READPKT_END;
                av_packet_free(&pNewPacket);
                pNewPacket = nullptr;
                continue;
            }
            if (pNewPacket->stream_index == pDecoder->mVideoStreamIndex) {
                av_packet_unref(pNewPacket);
                continue;
            }
        }
        
        if (!pNewPacket) {
            HBError = avcodec_send_packet(pDecoder->mPInputVideoCodecCtx, pNewPacket);
            if (HBError != 0) {
                if (HBError != AVERROR(EAGAIN)) {
                    pDecoder->mDecodeStateFlag |= DECODE_STATE_DECODE_ABORT;
                    pDecoder->mDecodeStateFlag |= DECODE_STATE_DECODE_END;
                }
                continue;
            }
            av_packet_unref(pNewPacket);
        }
        else if (!(pDecoder->mDecodeStateFlag & DECODE_STATE_FLUSH_MODE)) {
            pDecoder->mDecodeStateFlag |= DECODE_STATE_FLUSH_MODE;
            avcodec_send_packet(pDecoder->mPInputVideoCodecCtx, nullptr);
        }
        
        while (true) {
            HBError = avcodec_receive_frame(pDecoder->mPInputVideoCodecCtx, pNewFrame);
            if (HBError != 0) {
                if (HBError != AVERROR(EAGAIN)) {
                    av_frame_unref(pNewFrame);
                    pDecoder->mDecodeStateFlag = DECODE_STATE_DECODE_ABORT;
                }
                if (pDecoder->mDecodeStateFlag & DECODE_STATE_FLUSH_MODE)
                    pDecoder->mDecodeStateFlag |= DECODE_STATE_DECODE_END;
                break;
            }
            
            /** 得到帧数据 */
            AVFrame *pTmpFrame = nullptr;
            if (pDecoder->mIsNeedTransfer) {
                /** 执行格式转换 */
                if (pDecoder->_DoSwscale(pNewFrame, &pTmpFrame) != HB_OK) {
                    av_frame_unref(pNewFrame);
                    continue;
                }
            }
            else {
                pTmpFrame = av_frame_clone(pNewFrame);
                if (!pTmpFrame) {
                    av_frame_unref(pNewFrame);
                    LOGE("Clone target frame failed !");
                    continue;
                }
                
            }
            av_frame_unref(pNewFrame);

            /** 执行 pTmpFrame 入队操作 */
            pDecoder->mFrameQueue->push(pTmpFrame);
        }
    }
    
    
    av_packet_free(&pNewPacket);
    av_frame_free(&pNewFrame);
    
    return arg;
}

CSVideoDecoder::CSVideoDecoder() {
    memset(&mDecodeStateFlag, 0x00, sizeof(mDecodeStateFlag));
    imageParamInit(&mSrcVideoParams);
    imageParamInit(&mTargetVideoParams);

    mIsNeedTransfer = false;
    mVideoStreamIndex = INVALID_STREAM_INDEX;
    mPInVideoFormatCtx = nullptr;

    mPInputVideoCodecCtx = nullptr;
    mPInputVideoCodec = nullptr;
    mPVideoConvertCtx = nullptr;
    mFrameQueue = new FiFoQueue<AVFrame *>(S_MAX_BUFFER_CACHE);
    mDecodeThreadCtx.setFunction(nullptr, nullptr);
}

CSVideoDecoder::~CSVideoDecoder() {

}

int CSVideoDecoder::prepare() {
    
    if (baseInitial() != HB_OK) {
        LOGE("Video base initial failed !");
        goto VIDEO_DECODER_PREPARE_END_LABEL;
    }
    
    if (_mediaParamInitial() != HB_OK) {
        LOGE("Check Video decoder param failed !");
        goto VIDEO_DECODER_PREPARE_END_LABEL;
    }

    if (_DecoderInitial() != HB_OK) {
        LOGE("Video decoder initial failed !");
        goto VIDEO_DECODER_PREPARE_END_LABEL;
    }
    
    if (_ExportInitial() != HB_OK) {
        LOGE("Video decoder open failed !");
        goto VIDEO_DECODER_PREPARE_END_LABEL;
    }
    
    if (_SwscaleInitial() != HB_OK) {
        LOGE("Video swscale initail failed !");
        goto VIDEO_DECODER_PREPARE_END_LABEL;
    }

    return HB_OK;
    
VIDEO_DECODER_PREPARE_END_LABEL:
    release();
    return HB_ERROR;
}
    
int CSVideoDecoder::start() {
    if (HB_OK != mDecodeThreadCtx.setFunction(ThreadFunc_Video_Decoder, this)) {
        LOGE("Initial decode thread context failed !");
        return HB_ERROR;
    }
    
    if (mDecodeThreadCtx.start() != HB_OK) {
        LOGE("Start decoder thread context failed !");
        return HB_ERROR;
    }
    
    return HB_OK;
}
    
int CSVideoDecoder::stop() {
    
    return HB_OK;
}

int CSVideoDecoder::syncWait() {
    mDecodeThreadCtx.join();
    return HB_OK;
}

int  CSVideoDecoder::_DoSwscale(AVFrame *pInFrame, AVFrame **pOutFrame) {
    if (!pOutFrame) {
        LOGE("Video decoder do swscale invalid args !");
        return HB_ERROR;
    }
    
    uint8_t *pTargetImageBuffer = nullptr;
    *pOutFrame = av_frame_alloc();
    if (!(*pOutFrame)) {
        LOGE("Alloc new output frame failed !");
        goto DO_SWSCALE_END_LABEL;
    }
    
    pTargetImageBuffer = (uint8_t *)av_mallocz(mTargetVideoParams.mPreImagePixBufferSize);
    if (!pTargetImageBuffer) {
        LOGE("malloc target image buffer failed !");
        goto DO_SWSCALE_END_LABEL;
    }
    
    av_image_fill_arrays((*pOutFrame)->data, (*pOutFrame)->linesize,\
                         pTargetImageBuffer, getImageInnerFormat(mTargetVideoParams.mPixFmt),\
                         mTargetVideoParams.mWidth, mTargetVideoParams.mHeight, mTargetVideoParams.mAlign);
    
    pTargetImageBuffer = nullptr;
    if (sws_scale(mPVideoConvertCtx, pInFrame->data, pInFrame->linesize,\
                  0, mSrcVideoParams.mHeight, (*pOutFrame)->data, (*pOutFrame)->linesize) <= 0) {
        LOGE("swscale to target frame format failed !");
        goto DO_SWSCALE_END_LABEL;
    }
    return HB_OK;
    
DO_SWSCALE_END_LABEL:
    if (*pOutFrame) {
        av_frame_free(pOutFrame);
        *pOutFrame = nullptr;
    }
    if (pTargetImageBuffer)
        av_free(pTargetImageBuffer);
    
    return HB_ERROR;
}
    
int  CSVideoDecoder::_DecoderInitial() {
    
    if (mInMediaType == MD_TYPE_COMPRESS) {
        int HBError = HB_OK;
        AVStream* pVideoStream = nullptr;
        mPInputVideoCodecCtx = nullptr;
        memset(&mDecodeStateFlag, 0x00, sizeof(mDecodeStateFlag));
        
        if (CSMediaBase::_InMediaInitial() != HB_OK) {
            LOGE("Media base in media initial failed !");
            goto VIDEO_DECODER_INITIAL_END_LABEL;
        }
        
        mVideoStreamIndex = av_find_best_stream(mPInVideoFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
        if (mVideoStreamIndex < 0) {
            LOGW("Video decoder counldn't find valid audio stream, %s", av_err2str(mVideoStreamIndex));
            goto VIDEO_DECODER_INITIAL_END_LABEL;
        }
        
        pVideoStream = mPInVideoFormatCtx->streams[mVideoStreamIndex];
        mPInputVideoCodec = avcodec_find_decoder(pVideoStream->codecpar->codec_id);
        if (!mPInputVideoCodec) {
            LOGE("Codec <%d> not found !", pVideoStream->codecpar->codec_id);
            goto VIDEO_DECODER_INITIAL_END_LABEL;
        }
        
        mPInputVideoCodecCtx = avcodec_alloc_context3(mPInputVideoCodec);
        if (!mPInputVideoCodecCtx) {
            LOGE("Codec ctx <%d> not found !", pVideoStream->codecpar->codec_id);
            goto VIDEO_DECODER_INITIAL_END_LABEL;
        }
        avcodec_parameters_to_context(mPInputVideoCodecCtx, pVideoStream->codecpar);
        
        HBError = avcodec_open2(mPInputVideoCodecCtx, mPInputVideoCodec, NULL);
        if (HBError != 0) {
            LOGE("Could not open input codec context. <%s>", av_err2str(HBError));
            goto VIDEO_DECODER_INITIAL_END_LABEL;
        }
        
        /** 初始化原参数信息 */
        mSrcVideoParams.mPreImagePixBufferSize = av_image_get_buffer_size(mPInputVideoCodecCtx->pix_fmt, \
                                          mPInputVideoCodecCtx->width, mPInputVideoCodecCtx->height, mSrcVideoParams.mAlign);
        mSrcVideoParams.mWidth = mPInputVideoCodecCtx->width;
        mSrcVideoParams.mHeight = mPInputVideoCodecCtx->height;
        mSrcVideoParams.mPixFmt = getImageExternFormat(mPInputVideoCodecCtx->pix_fmt);
        
        av_dump_format(mPInVideoFormatCtx, mVideoStreamIndex, mSrcMediaFile, false);
    }
    
    return HB_OK;
    
VIDEO_DECODER_INITIAL_END_LABEL:
    if (mPInputVideoCodec)
        avcodec_free_context(&mPInputVideoCodecCtx);
    mPInputVideoCodec = nullptr;
    CSMediaBase::release();
    return HB_ERROR;
}

int CSVideoDecoder::_SwscaleInitial() {
    if (mTargetVideoParams.mPixFmt == CS_PIX_FMT_NONE)
        mTargetVideoParams.mPixFmt = mSrcVideoParams.mPixFmt;
    if (mTargetVideoParams.mWidth == 0 || mTargetVideoParams.mHeight == 0) {
        mTargetVideoParams.mWidth = mSrcVideoParams.mWidth;
        mTargetVideoParams.mHeight = mSrcVideoParams.mHeight;
    }
    mTargetVideoParams.mPreImagePixBufferSize = av_image_get_buffer_size(getImageInnerFormat(mTargetVideoParams.mPixFmt),\
                                             mTargetVideoParams.mWidth, mTargetVideoParams.mHeight, mTargetVideoParams.mAlign);
    if (!mTargetVideoParams.mPreImagePixBufferSize) {
        LOGE("Get target image buffer size failed, size:%d !", mTargetVideoParams.mPreImagePixBufferSize);
        goto VIDEO_SWSCALE_INITIAL_END_LABEL;
    }

    mPVideoConvertCtx = nullptr;
    mIsNeedTransfer = false;
    if (mTargetVideoParams.mPixFmt != mSrcVideoParams.mPixFmt \
        || mTargetVideoParams.mWidth != mSrcVideoParams.mWidth \
        || mTargetVideoParams.mHeight != mSrcVideoParams.mHeight) {
        mIsNeedTransfer = true;
        
        mPVideoConvertCtx = sws_getContext(mSrcVideoParams.mWidth, mSrcVideoParams.mHeight, getImageInnerFormat(mSrcVideoParams.mPixFmt), \
                                           mTargetVideoParams.mWidth, mTargetVideoParams.mHeight, getImageInnerFormat(mTargetVideoParams.mPixFmt), \
                                           SWS_BICUBIC, NULL, NULL, NULL);
        if (!mPVideoConvertCtx) {
            LOGE("Create video sws context failed !");
            goto VIDEO_SWSCALE_INITIAL_END_LABEL;
        }
    }
    
    return HB_OK;
VIDEO_SWSCALE_INITIAL_END_LABEL:
    if (mPVideoConvertCtx) {
        sws_freeContext(mPVideoConvertCtx);
        mPVideoConvertCtx = nullptr;
    }
    
    return HB_ERROR;
}

int  CSVideoDecoder::_mediaParamInitial() {

    mIsNeedTransfer = false;

    switch (mInMediaType) {
        case MD_TYPE_UNKNOWN: {
            if (mSrcMediaFile)
                mInMediaType = MD_TYPE_COMPRESS;
            else {
                LOGE("[%s] >>> [Type:%s] Audio decoder input file is invalid !", __func__, getMediaDataTypeDescript(mInMediaType));
                return HB_ERROR;
            }
        }
            break;
        case MD_TYPE_COMPRESS:
            if (!mSrcMediaFile) {
                LOGE("[%s] >>> [Type:%s]Audio decoder input file is invalid !", __func__, getMediaDataTypeDescript(mInMediaType));
                return HB_ERROR;
            }
            break;
        default:
            LOGE("Unknown media type !");
            return HB_ERROR;
    }
    
    switch (mOutMediaType) {
        case MD_TYPE_UNKNOWN: {
            if (mTrgMediaFile)
                mOutMediaType = MD_TYPE_RAW_BY_FILE;
            else
                mOutMediaType = MD_TYPE_RAW_BY_MEMORY;
        }
            break;
        case MD_TYPE_RAW_BY_FILE:
            if (!mTrgMediaFile) {
                LOGE("[%s] >>> [Type:%s]Audio decoder output file is invalid !", __func__, getMediaDataTypeDescript(mOutMediaType));
                return HB_ERROR;
            }
            break;
        case MD_TYPE_COMPRESS:
            LOGE("Cur work mode is not support this output type !");
            return HB_ERROR;
            
        default:
            LOGE("Unknown media type !");
            return HB_ERROR;
    }
    
    return HB_OK;
}

int CSVideoDecoder::_ExportInitial() {
    switch (mOutMediaType) {
        case MD_TYPE_RAW_BY_FILE:
            {
                if (CSMediaBase::_OutMediaInitial() != HB_OK) {
                    LOGE("Base media initial output media initial failed !");
                    return HB_ERROR;
                }
            }
            break;
        default:
            LOGE("output media initial failed, cur media type:%s", getMediaDataTypeDescript(mOutMediaType));
            return HB_ERROR;
    }

    return HB_OK;
}

int CSVideoDecoder::release() {
    if (mPInputVideoCodec)
        avcodec_free_context(&mPInputVideoCodecCtx);
    mPInputVideoCodec = nullptr;
    if (mPVideoConvertCtx) {
        sws_freeContext(mPVideoConvertCtx);
        mPVideoConvertCtx = nullptr;
    }
    CSMediaBase::release();
    return HB_OK;
}

}
