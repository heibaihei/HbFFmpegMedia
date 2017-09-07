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
    memset(&mSrcPicParam, 0x00, sizeof(ImageParams));
    
    mTrgPicMediaFile = nullptr;
    mTrgPicFileHandle = nullptr;
    mTrgPicDataType = MD_TYPE_UNKNOWN;
    memset(&mTrgPicParam, 0x00, sizeof(ImageParams));
}

CSPicture::~CSPicture() {
    if (mSrcPicMediaFile)
        av_freep(&mSrcPicMediaFile);
    if (mTrgPicMediaFile)
        av_freep(&mTrgPicMediaFile);
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

int CSPicture::picPrepare() {

    if (HB_OK != picBaseInitial()) {
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
    
    _EchoPictureMediaInfo();
    return HB_OK;
}

int CSPicture::picDispose() {
    if (HB_OK != picEncoderClose()) {
        return HB_ERROR;
    }
    
    if (HB_OK != picEncoderRelease()) {
        return HB_ERROR;
    }
    return HB_ERROR;
}
    
int  CSPicture::picBaseInitial() {
    if (HB_OK != globalInitial()) {
        LOGF("global initial failed !");
        return HB_ERROR;
    }
    
    if (HB_OK != _checkPicMediaValid()) {
        LOGE("Check pic media args failed !");
        return HB_ERROR;
    }
    
    switch (mTrgPicDataType) {
        case MD_TYPE_RAW_BY_MEMORY:
            break;
        case MD_TYPE_RAW_BY_FILE:
        {
            if (mTrgPicMediaFile && !mTrgPicFileHandle) {
                mTrgPicFileHandle = fopen(mTrgPicMediaFile, "rb");
                if (!mTrgPicFileHandle) {
                    LOGE("Picture encoder open failed !");
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
                    LOGE("Picture encoder open failed !");
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

int  CSPicture::getPicRawData(uint8_t** pData, int* pDataSizes) {

    int HbErr = HB_ERROR;
    if (!pData || !pDataSizes) {
        LOGE("Picture get raw data the args is invalid !");
        return HB_ERROR;
    }

    *pData = NULL;
    *pDataSizes = 0;
    
    uint8_t* pictureDataBuffer = nullptr;
    int  pictureDataBufferSize = 0;
    
    if (mSrcPicDataType != MD_TYPE_COMPRESS) {
        if (mSrcPicMediaFile && mSrcPicFileHandle) {
            pictureDataBufferSize = av_image_get_buffer_size(getImageInnerFormat(mSrcPicParam.mPixFmt), mSrcPicParam.mWidth, mSrcPicParam.mHeight, mSrcPicParam.mAlign);
            if (!pictureDataBufferSize) {
                LOGE("Picture get buffer size failed<%d> ! ", pictureDataBufferSize);
                goto READ_PIC_DATA_END_LABEL;
            }
            
            pictureDataBuffer = (uint8_t *)av_mallocz(pictureDataBufferSize);
            if (!pictureDataBuffer) {
                LOGE("Picture create data buffer failed !");
                goto READ_PIC_DATA_END_LABEL;
            }

            if (fread(pictureDataBuffer, 1, pictureDataBufferSize, mSrcPicFileHandle) <= 0) {
                if (feof(mSrcPicFileHandle))
                    HbErr = HB_EOF;
                else
                    LOGE("Read picture raw data failed !");
                goto READ_PIC_DATA_END_LABEL;
            }
        }
        else
            LOGE("Get data by other ways <memory ...> !");
        
        *pData = pictureDataBuffer;
        *pDataSizes = pictureDataBufferSize;
    }
    else {
        /** 以别的方式获取数据 */
        LOGE("Get data by other ways !");
    }
    
    return HB_OK;
    
READ_PIC_DATA_END_LABEL:
    if (pictureDataBuffer) {
        av_free(pictureDataBuffer);
        pictureDataBuffer = nullptr;
    }
    return HbErr;
}
    
int  CSPicture::pictureEncode(uint8_t* pData, int pDataSizes) {
    if (!pData || pDataSizes <= 0) {
        LOGE("picture encoder the args is invalid !");
        return HB_ERROR;
    }
    
    /** 进行对应图像格式转换 */
    if (HB_OK != pictureSwscale(&pData, &pDataSizes, &mSrcPicParam, &mTrgPicParam)) {
        LOGE("Picture convert failed !");
        av_free(pData);
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
    
int  CSPicture::pictureSwscale(uint8_t** pData, int* pDataSizes, ImageParams* srcParam, ImageParams* dstParam) {

    int pictureDataBufferSize = 0;
    uint8_t *pictureDataBuffer = NULL;
    int pictureSrcDataLineSize[4] = {0, 0, 0, 0};
    int pictureDstDataLineSize[4] = {0, 0, 0, 0};
    uint8_t *pictureSrcData[4] = {NULL};
    uint8_t *pictureDstData[4] = {NULL};
    
    if (!pData || !pDataSizes) {
        LOGE("picture encoder the args is invalid !");
        return HB_ERROR;
    }
    
    SwsContext *pictureConvertCtx = sws_getContext(srcParam->mWidth, srcParam->mHeight, getImageInnerFormat(srcParam->mPixFmt), \
                                                   dstParam->mWidth, dstParam->mHeight, getImageInnerFormat(dstParam->mPixFmt), \
                                                   SWS_BICUBIC, NULL, NULL, NULL \
                                                   );
    if (!pictureConvertCtx) {
        LOGE("Create picture sws context failed !");
        return HB_ERROR;
    }
    
    pictureDataBufferSize = av_image_get_buffer_size(getImageInnerFormat(dstParam->mPixFmt), dstParam->mWidth, dstParam->mHeight, dstParam->mAlign);
    if (!pictureDataBufferSize) {
        LOGE("Picture get buffer size failed<%d> ! ", pictureDataBufferSize);
        goto SWS_PIC_DATA_END_LABEL;
    }
    
    pictureDataBuffer = (uint8_t *)av_mallocz(pictureDataBufferSize);
    if (!pictureDataBuffer) {
        LOGE("Picture create data buffer failed !");
        goto SWS_PIC_DATA_END_LABEL;
    }
    
    av_image_fill_arrays(pictureSrcData, pictureSrcDataLineSize, *pData, getImageInnerFormat(srcParam->mPixFmt), srcParam->mWidth, srcParam->mHeight, srcParam->mAlign);
    av_image_fill_arrays(pictureDstData, pictureDstDataLineSize, pictureDataBuffer, getImageInnerFormat(dstParam->mPixFmt), dstParam->mWidth, dstParam->mHeight, dstParam->mAlign);
    
    if (sws_scale(pictureConvertCtx, (const uint8_t* const*)pictureSrcData, pictureSrcDataLineSize, 0, srcParam->mHeight, pictureDstData, pictureDstDataLineSize) <= 0) {
        LOGE("Picture sws scale failed !");
        av_freep(&pictureDataBuffer);
        goto SWS_PIC_DATA_END_LABEL;
    }
    
    /** 将外部传入的数据移除，使用内部创建的图像空间 */
    av_free(*pData);
    *pData = pictureDataBuffer;
    *pDataSizes = pictureDataBufferSize;
    if (pictureConvertCtx) {
        sws_freeContext(pictureConvertCtx);
        pictureConvertCtx = nullptr;
    }
    return HB_OK;
    
SWS_PIC_DATA_END_LABEL:
    if (pictureDataBuffer)
        av_freep(&pictureDataBuffer);
    
    if (pictureConvertCtx) {
        sws_freeContext(pictureConvertCtx);
        pictureConvertCtx = nullptr;
    }
    return HB_ERROR;
}
    
int  CSPicture::picDecoderInitial() {
    if (mSrcPicDataType == MD_TYPE_COMPRESS) {
    
    }
    return HB_OK;
}

int  CSPicture::picDecoderOpen() {
    return HB_OK;
}

int  CSPicture::picDecoderClose() {
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
        LOGI("<<< =================================================");
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
        LOGI("<<< =================================================");
    }
}

int CSPicture::setSrcPicDataType(MEDIA_DATA_TYPE type) {
    mSrcPicDataType = type;
    return HB_OK;
}

int CSPicture::setTrgPicDataType(MEDIA_DATA_TYPE type) {
    mTrgPicDataType = type;
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

char *CSPicture::getInputPicMediaFile() {
    return mSrcPicMediaFile;
}

/**
 *  配置输出的音频文件
 */
void CSPicture::setOutputPicMediaFile(char *file) {
    if (mTrgPicMediaFile)
        av_freep(&mTrgPicMediaFile);
    mTrgPicMediaFile = av_strdup(file);
}

char *CSPicture::getOutputPicMediaFile() {
    return mTrgPicMediaFile;
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
