//
//  CSPicture.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/8/16.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSPicture.h"

namespace HBMedia {

CSPicture::CSPicture() {
    mSrcPicMediaFile = nullptr;
    mSrcPicFileHandle = nullptr;
    mSrcPicDataType = MD_TYPE_UNKNOWN;
    imageParamInit(&mSrcPicParam);
    
    mTrgPicMediaFile = nullptr;
    mTrgPicFileHandle = nullptr;
    mTrgPicDataType = MD_TYPE_UNKNOWN;
    imageParamInit(&mTrgPicParam);
}

CSPicture::~CSPicture() {
    if (mSrcPicMediaFile)
        av_freep(&mSrcPicMediaFile);
    if (mTrgPicMediaFile)
        av_freep(&mTrgPicMediaFile);
}

int CSPicture::prepare() {
    if (HB_OK != picCommonInitial()) {
        return HB_ERROR;
    }
    
    if (HB_OK != picEncoderInitial()) {
        return HB_ERROR;
    }
    
    if (HB_OK != picDecoderInitial()) {
        return HB_ERROR;
    }
    
    if (HB_OK != picEncoderOpen()) {
        return HB_ERROR;
    }
    
    if (HB_OK != picDecoderOpen()) {
        return HB_ERROR;
    }
    
    /** 判断是否需要进行图像格式转换 */
    mIsNeedTransform = false;
    if (mSrcPicParam.mPixFmt != mTrgPicParam.mPixFmt \
        || mSrcPicParam.mWidth != mTrgPicParam.mPixFmt \
        || mSrcPicParam.mHeight != mTrgPicParam.mPixFmt \
        || mSrcPicParam.mAlign != mTrgPicParam.mAlign) {
        mIsNeedTransform = true;
    }
    
    /** 计算输入输出的图像空间 */
    mSrcPicParam.mDataSize = av_image_get_buffer_size(getImageInnerFormat(mSrcPicParam.mPixFmt), mSrcPicParam.mWidth, mSrcPicParam.mHeight, mSrcPicParam.mAlign);
    mTrgPicParam.mDataSize = av_image_get_buffer_size(getImageInnerFormat(mTrgPicParam.mPixFmt), mTrgPicParam.mWidth, mTrgPicParam.mHeight, mTrgPicParam.mAlign);
    if (!mSrcPicParam.mDataSize || !mTrgPicParam.mDataSize) {
        LOGE("Get picture per frame sizes failed, In:%d, Out:%d", mSrcPicParam.mDataSize, mTrgPicParam.mDataSize);
        return HB_ERROR;
    }
    
    mInDataBuffer = (uint8_t *)av_mallocz(mSrcPicParam.mDataSize);
    mOutDataBuffer = (uint8_t *)av_mallocz(mTrgPicParam.mDataSize);
    if (!mInDataBuffer || !mOutDataBuffer) {
        LOGE("Malloc picture room failed, In:%p, Out:%p", mInDataBuffer, mOutDataBuffer);
        return HB_ERROR;
    }
    
    _EchoPictureMediaInfo();
    return HB_OK;
}

int CSPicture::close() {
    switch (mSrcPicDataType) {
        case MD_TYPE_COMPRESS:
            {
                if (HB_OK != picDecoderClose()) {
                    return HB_ERROR;
                }
            }
            break;
        case MD_TYPE_RAW_BY_FILE:
            break;
        default:
            break;
    }
    
    switch (mTrgPicDataType) {
        case MD_TYPE_COMPRESS:
            {
                if (HB_OK != picEncoderClose()) {
                    return HB_ERROR;
                }
            }
            break;
        case MD_TYPE_RAW_BY_FILE:
            break;
        default:
            break;
    }
    return HB_OK;
}

int CSPicture::release() {
    switch (mSrcPicDataType) {
        case MD_TYPE_COMPRESS:
            {
                if (HB_OK != picDecoderRelease()) {
                    return HB_ERROR;
                }
            }
            break;
        case MD_TYPE_RAW_BY_FILE:
            break;
        default:
            break;
    }
    
    switch (mTrgPicDataType) {
        case MD_TYPE_COMPRESS:
            {
                if (HB_OK != picEncoderRelease()) {
                    return HB_ERROR;
                }
            }
            break;
        case MD_TYPE_RAW_BY_FILE:
            break;
        default:
            break;
    }
    return HB_OK;
}

int CSPicture::dispose() {
    if (close() != HB_OK) {
        LOGE("Picture close occur unexception !");
    }
    if (release() != HB_OK) {
        LOGE("Picture release occur unexception !");
    }
    
    av_freep(&mInDataBuffer);
    av_freep(&mOutDataBuffer);
    return HB_OK;
}
    
int  CSPicture::picCommonInitial() {
    if (HB_OK != globalInitial()) {
        LOGF("global ffmpeg initial failed !");
        return HB_ERROR;
    }
    
    if (HB_OK != _checkPicMediaValid()) {
        LOGE("Picture media args invalid !");
        return HB_ERROR;
    }
    
    switch (mTrgPicDataType) {
        case MD_TYPE_RAW_BY_MEMORY:
            break;
        case MD_TYPE_RAW_BY_FILE:
            {
                if (mTrgPicMediaFile && !mTrgPicFileHandle) {
                    mTrgPicFileHandle = fopen(mTrgPicMediaFile, "wb+");
                    if (!mTrgPicFileHandle) {
                        LOGE("Open target media file failed !");
                        return HB_ERROR;
                    }
                }

            }
            break;
        case MD_TYPE_RAW_BY_PROTOCOL:
            break;
        case MD_TYPE_COMPRESS:
            /** 在后面编码模块中进行初始化 */
            break;
        default:
            break;
    }
    
    switch (mSrcPicDataType) {
        case MD_TYPE_RAW_BY_MEMORY:
            break;
        case MD_TYPE_RAW_BY_FILE:
            {
                if (mSrcPicMediaFile && !mSrcPicFileHandle) {
                    mSrcPicFileHandle = fopen(mSrcPicMediaFile, "rb");
                    if (!mSrcPicFileHandle) {
                        LOGE("Open source media file failed !");
                        return HB_ERROR;
                    }
                }
            }
            break;
        case MD_TYPE_RAW_BY_PROTOCOL:
            break;
        case MD_TYPE_COMPRESS:
            /** 在后面解码模块中进行初始化 */
            break;
        default:
            break;
    }
    
    return HB_OK;
}
    
int  CSPicture::picEncoderInitial() {
    if (mTrgPicDataType == MD_TYPE_COMPRESS) {
        if (!mTrgPicParam.mFormatType) {
            LOGE("Target picture format type unknown !");
            return HB_ERROR;
        }
        
        mOutMCodec.mFormat = avformat_alloc_context();
        if (!mOutMCodec.mFormat) {
            LOGE("Picture encoder initial, alloc avformat context failed !");
            return HB_ERROR;
        }
        mOutMCodec.mFormat->oformat = av_guess_format(mTrgPicParam.mFormatType, NULL, NULL);
        if (!mOutMCodec.mFormat->oformat) {
            LOGE("Picture can't find the valid muxer !");
            return HB_ERROR;
        }
        
        mOutMCodec.mStream = avformat_new_stream(mOutMCodec.mFormat, NULL);
        if (!mOutMCodec.mStream) {
            LOGE("Pic encoder initial failed, new stream failed !");
            return HB_ERROR;
        }
        
        mOutMCodec.mCodec = avcodec_find_encoder(mOutMCodec.mFormat->oformat->video_codec);
        if (!mOutMCodec.mCodec) {
            LOGE("Picture encoder initial, find avcodec encoder failed !");
            return HB_ERROR;
        }
        
        mOutMCodec.mCodecCtx = avcodec_alloc_context3(mOutMCodec.mCodec);
        mOutMCodec.mCodecCtx->codec_id = mOutMCodec.mCodec->id;
        mOutMCodec.mCodecCtx->codec_type = mOutMCodec.mCodec->type;
        mOutMCodec.mCodecCtx->pix_fmt = getImageInnerFormat(mTrgPicParam.mPixFmt);
        mOutMCodec.mCodecCtx->width = mTrgPicParam.mWidth;
        mOutMCodec.mCodecCtx->height = mTrgPicParam.mHeight;
        mOutMCodec.mCodecCtx->time_base.num = 1;
        mOutMCodec.mCodecCtx->time_base.den = 30;
        
        avcodec_parameters_from_context(mOutMCodec.mStream->codecpar, mOutMCodec.mCodecCtx);
        av_dump_format(mOutMCodec.mFormat, 0, mTrgPicMediaFile, 1);
    }
    return HB_OK;
}
    
int  CSPicture::picEncoderOpen() {
    if (mTrgPicDataType == MD_TYPE_COMPRESS) {
        int HbErr = 0;
        AVDictionary *opts = NULL;
        av_dict_set(&opts, "threads", "auto", 0);
        
        if ((HbErr = avio_open(&(mOutMCodec.mFormat->pb), mTrgPicMediaFile, AVIO_FLAG_READ_WRITE)) < 0) {
            LOGE("Could't open output file, %s !", makeErrorStr(HbErr));
            return HB_ERROR;
        }
        
        if ((HbErr = avcodec_open2(mOutMCodec.mCodecCtx, mOutMCodec.mCodec, &opts)) < 0) {
            LOGE("Picture encoder open failed, %s!", makeErrorStr(HbErr));
            return HB_ERROR;
        }
        
        if ((HbErr = avformat_write_header(mOutMCodec.mFormat, NULL)) < 0) {
            LOGE("Avformat write header failed, %s!", makeErrorStr(HbErr));
            return HB_ERROR;
        }
    }
    
    return HB_OK;
}

int  CSPicture::picEncoderClose() {
    /** 写入图片的尾部格式 */
    av_write_trailer(mOutMCodec.mFormat);
    
    /** 关闭编解码器 */
    if (mOutMCodec.mCodecCtx)
        avcodec_close(mOutMCodec.mCodecCtx);
    
    /** 关闭文件IO */
    avio_close(mOutMCodec.mFormat->pb);
    
    /** 如果是非压缩数据，需要关闭文件 */
    if (mSrcPicDataType != MD_TYPE_COMPRESS && mSrcPicFileHandle) {
        fclose(mSrcPicFileHandle);
        mSrcPicFileHandle = nullptr;
    }
    return HB_OK;
}

int  CSPicture::picEncoderRelease() {
    avcodec_free_context(&(mOutMCodec.mCodecCtx));
    avformat_free_context(mOutMCodec.mFormat);
    
    return HB_OK;
}

int  CSPicture::sendImageData(uint8_t** pData, int* pDataSizes) {
    
    int HbErr = transformImageData(pData, pDataSizes);
    if (HbErr != HB_OK) {
        LOGE("Transform picture data failed !");
        return HB_ERROR;
    }
    
    switch (mTrgPicDataType) {
        case MD_TYPE_COMPRESS:
            {
                if (pictureEncode(*pData, *pDataSizes) != HB_OK) {
                    LOGE("Picture encode exit !");
                    return HB_ERROR;
                }
            }
            break;
        case MD_TYPE_RAW_BY_FILE:
            {
                if (!mTrgPicMediaFile || !mTrgPicFileHandle) {
                    LOGE("Get data failed, [Handle:%p] file:%s!", mTrgPicFileHandle, mTrgPicMediaFile);
                    return HB_ERROR;
                }
                
                if (fwrite(mOutDataBuffer, mTrgPicParam.mDataSize, 1, mTrgPicFileHandle) <= 0) {
                    LOGE("Write picture data failed !");
                    return HB_ERROR;
                }
            }
            break;
        case MD_TYPE_RAW_BY_MEMORY:
        case MD_TYPE_RAW_BY_PROTOCOL:
            break;
        default:
            break;
    }
    return HB_OK;
}

int  CSPicture::transformImageData(uint8_t** pData, int* pDataSizes) {
    int HBError = HB_OK;
    if (!mIsNeedTransform)
        return HBError;
    
    HBError = pictureSwscale(pData, pDataSizes);
    if (HB_OK != HBError) {
        LOGE("Picture transform failed, %s!", av_err2str(HBError));
        av_free(pData);
        return HB_ERROR;
    }
    return HB_OK;
}

int  CSPicture::receiveImageData(uint8_t** pData, int* pDataSizes) {

    int HbErr = HB_ERROR;
    if (!pData || !pDataSizes) {
        LOGE("Picture get raw data the args is invalid !");
        return HB_ERROR;
    }

    *pData = NULL;
    *pDataSizes = 0;
    
    switch (mSrcPicDataType) {
        case MD_TYPE_RAW_BY_FILE:
            {
                if (!mSrcPicMediaFile || !mSrcPicFileHandle) {
                    LOGE("Get data failed, [Handle:%p] file:%s!", mSrcPicFileHandle, mSrcPicMediaFile);
                    HbErr = HB_ERROR;
                    goto READ_PIC_DATA_END_LABEL;
                }
                
                if (fread(mInDataBuffer, 1, mSrcPicParam.mDataSize, mSrcPicFileHandle) <= 0) {
                    if (feof(mSrcPicFileHandle))
                        HbErr = HB_EOF;
                    else
                        LOGE("Read picture raw data failed !");
                    goto READ_PIC_DATA_END_LABEL;
                }
            }
            break;
        case MD_TYPE_RAW_BY_MEMORY:
            {/** 从内存数据中获取数据 */
            }
            break;
        case MD_TYPE_COMPRESS:
            {/** 从解码模块中获取数据 */
                if (pictureDecode(pData, pDataSizes) != HB_OK) {
                    LOGE("Picture decode get data failed !");
                    HbErr = HB_ERROR;
                    goto READ_PIC_DATA_END_LABEL;
                }
            }
            break;
        default:
            LOGE("Get data by other ways !");
            break;
    }
    
    *pData = mInDataBuffer;
    *pDataSizes = mSrcPicParam.mDataSize;
    return HB_OK;
    
READ_PIC_DATA_END_LABEL:
    return HbErr;
}
    
int  CSPicture::pictureEncode(uint8_t* pData, int pDataSizes) {
    if (mTrgPicDataType != MD_TYPE_COMPRESS) {
        LOGE("Picture target media type:%s", getMediaDataTypeDescript(mTrgPicDataType));
        return HB_ERROR;
    }
    
    if (!pData || pDataSizes <= 0) {
        LOGE("picture encoder the args is invalid !");
        return HB_ERROR;
    }
    
    AVPacket* pNewPacket = av_packet_alloc();
    AVFrame* pNewFrame = av_frame_alloc();

    /**
     *   如果传入的数据量比较大，是否需要进行循环 send 帧数据
     */
//    while (true) {
        av_image_fill_arrays(pNewFrame->data, pNewFrame->linesize, pData, getImageInnerFormat(mTrgPicParam.mPixFmt), mTrgPicParam.mWidth, mTrgPicParam.mHeight, mTrgPicParam.mAlign);
        pNewFrame->width = mTrgPicParam.mWidth;
        pNewFrame->height = mTrgPicParam.mHeight;
        pNewFrame->format = mTrgPicParam.mPixFmt;
        
        int HbError = avcodec_send_frame(mOutMCodec.mCodecCtx, pNewFrame);
        if (HbError<0 && HbError != AVERROR(EAGAIN) && HbError != AVERROR_EOF) {
            av_frame_unref(pNewFrame);
            LOGE("Send packet failed !\n");
            return HB_ERROR;
        }
        
        av_frame_unref(pNewFrame);
    
        while(true) {
            HbError = avcodec_receive_packet(mOutMCodec.mCodecCtx, pNewPacket);
            if (HbError == 0) {
                pNewPacket->stream_index = mOutMCodec.mStream->index;
                HbError = av_write_frame(mOutMCodec.mFormat, pNewPacket);
                av_packet_unref(pNewPacket);
            }
            else if (HbError == AVERROR(EAGAIN))
                break;
            else if (HbError<0 && HbError!=AVERROR_EOF)
                break;
        }
//    }
    
    av_packet_free(&pNewPacket);
    av_frame_free(&pNewFrame);
    return HB_OK;
}

int  CSPicture::pictureFlushEncode() {
    
    AVPacket* pNewPacket = av_packet_alloc();
    
    int HbError = avcodec_send_frame(mOutMCodec.mCodecCtx, NULL);
    if (HbError<0 && HbError != AVERROR(EAGAIN) && HbError != AVERROR_EOF) {
        LOGE("Picture flush encoder, send frame failed <%s> !\n", makeErrorStr(HbError));
        return HB_ERROR;
    }
    
    while(true) {
        HbError = avcodec_receive_packet(mOutMCodec.mCodecCtx, pNewPacket);
        if (HbError == 0) {
            pNewPacket->stream_index = mOutMCodec.mStream->index;
            HbError = av_write_frame(mOutMCodec.mFormat, pNewPacket);
            av_packet_unref(pNewPacket);
        }
        else if (HbError == AVERROR(EAGAIN))
            break;
        else if (HbError == AVERROR_EOF)
            break;
        else if (HbError<0 && HbError!=AVERROR_EOF)
            return HB_ERROR;
    }
    return HB_OK;
}
    
int  CSPicture::pictureSwscale(uint8_t** pData, int* pDataSizes) {
    int HBErr = HB_OK;
    int pictureSrcDataLineSize[4] = {0, 0, 0, 0};
    int pictureDstDataLineSize[4] = {0, 0, 0, 0};
    uint8_t *pictureSrcData[4] = {NULL};
    uint8_t *pictureDstData[4] = {NULL};

    if (!pData || !pDataSizes) {
        LOGE("picture encoder the args is invalid !");
        return HB_ERROR;
    }

    SwsContext *pictureConvertCtx = sws_getContext(mSrcPicParam.mWidth, mSrcPicParam.mHeight, getImageInnerFormat(mSrcPicParam.mPixFmt), \
                                                   mTrgPicParam.mWidth, mTrgPicParam.mHeight, getImageInnerFormat(mTrgPicParam.mPixFmt), \
                                                   SWS_BICUBIC, NULL, NULL, NULL);
    if (!pictureConvertCtx) {
        LOGE("Create picture sws context failed !");
        return HB_ERROR;
    }

    av_image_fill_arrays(pictureSrcData, pictureSrcDataLineSize, *pData, getImageInnerFormat(mSrcPicParam.mPixFmt), mSrcPicParam.mWidth, mSrcPicParam.mHeight, mSrcPicParam.mAlign);
    av_image_fill_arrays(pictureDstData, pictureDstDataLineSize, mOutDataBuffer, getImageInnerFormat(mTrgPicParam.mPixFmt), mTrgPicParam.mWidth, mTrgPicParam.mHeight, mTrgPicParam.mAlign);
    
    if (sws_scale(pictureConvertCtx, (const uint8_t* const*)pictureSrcData, pictureSrcDataLineSize, 0, mSrcPicParam.mHeight, pictureDstData, pictureDstDataLineSize) <= 0) {
        LOGE("Picture sws scale failed !");
        HBErr = HB_ERROR;
    }
    else {
        *pData = mOutDataBuffer;
        *pDataSizes = mTrgPicParam.mDataSize;
    }

    if (pictureConvertCtx) {
        sws_freeContext(pictureConvertCtx);
        pictureConvertCtx = nullptr;
    }
    return HBErr;
}
    
int  CSPicture::picDecoderInitial() {
    if (mSrcPicDataType == MD_TYPE_COMPRESS) {
        mInMCodec.mFormat = avformat_alloc_context();
        int HBError = avformat_open_input(&(mInMCodec.mFormat), mSrcPicMediaFile, NULL, NULL);
        if (HBError != 0) {
            LOGE("Audio decoder couldn't open input file. <%d> <%s>", HBError, av_err2str(HBError));
            return HB_ERROR;
        }
        
        HBError = avformat_find_stream_info(mInMCodec.mFormat, NULL);
        if (HBError < 0) {
            LOGE("Audio decoder couldn't find stream information. <%s>", av_err2str(HBError));
            return HB_ERROR;
        }
        
        for (int i=0; i<mInMCodec.mFormat->nb_streams; i++) {
            if (mInMCodec.mFormat->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                mInMCodec.mStream = mInMCodec.mFormat->streams[i];
                break;
            }
        }
        
        if (!mInMCodec.mStream) {
            LOGE("Picture find best stream info failed !");
            return HB_ERROR;
        }
        
        mInMCodec.mCodec = avcodec_find_decoder(mInMCodec.mStream->codecpar->codec_id);
        if (!mInMCodec.mCodec) {
            LOGE("Codec ctx <%d> not found !", mInMCodec.mStream->codecpar->codec_id);
            return HB_ERROR;
        }
        
        mInMCodec.mCodecCtx = avcodec_alloc_context3(mInMCodec.mCodec);
        if (!mInMCodec.mCodecCtx) {
            LOGE("Codec ctx <%d> not found !", mInMCodec.mStream->codecpar->codec_id);
            return HB_ERROR;
        }
        
        avcodec_parameters_to_context(mInMCodec.mCodecCtx, mInMCodec.mStream->codecpar);
        av_dump_format(mInMCodec.mFormat, mInMCodec.mStream->index, mSrcPicMediaFile, false);
    }
    return HB_OK;
}

int  CSPicture::picDecoderOpen() {
    if (mSrcPicDataType == MD_TYPE_COMPRESS) {
        int HBError = avcodec_open2(mInMCodec.mCodecCtx, mInMCodec.mCodec, NULL);
        if (HBError < 0) {
            LOGE("Could not open codec. <%s>", av_err2str(HBError));
            return HB_ERROR;
        }
    }
    
    return HB_OK;
}

int  CSPicture::picDecoderClose() {
    if (mInMCodec.mCodecCtx && avcodec_is_open(mInMCodec.mCodecCtx)) {
        avcodec_close(mInMCodec.mCodecCtx);
    }
    return HB_OK;
}

int  CSPicture::picDecoderRelease() {
    return HB_OK;
}
    
void CSPicture::_EchoPictureMediaInfo() {
    /** 输出输入媒体信息： */
    {
        LOGI(">>> [INPUT] =========================================");
        LOGI("MediaType:%s", getMediaDataTypeDescript(mSrcPicDataType));
        LOGI("File:[%p]%s", mSrcPicFileHandle, mSrcPicMediaFile);
        LOGI(" --> ImageParam");
        LOGI(" Muxer:%s", mSrcPicParam.mFormatType);
        LOGI(" PixFmt:%s", getImagePixFmtDescript(mSrcPicParam.mPixFmt));
        LOGI(" width:%f", mSrcPicParam.mWidth);
        LOGI(" height:%f", mSrcPicParam.mHeight);
        LOGI(" Align:%s", (mSrcPicParam.mAlign == 1 ? "Yes":"No"));
        LOGI(" Per image size:%d", mSrcPicParam.mDataSize);
        LOGI("<<< =================================================\r\n");
    }
    
    /** 输出输出媒体信息： */
    {
        LOGI(">>> [OUTPUT] ========================================");
        LOGI("MediaType:%s", getMediaDataTypeDescript(mTrgPicDataType));
        LOGI("File:[%p]%s", mTrgPicFileHandle, mTrgPicMediaFile);
        LOGI(" --> ImageParam");
        LOGI(" Muxer:%s", mTrgPicParam.mFormatType);
        LOGI(" PixFmt:%s", getImagePixFmtDescript(mTrgPicParam.mPixFmt));
        LOGI(" width:%f", mTrgPicParam.mWidth);
        LOGI(" height:%f", mTrgPicParam.mHeight);
        LOGI(" Align:%s", (mTrgPicParam.mAlign == 1 ? "Yes":"No"));
        LOGI(" Per image size:%d", mTrgPicParam.mDataSize);
        LOGI("<<< =================================================\r\n");
    }
    
    /** 图像格式转换 */
    {
        LOGI(">>> [Transform] =====================================");
        LOGI("Need Transform:%s", (mIsNeedTransform ? "Yes" : "No"));
        LOGI("<<< =================================================\r\n");
    }
}

int  CSPicture::pictureDecode(uint8_t** pData, int* pDataSizes) {
    int HBError = HB_OK;
    if (mSrcPicDataType == MD_TYPE_COMPRESS) {
        AVPacket *pNewPacket = av_packet_alloc();
        while (true) {
            HBError = av_read_frame(mInMCodec.mFormat, pNewPacket);
            if (HBError != 0) {
                LOGE("Picture decode, read frame failed ! <%d> <%s>", HBError, av_err2str(HBError));
                HBError = HB_ERROR;
                break;
            }

            if (pNewPacket->stream_index != mInMCodec.mStream->index) {
                av_packet_unref(pNewPacket);
                continue;
            }
            
            HBError = avcodec_send_packet(mInMCodec.mCodecCtx, pNewPacket);
            if (HBError != 0) {
                LOGE("Picture decode, send packet to codec failed ! <%d> <%s>", HBError, av_err2str(HBError));
                HBError = HB_ERROR;
                break;
            }
            
            AVFrame* pNewFrame = av_frame_alloc();
            HBError = avcodec_receive_frame(mInMCodec.mCodecCtx, pNewFrame);
            if (HBError != 0) {
                LOGE("Picture decode, receive frame from codec failed ! <%d> <%s>", HBError, av_err2str(HBError));
                HBError = HB_ERROR;
                break;
            }
            
            av_image_copy_to_buffer(mInDataBuffer, mSrcPicParam.mDataSize, pNewFrame->data, pNewFrame->linesize, getImageInnerFormat(mSrcPicParam.mPixFmt), mSrcPicParam.mWidth, mSrcPicParam.mHeight, mSrcPicParam.mAlign);
            
            *pData = mInDataBuffer;
            av_frame_unref(pNewFrame);
            av_frame_free(&pNewFrame);
            break;
        }
        av_packet_free(&pNewPacket);
    }
    
    return HBError;
}
    
int  CSPicture::pictureFlushDecode() {
    if (mSrcPicDataType == MD_TYPE_COMPRESS) {
        
    }
    return HB_ERROR;
}

int CSPicture::_checkPicMediaValid() {
    if (mTrgPicDataType == MD_TYPE_UNKNOWN || mSrcPicDataType == MD_TYPE_UNKNOWN) {
        LOGE("Unknown picture data type !");
        return HB_ERROR;
    }
    
    if (MD_TYPE_RAW_BY_MEMORY != mTrgPicDataType) {
        if (!mTrgPicMediaFile) {
            LOGE("Please input valid target picture file url !");
            return HB_ERROR;
        }
    }
    if (MD_TYPE_RAW_BY_MEMORY != mSrcPicDataType) {
        if (!mSrcPicMediaFile) {
            LOGE("Please input valid source picture file url !");
            return HB_ERROR;
        }
    }
    
    if ((getImageInnerFormat(mSrcPicParam.mPixFmt) == AV_PIX_FMT_YUVJ420P && mSrcPicParam.mAlign == 0) \
        || (getImageInnerFormat(mTrgPicParam.mPixFmt)  == AV_PIX_FMT_YUVJ420P && mTrgPicParam.mAlign == 0)) {
        LOGE("Picture check valid: AV_PIX_FMT_YUVJ420P format not support (0) valud of align !");
        return HB_ERROR;
    }
    
    return HB_OK;
}

/**
 *  配置输入的音频文件
 */
void CSPicture::setInputPicMediaFile(char *file) {
    if (mSrcPicMediaFile)
        av_freep(&mSrcPicMediaFile);
    mSrcPicMediaFile = av_strdup(file);
}

/**
 *  配置输出的音频文件
 */
void CSPicture::setOutputPicMediaFile(char *file) {
    if (mTrgPicMediaFile)
        av_freep(&mTrgPicMediaFile);
    mTrgPicMediaFile = av_strdup(file);
}

int CSPicture::setSrcPictureParam(ImageParams* param) {
    if (!param) {
        LOGE("CSPicture set source picture param failed, arg invalid !");
        return HB_ERROR;
    }
    memcpy(&mSrcPicParam, param, sizeof(ImageParams));
    return HB_OK;
}

int CSPicture::setTrgPictureParam(ImageParams* param) {
    if (!param) {
        LOGE("CSPicture set target picture param failed, arg invalid !");
        return HB_ERROR;
    }
    memcpy(&mTrgPicParam, param, sizeof(ImageParams));
    return HB_OK;
}
    
}
