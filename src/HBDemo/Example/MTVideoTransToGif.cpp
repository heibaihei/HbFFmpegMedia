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
        coder->mPVideoStream = nullptr;
        coder->mPVideoCodec = nullptr;
        coder->mVideoStreamIndex = INVALID_STREAM_INDEX;
    }
    return coder;
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
    if (!(mState & STATE_UNKNOWN)) {
        LOGE("Video transer state error, no initial state, can't do prepare !");
        return HB_ERROR;
    }
    
    av_register_all();
    avformat_network_init();
    
    if (_InputMediaInitial() != HB_OK) {
        LOGE("Input media initial failed !");
        goto PREPARE_END_LABEL;
    }
    
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
    _release();
    return HB_ERROR;
}

int VideoFormatTranser::doConvert() {
    if (!(mState & STATE_INITIALED)) {
        LOGE("Video format transer has't initial, can't do format convert !");
        return HB_ERROR;
    }
    
    int HBError = HB_ERROR;
    AVPacket *pNewPacket = nullptr;
    bool bNeedTranscode = ((mPVideoConvertCtx || (mPMediaDecoder->mPVideoCodec->id != mPMediaEncoder->mPVideoCodec->id)) ? true : false);
    if ((HBError = avformat_write_header(mPMediaEncoder->mPVideoFormatCtx, NULL)) < 0) {
        LOGE("Avformat write header failed, %s!", makeErrorStr(HBError));
        goto CONVERT_END_LABEL;
    }
    
    pNewPacket = av_packet_alloc();
    while (!((mState & STATE_ABORT) || (mState & STATE_TRANS_END))) {
        
        if (!(mState & STATE_READ_END)) {
            HBError = av_read_frame(mPMediaDecoder->mPVideoFormatCtx, pNewPacket);
            if (HBError < 0) {
                mState |= STATE_READ_END;
                if (HBError == AVERROR_EOF && !bNeedTranscode)
                    mState |= STATE_TRANS_END;
                LOGW("Read input video data end !");
                continue;
            }
            
            /** 得到数据包 */
            if (pNewPacket->stream_index != mPMediaDecoder->mVideoStreamIndex) {
                av_packet_unref(pNewPacket);
                continue;
            }
        }
        
        if (bNeedTranscode) {
            if ((HBError = _TransMedia(&pNewPacket)) != 0) {
                if (pNewPacket)
                    av_packet_unref(pNewPacket);
                if (HBError == -2) {/** 转码发生异常，则直接退出 */
                    mState |= STATE_ABORT;
                    LOGE("Trans video media packet occur exception !");
                }
                continue;
            }
        }
        
        HBError = av_interleaved_write_frame(mPMediaEncoder->mPVideoFormatCtx, pNewPacket);
        if (HBError < 0) {
            LOGE("Write frame failed !");
            mState |= STATE_ABORT;/** 如果写入失败，则直接退出 */
        }
        av_packet_unref(pNewPacket);
    }
    
    if ((HBError = av_write_trailer(mPMediaEncoder->mPVideoFormatCtx)) != 0)
        LOGE("AVformat wirte tailer failed, %s ", makeErrorStr(HBError));
    
    av_packet_free(&pNewPacket);
    _release();
    return HB_OK;

CONVERT_END_LABEL:
    av_packet_free(&pNewPacket);
    _release();
    return HB_ERROR;
}

int VideoFormatTranser::_release() {
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
    
    if (mPMediaEncoder->mPVideoFormatCtx)
        avformat_close_input(&(mPMediaEncoder->mPVideoFormatCtx));
    if (mPMediaDecoder->mPVideoFormatCtx)
        avformat_close_input(&(mPMediaDecoder->mPVideoFormatCtx));
    
    mPMediaDecoder->mPVideoStream = nullptr;
    mPMediaEncoder->mPVideoStream = nullptr;
    
    memset(&mState, 0x00, sizeof(mState));
    mState |= STATE_UNKNOWN;
    return HB_OK;
}

