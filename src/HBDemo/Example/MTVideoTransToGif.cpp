//
//  MTVideoChangeToGif.cpp
//  Sample
//
//  Created by zj-db0519 on 2017/11/2.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "MTVideoTransToGif.h"

namespace FormatConvert {

#define STATE_UNKNOWN            0X0001
#define STATE_INITIALED          0X0002
#define STATE_ABORT              0X0004
#define STATE_READ_END           0X0008
#define STATE_TRANS_END          0X0010
    
#define STATE_DECODE_END         0X0020
#define STATE_DECODE_FLUSHING    0X0040
#define STATE_DECODE_ABORT       0X0080
    
#define STATE_ENCODE_END         0X0100
#define STATE_ENCODE_FLUSHING    0X0200
#define STATE_ENCODE_ABORT       0X0400

MediaCoder* AllocMediaCoder() {
    MediaCoder *coder = (MediaCoder *)malloc(sizeof(MediaCoder));
    if (coder) {
        coder->mPVideoFormatCtx = nullptr;
        coder->mPVideoCodecCtx = nullptr;
        coder->mPVideoCodec = nullptr;
        coder->mVideoStreamIndex = INVALID_STREAM_INDEX;
    }
    return coder;
}

void FreeMediaCoder(MediaCoder* pCoder) {
/**  huangcl 待补充如何释放的操作 */
    
}

void ImageParamsInitial(ImageParams *pParams) {
    if (pParams) {
        pParams->mPixFmt = CS_PIX_FMT_YUV420P;
        pParams->mWidth = 0;
        pParams->mHeight = 0;
        pParams->mFormatType = nullptr;
        pParams->mAlign = 1;
        pParams->mDataSize = 0;
        pParams->mBitRate = 0;
        pParams->mRotate = 0;
        pParams->mFrameRate = 0.0;
    }
}

VideoFormatTranser::VideoFormatTranser() {
    mPMediaDecoder = AllocMediaCoder();
    mPMediaEncoder = AllocMediaCoder();
    mPVideoConvertCtx = nullptr;
    mInputMediaFile = nullptr;
    mOutputMediaFile = nullptr;
    ImageParamsInitial(&mInputImageParams);
    ImageParamsInitial(&mOutputImageParams);
    memset(&mState, 0x00, sizeof(mState));
    mState |= STATE_UNKNOWN;
}

int VideoFormatTranser::prepare() {
    av_register_all();
    avformat_network_init();
    
    if (_InputMediaInitial() != HB_OK) {
        LOGE("Input media initial failed !");
        goto PREPARE_END_LABEL;
    }
    
    /** TODO：暂时拷贝输入视频的参数作为输出视频的参数 */
    memcpy(&mOutputImageParams, &mInputImageParams, sizeof(ImageParams));
    
    if (_OutputMediaInitial() != HB_OK) {
        LOGE("Output media initial failed !");
        goto PREPARE_END_LABEL;
    }
    
    if (_SwsMediaInitial() != HB_OK) {
        LOGE("Media sws initial failed !");
        goto PREPARE_END_LABEL;
    }
    mState |= STATE_INITIALED;
    return HB_OK;
    
PREPARE_END_LABEL:
    mState |= STATE_ABORT;
    return HB_ERROR;
}

int VideoFormatTranser::doConvert() {
    int HBError = HB_ERROR;
    if (!(mState & STATE_INITIALED)) {
        LOGE("Video format transer has't initial, can't do format convert !");
        return HBError;
    }
    
    if ((HBError = avformat_write_header(mPMediaEncoder->mPVideoFormatCtx, NULL)) < 0) {
        LOGE("Avformat write header failed, %s!", makeErrorStr(HBError));
        return HBError;
    }
    
    bool bNeedTranscode = ((mPVideoConvertCtx || (mPMediaDecoder->mPVideoCodec->id != mPMediaEncoder->mPVideoCodec->id)) ? true : false);
    AVPacket *pNewPacket = av_packet_alloc();
    while (!((mState & STATE_ABORT) || (mState & STATE_TRANS_END))) {
        
        if (!(mState & STATE_READ_END)) {
            HBError = av_read_frame(mPMediaDecoder->mPVideoFormatCtx, pNewPacket);
            if (HBError < 0) {
                if (HBError == AVERROR_EOF) {
                    mState |= STATE_READ_END;
                    if (!bNeedTranscode)
                        mState |= STATE_TRANS_END;
                }
                else
                    mState |= STATE_ABORT;
                continue;
            }
            
            if (pNewPacket->stream_index != mPMediaDecoder->mVideoStreamIndex) {
                av_packet_unref(pNewPacket);
                continue;
            }
        }
        
        if (bNeedTranscode) {
            if ((HBError = _TransMedia(&pNewPacket)) != 0) {
                if (HBError == -2) {
                    mState |= STATE_ABORT;
                    LOGE("Trans video media packet occur exception !");
                }
                if (pNewPacket) {
                    av_packet_unref(pNewPacket);
                }
                continue;
            }
        }
        
        HBError = av_interleaved_write_frame(mPMediaEncoder->mPVideoFormatCtx, pNewPacket);
        if (HBError < 0) {
            LOGE("Video format transer write frame failed !");
            av_packet_free(&pNewPacket);
            mState |= STATE_ABORT;
            continue;
        }
        av_packet_unref(pNewPacket);
    }
    
    if ((HBError = av_write_trailer(mPMediaEncoder->mPVideoFormatCtx)) != 0) {
        LOGE("AVformat wirte tailer failed, %s ", makeErrorStr(HBError));
    }
    
    if (bNeedTranscode) {
        if (mPMediaDecoder->mPVideoCodecCtx) {
            if (avcodec_is_open(mPMediaDecoder->mPVideoCodecCtx))
                avcodec_close(mPMediaDecoder->mPVideoCodecCtx);
        }
        if (mPMediaEncoder->mPVideoCodecCtx) {
            if (avcodec_is_open(mPMediaEncoder->mPVideoCodecCtx))
                avcodec_close(mPMediaEncoder->mPVideoCodecCtx);
        }
    }
    
    avformat_close_input(&mPMediaEncoder->mPVideoFormatCtx);
    return HB_OK;
}

int VideoFormatTranser::release() {
    if (mPMediaDecoder->mPVideoCodecCtx) {
        if (avcodec_is_open(mPMediaDecoder->mPVideoCodecCtx))
            avcodec_close(mPMediaDecoder->mPVideoCodecCtx);
        avcodec_free_context(&(mPMediaDecoder->mPVideoCodecCtx));
        mPMediaDecoder->mPVideoCodecCtx = nullptr;
        mPMediaDecoder->mPVideoCodec = nullptr;
    }
    if (mPMediaEncoder->mPVideoCodecCtx) {
        if (avcodec_is_open(mPMediaEncoder->mPVideoCodecCtx))
            avcodec_close(mPMediaEncoder->mPVideoCodecCtx);
        avcodec_free_context(&(mPMediaEncoder->mPVideoCodecCtx));
        mPMediaEncoder->mPVideoCodecCtx = nullptr;
        mPMediaEncoder->mPVideoCodec = nullptr;
    }
    
    avformat_close_input(&(mPMediaEncoder->mPVideoFormatCtx));
    avformat_close_input(&(mPMediaDecoder->mPVideoFormatCtx));
    
    return HB_OK;
}

int VideoFormatTranser::_TransMedia(AVPacket** pInPacket) {
    int HBError = -1;
    if (!pInPacket || !(*pInPacket)) {
        LOGE("Video trans failed, input a invalid packet !");
        return HBError;
    }
    
    /** 丢入原始数据包 */
    if (!(mState & STATE_READ_END) && !(mState & STATE_DECODE_ABORT)) {
        HBError = avcodec_send_packet(mPMediaDecoder->mPVideoCodecCtx, *pInPacket);
        if (HBError != 0) {
            if (HBError != AVERROR(EAGAIN))
                mState |= STATE_DECODE_ABORT;
            return -1;
        }
    }
    else if (((mState & STATE_READ_END) || (mState & STATE_DECODE_ABORT)) \
             && !(mState & STATE_DECODE_FLUSHING)) {
        HBError = avcodec_send_packet(mPMediaDecoder->mPVideoCodecCtx, NULL);
        mState |= STATE_DECODE_FLUSHING;
    }
    
    AVFrame* pNewFrame = nullptr;
    if (!(mState & STATE_DECODE_END)) {
        /** 获取原始数据帧 */
        pNewFrame = av_frame_alloc();
        while (true) {
            HBError = avcodec_receive_frame(mPMediaDecoder->mPVideoCodecCtx, pNewFrame);
            if (HBError != 0) {
                if (HBError != AVERROR(EAGAIN))
                    mState |= STATE_DECODE_END;
                else if (mState & STATE_DECODE_FLUSHING)
                    mState |= STATE_DECODE_END;
                return -1;
            }
            break;
        }
    }
    
    /** 将解码数据进行图像转码 */
    if (pNewFrame && mPVideoConvertCtx) {
        if (_ImageConvert(&pNewFrame) != HB_OK) {
            LOGE("image convert failed !");
            av_frame_unref(pNewFrame);
            return -1;
        }
    }
    
    if (!(mState & STATE_DECODE_END) && !(mState & STATE_DECODE_ABORT)) {
        HBError = avcodec_send_frame(mPMediaEncoder->mPVideoCodecCtx, pNewFrame);
        if (HBError != 0) {
            if (HBError != AVERROR(EAGAIN))
                mState |= STATE_ENCODE_ABORT;
            av_frame_free(&pNewFrame);
            return -1;
        }
        av_frame_free(&pNewFrame);
    }
    else if (((mState & STATE_DECODE_END) || (mState & STATE_ENCODE_ABORT)) \
             && !(mState & STATE_ENCODE_FLUSHING)) {
        HBError = avcodec_send_frame(mPMediaEncoder->mPVideoCodecCtx, NULL);
        mState |= STATE_ENCODE_FLUSHING;
    }
    
    AVPacket *pNewPacket = av_packet_alloc();
    while (true) {
        HBError = avcodec_receive_packet(mPMediaEncoder->mPVideoCodecCtx, pNewPacket);
        if (HBError != 0) {
            if (HBError != AVERROR(EAGAIN)) {
                mState |= STATE_ENCODE_END;
                mState |= STATE_ABORT;
            }
            else if (mState & STATE_ENCODE_FLUSHING) {
                mState |= STATE_ENCODE_END;
                mState |= STATE_TRANS_END;
            }
            av_packet_free(&pNewPacket);
            return -1;
        }
        break;
    }
    
    /** 将外部传入的 pInPacket 内存清空，并将该指针指向新的 packet */
    av_packet_free(pInPacket);
    *pInPacket = pNewPacket;
    return 0;
}
    
int VideoFormatTranser::_ImageConvert(AVFrame** pInFrame) {
    int HBError = -1;
    if (!pInFrame || !(*pInFrame)) {
        LOGE("Video trans frame failed, input a invalid frame !");
        return HBError;
    }
    
    AVFrame *pNewFrame = av_frame_alloc();
    if (!pNewFrame) {
        LOGE("Image conver, alloc new frame room failed !");
        return HB_ERROR;
    }
    
    HBError = av_image_alloc(pNewFrame->data, pNewFrame->linesize, mOutputImageParams.mWidth, mOutputImageParams.mHeight, \
                  getImageInnerFormat(mOutputImageParams.mPixFmt), mOutputImageParams.mAlign);
    if (HBError < 0) {
        LOGE("Video format alloc new image room failed !");
        av_frame_free(&pNewFrame);
        return HB_ERROR;
    }
    
    if (sws_scale(mPVideoConvertCtx, (*pInFrame)->data, (*pInFrame)->linesize, 0, (*pInFrame)->height, \
                  pNewFrame->data, pNewFrame->linesize) <= 0) {
        av_frame_free(&pNewFrame);
        LOGE("Image convert sws failed !");
        return HB_ERROR;
    }
    
    av_frame_free(&pNewFrame);
    *pInFrame = pNewFrame;
    return HB_OK;
}

int VideoFormatTranser::_InputMediaInitial() {
    int HBError = avformat_open_input(&(mPMediaDecoder->mPVideoFormatCtx), mInputMediaFile, NULL, NULL);
    if (HBError != 0) {
        LOGE("Video decoder couldn't open input file <%s>", av_err2str(HBError));
        return HB_ERROR;
    }
    
    HBError = avformat_find_stream_info(mPMediaDecoder->mPVideoFormatCtx, NULL);
    if (HBError < 0) {
        LOGE("Video decoder couldn't find stream information. <%s>", av_err2str(HBError));
        return HB_ERROR;
    }
    
    AVFormatContext* pFormatCtx = mPMediaDecoder->mPVideoFormatCtx;
    mPMediaDecoder->mVideoStreamIndex = INVALID_STREAM_INDEX;
    for (int i=0; i<pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            mPMediaDecoder->mVideoStreamIndex = i;
            break;
        }
    }
    