int VideoFormatTranser::_TransMedia(AVPacket** pInPacket) {
    int HBError = -1;
    if (!pInPacket || !(*pInPacket)) {
        LOGE("Video trans failed, input a invalid packet !");
        return HBError;
    }
    
    AVFrame* pNewFrame = nullptr;
    if (!(mState & STATE_DECODE_END)) {
        if (!(mState & STATE_READ_END) && !(mState & STATE_DECODE_ABORT)) {
            /** 只有读数据包未结束以及本身解码器没有发生异常，说明都要往里面丢数据 */
            HBError = avcodec_send_packet(mPMediaDecoder->mPVideoCodecCtx, *pInPacket);
            if (HBError != 0) {
                if (HBError != AVERROR(EAGAIN)) {
                    mState |= STATE_DECODE_ABORT;
                    if (!(mState & STATE_READ_END)) {
                        /** 如果解包未结束，但是解码发生异常，则直接当作异常退出 */
                        mState |= STATE_ABORT;
                    }
                }
                return -1;
            }
        }
        else if (!(mState & STATE_DECODE_FLUSHING) \
                 && ((mState & STATE_READ_END) || (mState & STATE_DECODE_ABORT))) {
            HBError = avcodec_send_packet(mPMediaDecoder->mPVideoCodecCtx, NULL);
            mState |= STATE_DECODE_FLUSHING;
            LOGW("Decode process into flush buffer !");
        }
        
        pNewFrame = av_frame_alloc();
        if (!pNewFrame) {
            LOGE("Trans media malloc new frame failed !");
            return -2;
        }
        
        while (true) {
            HBError = avcodec_receive_frame(mPMediaDecoder->mPVideoCodecCtx, pNewFrame);
            if (HBError != 0) {
                if ((HBError != AVERROR(EAGAIN)) || (mState & STATE_DECODE_FLUSHING)) {
                    mState |= STATE_DECODE_END;
                    LOGW("Decode process end!");
                }
                av_frame_free(&pNewFrame);
                return -1;
            }
            break;
        }
    }
    
    /** 将解码数据进行图像转码 */
    if (mPVideoConvertCtx && pNewFrame) {
        if (_ImageConvert(&pNewFrame) != HB_OK) {
            if (pNewFrame)
                av_frame_free(&pNewFrame);
            LOGE("image convert failed !");
            return -1;
        }
    }
    
    if (!(mState & STATE_DECODE_END) && !(mState & STATE_ENCODE_ABORT)) {
        HBError = avcodec_send_frame(mPMediaEncoder->mPVideoCodecCtx, pNewFrame);
        if (HBError != 0) {
            LOGE("image convert failed, %s", makeErrorStr(HBError));
            av_frame_free(&pNewFrame);
            if (HBError != AVERROR(EAGAIN)) {
                mState |= STATE_ENCODE_ABORT;
                if (!(mState & STATE_DECODE_END)) {
                    /** 如果帧原始数据未解码结束，编码器发生异常，则直接当作异常退出 */
                    mState |= STATE_ABORT;
                }
            }
            return -1;
        }
        av_frame_free(&pNewFrame);
    }
    else if (!(mState & STATE_ENCODE_FLUSHING) \
             && ((mState & STATE_DECODE_END) || (mState & STATE_ENCODE_ABORT))) {
        HBError = avcodec_send_frame(mPMediaEncoder->mPVideoCodecCtx, NULL);
        mState |= STATE_ENCODE_FLUSHING;
        LOGW("Encode process into flush buffer !");
    }
    
    AVPacket *pNewPacket = av_packet_alloc();
    if (!pNewPacket) {
        LOGE("Video format transer alloc new packet room failed !");
        return -2;
    }
    
    while (true) {
        HBError = avcodec_receive_packet(mPMediaEncoder->mPVideoCodecCtx, pNewPacket);
        if (HBError != 0) {
            if ((HBError != AVERROR(EAGAIN))) {
                if (mState & STATE_ENCODE_FLUSHING) {
                    LOGW("tran codec finished !%s", av_err2str(HBError));
                    mState |= STATE_ENCODE_END;
                }
                else {
                    LOGE("trans decode video frame failed !%s", av_err2str(HBError));
                    mState |= STATE_ENCODE_ABORT;
                }
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
        return HB_ERROR;
    }
    
    AVFrame *pNewFrame = av_frame_alloc();
    if (!pNewFrame) {
        LOGE("Image conver, alloc new frame room failed !");
        goto IMAGE_CONVERT_END_LABEL;
    }
    
    HBError = av_image_alloc(pNewFrame->data, pNewFrame->linesize, mOutputImageParams.mWidth, mOutputImageParams.mHeight, \
                  getImageInnerFormat(mOutputImageParams.mPixFmt), mOutputImageParams.mAlign);
    if (HBError < 0) {
        LOGE("Video format alloc new image room failed !");
        goto IMAGE_CONVERT_END_LABEL;
    }
    
    if (sws_scale(mPVideoConvertCtx, (*pInFrame)->data, (*pInFrame)->linesize, 0, (*pInFrame)->height, \
                  pNewFrame->data, pNewFrame->linesize) <= 0) {
        LOGE("Image convert sws failed !");
        goto IMAGE_CONVERT_END_LABEL;
    }
    /** 此部分操作待续 */
    pNewFrame->width = (*pInFrame)->width;
    pNewFrame->height = (*pInFrame)->height;
    pNewFrame->pts =  av_rescale_q((*pInFrame)->pts, \
                                   mPMediaDecoder->mPVideoStream->time_base, \
                                   mPMediaEncoder->mPVideoStream->time_base);
    pNewFrame->format = getImageInnerFormat(mOutputImageParams.mPixFmt);
    
    av_frame_free(pInFrame);
    *pInFrame = pNewFrame;
    return HB_OK;
    
IMAGE_CONVERT_END_LABEL:
    if (pNewFrame)
        av_frame_free(&pNewFrame);
    return HB_ERROR;
}

int VideoFormatTranser::_InputMediaInitial() {
    AVDictionaryEntry *tag = NULL;
    AVFormatContext* pFormatCtx = nullptr;
    AVStream* pVideoStream = nullptr;
    
    int HBError = avformat_open_input(&(mPMediaDecoder->mPVideoFormatCtx), mInputMediaFile, NULL, NULL);
    if (HBError != 0) {
        LOGE("Video decoder couldn't open input file <%s>", av_err2str(HBError));
        goto INPUT_INITIAL_END_LABEL;
    }
    
    HBError = avformat_find_stream_info(mPMediaDecoder->mPVideoFormatCtx, NULL);
    if (HBError < 0) {
        LOGE("Video decoder couldn't find stream information. <%s>", av_err2str(HBError));
        goto INPUT_INITIAL_END_LABEL;
    }
    
    pFormatCtx = mPMediaDecoder->mPVideoFormatCtx;
    mPMediaDecoder->mPVideoStream = nullptr;
    mPMediaDecoder->mVideoStreamIndex = INVALID_STREAM_INDEX;
    for (int i=0; i<pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            mPMediaDecoder->mVideoStreamIndex = i;
            mPMediaDecoder->mPVideoStream = pFormatCtx->streams[i];
            break;
        }
    }
    
    if (mPMediaDecoder->mVideoStreamIndex == INVALID_STREAM_INDEX) {
        LOGW("Video decoder counldn't find valid audio stream !");
        goto INPUT_INITIAL_END_LABEL;
    }
    
    pVideoStream = pFormatCtx->streams[mPMediaDecoder->mVideoStreamIndex];
    mPMediaDecoder->mPVideoCodec = avcodec_find_decoder(pVideoStream->codecpar->codec_id);
    if (!mPMediaDecoder->mPVideoCodec) {
        LOGE("Codec <%d> not found !", pVideoStream->codecpar->codec_id);
        goto INPUT_INITIAL_END_LABEL;
    }
    
    mPMediaDecoder->mPVideoCodecCtx = avcodec_alloc_context3(mPMediaDecoder->mPVideoCodec);
    if (!mPMediaDecoder->mPVideoCodecCtx) {
        LOGE("Codec ctx <%d> not found !", pVideoStream->codecpar->codec_id);
        goto INPUT_INITIAL_END_LABEL;
    }
    avcodec_parameters_to_context(mPMediaDecoder->mPVideoCodecCtx, pVideoStream->codecpar);
    
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
        goto INPUT_INITIAL_END_LABEL;
    }
    
    return HB_OK;
INPUT_INITIAL_END_LABEL:
    if (mPMediaDecoder->mPVideoCodecCtx) {
        avcodec_close(mPMediaDecoder->mPVideoCodecCtx);
        avcodec_free_context(&(mPMediaDecoder->mPVideoCodecCtx));
    }
    if (mPMediaDecoder->mPVideoFormatCtx) {
        avformat_close_input(&(mPMediaDecoder->mPVideoFormatCtx));
        mPMediaDecoder->mPVideoFormatCtx = nullptr;
    }
    mPMediaDecoder->mVideoStreamIndex = INVALID_STREAM_INDEX;
    mPMediaDecoder->mPVideoStream = nullptr;
    return HB_ERROR;
}

int VideoFormatTranser::_OutputMediaInitial() {
    
    {/** 参数初始化 */
        if (mOutputImageParams.mWidth <= 0 || mOutputImageParams.mHeight <= 0) {
            mOutputImageParams.mWidth = mInputImageParams.mWidth;
            mOutputImageParams.mHeight = mInputImageParams.mHeight;
        }
        if (mOutputImageParams.mBitRate <= 1000)
            mOutputImageParams.mBitRate = mInputImageParams.mBitRate;
        if (mOutputImageParams.mFrameRate <= 0)
            mOutputImageParams.mFrameRate = mInputImageParams.mFrameRate;
        mOutputImageParams.mPixFmt = CS_PIX_FMT_RGB8;
        mOutputImageParams.mAlign = 4;
    }
    
    AVFormatContext* pFormatCtx = nullptr;
    AVStream *pVideoStream = nullptr;
    AVCodecContext *pCodecCtx = nullptr;
    AVDictionary *opts = NULL;
    
    /** 是否要指定输出格式为 gif */
    int HBError = avformat_alloc_output_context2(&(mPMediaEncoder->mPVideoFormatCtx), NULL, NULL, mOutputMediaFile);
    if (HBError < 0) {
        LOGE("Create output media avforamt failed, %s !", av_err2str(HBError));
        goto OUTPUT_INITIAL_END_LABEL;
    }
    
    pFormatCtx = mPMediaEncoder->mPVideoFormatCtx;
    HBError = avio_open(&(pFormatCtx->pb), mOutputMediaFile, AVIO_FLAG_WRITE);
    if (HBError < 0) {
        LOGE("Avio open output file failed, %s !", av_err2str(HBError));
        goto OUTPUT_INITIAL_END_LABEL;
    }
    
    strncpy(pFormatCtx->filename, mOutputMediaFile, strlen(mOutputMediaFile));
    mPMediaEncoder->mPVideoCodec = avcodec_find_encoder_by_name("gif");
    if (mPMediaEncoder->mPVideoCodec == NULL) {
        LOGE("Avcodec find output encode codec failed !");
        goto OUTPUT_INITIAL_END_LABEL;
    }
    
    pVideoStream = avformat_new_stream(mPMediaEncoder->mPVideoFormatCtx, mPMediaEncoder->mPVideoCodec);
    if (pVideoStream == NULL) {
        LOGE("Avformat new output video stream failed !");
        goto OUTPUT_INITIAL_END_LABEL;
    }
    pVideoStream->time_base.num = 1;
    pVideoStream->time_base.den = 90000;
    mPMediaEncoder->mVideoStreamIndex = pVideoStream->index;
    mPMediaEncoder->mPVideoStream = pVideoStream;
    
    mPMediaEncoder->mPVideoCodecCtx = avcodec_alloc_context3(mPMediaEncoder->mPVideoCodec);
    if (mPMediaEncoder->mPVideoCodecCtx == NULL) {
        LOGE("avcodec context alloc output video context failed !");
        goto OUTPUT_INITIAL_END_LABEL;
    }
    
    pCodecCtx = mPMediaEncoder->mPVideoCodecCtx;
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

//    av_dict_set(&opts, "profile", "baseline", 0);
    if (pCodecCtx->codec_id == AV_CODEC_ID_H264) {
        av_opt_set(pCodecCtx->priv_data, "level", "4.1", 0);
        av_opt_set(pCodecCtx->priv_data, "preset", "superfast", 0);
        av_opt_set(pCodecCtx->priv_data, "tune", "zerolatency", 0);
    }
    
    av_dict_set(&opts, "threads", "auto", 0);
    HBError = avcodec_open2(pCodecCtx, mPMediaEncoder->mPVideoCodec, &opts);
    if (HBError < 0) {
        av_dict_free(&opts);
        LOGE("Avcodec open output failed !");
        goto OUTPUT_INITIAL_END_LABEL;
    }
    av_dict_free(&opts);
    
    HBError = avcodec_parameters_from_context(pVideoStream->codecpar, pCodecCtx);
    if (HBError < 0) {
        LOGE("AVcodec Copy context paramter error!\n");
        goto OUTPUT_INITIAL_END_LABEL;
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
    
OUTPUT_INITIAL_END_LABEL:
    if (mPMediaEncoder->mPVideoCodecCtx) {
        avcodec_close(mPMediaEncoder->mPVideoCodecCtx);
        avcodec_free_context(&(mPMediaEncoder->mPVideoCodecCtx));
    }
    mPMediaEncoder->mPVideoCodec = nullptr;
    if (mPMediaEncoder->mPVideoFormatCtx) {
        avformat_close_input(&(mPMediaEncoder->mPVideoFormatCtx));
        mPMediaEncoder->mPVideoFormatCtx = nullptr;
    }
    return HB_ERROR;
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

void VideoFormatTranser::setVideoOutputFrameRate(float frameRate) {
    mOutputImageParams.mFrameRate = frameRate;
}

void VideoFormatTranser::setVideoOutputBitrate(int64_t bitrate) {
    mOutputImageParams.mBitRate = bitrate;
}

void VideoFormatTranser::setVideoOutputSize(int width, int height) {
    mOutputImageParams.mWidth = width;
    mOutputImageParams.mHeight = height;
}

void VideoFormatTranser::setOutputVideoMediaFile(char *pFilePath) {
    if (mOutputMediaFile)
        av_freep(&mOutputMediaFile);
    mOutputMediaFile = av_strdup(pFilePath);
}

}