    if (mPMediaDecoder->mVideoStreamIndex == INVALID_STREAM_INDEX) {
        LOGW("Video decoder counldn't find valid audio stream !");
        return HB_ERROR;
    }
    
    AVStream* pVideoStream = pFormatCtx->streams[mPMediaDecoder->mVideoStreamIndex];
    mPMediaDecoder->mPVideoCodec = avcodec_find_decoder(pVideoStream->codecpar->codec_id);
    if (!mPMediaDecoder->mPVideoCodec) {
        LOGE("Codec <%d> not found !", pVideoStream->codecpar->codec_id);
        return HB_ERROR;
    }
    
    mPMediaDecoder->mPVideoCodecCtx = avcodec_alloc_context3(mPMediaDecoder->mPVideoCodec);
    if (!mPMediaDecoder->mPVideoCodecCtx) {
        LOGE("Codec ctx <%d> not found !", pVideoStream->codecpar->codec_id);
        return HB_ERROR;
    }
    avcodec_parameters_to_context(mPMediaDecoder->mPVideoCodecCtx, pVideoStream->codecpar);
    
    
    AVDictionaryEntry *tag = NULL;
    tag = av_dict_get(pVideoStream->metadata, "rotate", tag, 0);
    if (tag != NULL) {
        mInputImageParams.mRotate = atoi(tag->value);
    } else {
        mInputImageParams.mRotate = 0;
    }
    LOGD("input video rotate:%d", mInputImageParams.mRotate);
    mInputImageParams.mBitRate = pVideoStream->codecpar->bit_rate;
    mInputImageParams.mWidth = pVideoStream->codecpar->width;
    mInputImageParams.mHeight = pVideoStream->codecpar->height;
    mInputImageParams.mPixFmt = getImageExternFormat((AVPixelFormat)(pVideoStream->codecpar->format));
    mInputImageParams.mFrameRate = pVideoStream->avg_frame_rate.num * 1.0 / pVideoStream->avg_frame_rate.den;
    mInputImageParams.mDataSize = av_image_get_buffer_size((AVPixelFormat)(pVideoStream->codecpar->format), \
                                                           mInputImageParams.mWidth, mInputImageParams.mHeight, mInputImageParams.mAlign);

    av_dump_format(pFormatCtx, mPMediaDecoder->mVideoStreamIndex, mInputMediaFile, 0);

    HBError = avcodec_open2(mPMediaDecoder->mPVideoCodecCtx, mPMediaEncoder->mPVideoCodec, NULL);
    if (HBError < 0) {
        LOGE("Could not open codec. <%s>", av_err2str(HBError));
        return HB_ERROR;
    }
    
    return HB_OK;
}

int VideoFormatTranser::_OutputMediaInitial() {
    /** 是否要指定输出格式为 gif */
    int HBError = avformat_alloc_output_context2(&(mPMediaEncoder->mPVideoFormatCtx), NULL, NULL, mOutputMediaFile);
    if (HBError < 0) {
        LOGE("Create output media avforamt failed, %s !", av_err2str(HBError));
        return HBError;
    }
    
    AVFormatContext* pFormatCtx = mPMediaDecoder->mPVideoFormatCtx;
    HBError = avio_open(&(pFormatCtx->pb), mOutputMediaFile, AVIO_FLAG_WRITE);
    if (HBError < 0) {
        LOGE("Avio open output file failed, %s !", av_err2str(HBError));
        return HBError;
    }
    strncpy(pFormatCtx->filename, mOutputMediaFile, strlen(mOutputMediaFile));
    
    mPMediaEncoder->mPVideoCodec = avcodec_find_encoder_by_name("libx264");
    if (mPMediaEncoder->mPVideoCodec == NULL) {
        LOGE("Avcodec find output encode codec failed !");
        return HBError;
    }
    
    AVStream *pVideoStream = avformat_new_stream(mPMediaEncoder->mPVideoFormatCtx, mPMediaEncoder->mPVideoCodec);
    if (pVideoStream == NULL) {
        LOGE("Avformat new output video stream failed !");
        return HBError;
    }
    pVideoStream->time_base.num = 1;
    pVideoStream->time_base.den = 90000;
    mPMediaEncoder->mVideoStreamIndex = pVideoStream->index;
    
    mPMediaEncoder->mPVideoCodecCtx = avcodec_alloc_context3(mPMediaEncoder->mPVideoCodec);
    if (mPMediaEncoder->mPVideoCodecCtx == NULL) {
        LOGE("avcodec context alloc output video context failed !");
        return HBError;
    }
    AVCodecContext *pCodecCtx = mPMediaEncoder->mPVideoCodecCtx;
    
    if (mOutputImageParams.mWidth < 0) {
        pCodecCtx->width = -mOutputImageParams.mWidth;
    } else {
        pCodecCtx->width = mOutputImageParams.mWidth;
    }
    
    if (mOutputImageParams.mHeight < 0) {
        pCodecCtx->height = -mOutputImageParams.mHeight;
    } else {
        pCodecCtx->height = mOutputImageParams.mHeight;
    }

    pCodecCtx->pix_fmt = getImageInnerFormat(mOutputImageParams.mPixFmt);
    pCodecCtx->codec_id = mPMediaEncoder->mPVideoCodec->id;
    pCodecCtx->codec_type = mPMediaEncoder->mPVideoCodec->type;
    pCodecCtx->gop_size = 250;
    pCodecCtx->framerate.den = 30;
    pCodecCtx->framerate.num = 1;// {30, 1};
    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 30;
    pCodecCtx->keyint_min = 60;
    pCodecCtx->bit_rate = mOutputImageParams.mBitRate;

    if (pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
        pCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
    AVDictionary *opts = NULL;
    av_dict_set(&opts, "profile", "baseline", 0);
    
    if (pCodecCtx->codec_id == AV_CODEC_ID_H264) {
        av_opt_set(pCodecCtx->priv_data, "level", "4.1", 0);
        av_opt_set(pCodecCtx->priv_data, "preset", "superfast", 0);
        av_opt_set(pCodecCtx->priv_data, "tune", "zerolatency", 0);
    }
    
    av_dict_set(&opts, "threads", "5", 0);
    HBError = avcodec_open2(pCodecCtx, mPMediaEncoder->mPVideoCodec, &opts);
    if (HBError < 0) {
        av_dict_free(&opts);
        LOGE("Avcodec open output failed !");
        return HBError;
    }
    av_dict_free(&opts);
    
    HBError = avcodec_parameters_from_context(pVideoStream->codecpar, pCodecCtx);
    if (HBError < 0) {
        LOGE("AVcodec Copy context paramter error!\n");
        return HBError;
    }
    
    mOutputImageParams.mBitRate = pVideoStream->codecpar->bit_rate;
    mOutputImageParams.mWidth = pVideoStream->codecpar->width;
    mOutputImageParams.mHeight = pVideoStream->codecpar->height;
    mOutputImageParams.mPixFmt = getImageExternFormat((AVPixelFormat)(pVideoStream->codecpar->format));
    mOutputImageParams.mFrameRate = pVideoStream->avg_frame_rate.num * 1.0 / pVideoStream->avg_frame_rate.den;
    mOutputImageParams.mDataSize = av_image_get_buffer_size((AVPixelFormat)(pVideoStream->codecpar->format), \
                                                           mOutputImageParams.mWidth, mOutputImageParams.mHeight, mOutputImageParams.mAlign);
    
    av_dump_format(pFormatCtx, mPMediaEncoder->mVideoStreamIndex, mOutputMediaFile, 1);
    return HB_OK;
}

int VideoFormatTranser::_SwsMediaInitial() {
    if (mInputImageParams.mWidth != mOutputImageParams.mWidth \
        || mInputImageParams.mHeight != mOutputImageParams.mHeight \
        || mInputImageParams.mPixFmt != mOutputImageParams.mPixFmt)
    {
        mPVideoConvertCtx = sws_getContext(mInputImageParams.mWidth, mInputImageParams.mHeight, getImageInnerFormat(mInputImageParams.mPixFmt), \
                                mOutputImageParams.mWidth, mOutputImageParams.mHeight, getImageInnerFormat(mOutputImageParams.mPixFmt), \
                                SWS_BICUBIC, NULL, NULL, NULL);
        if (!mPVideoConvertCtx) {
            LOGE("Create video format sws context failed !");
            return HB_ERROR;
        }
    }
    return HB_OK;
}
    
VideoFormatTranser::~VideoFormatTranser() {
    if (mInputMediaFile)
        av_freep(&mInputMediaFile);
    if (mOutputMediaFile)
        av_freep(&mOutputMediaFile);
}

void VideoFormatTranser::setInputVideoMediaFile(char *pFilePath) {
    if (mInputMediaFile)
        av_freep(&mInputMediaFile);
    mInputMediaFile = av_strdup(pFilePath);
}

void VideoFormatTranser::setOutputVideoMediaFile(char *pFilePath) {
    if (mOutputMediaFile)
        av_freep(&mOutputMediaFile);
    mOutputMediaFile = av_strdup(pFilePath);
}

}
