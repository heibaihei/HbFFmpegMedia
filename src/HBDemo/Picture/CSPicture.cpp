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
    /**
     * 输入媒体信息
     */
    mSrcPicMediaFile = nullptr;
    mSrcPicFileHandle = nullptr;
    mSrcPicDataType = PIC_D_TYPE_UNKNOWN;
    memset(&mSrcPicParam, 0x00, sizeof(ImageParams));
    
    /**
     * 输出媒体信息
     */
    mTrgPicMediaFile = nullptr;
    mTrgPicFileHandle = nullptr;
    mTrgPicDataType = PIC_D_TYPE_UNKNOWN;
    memset(&mTrgPicParam, 0x00, sizeof(ImageParams));
    
    mOutputPicCodec = nullptr;
    mOutputPicCodecCtx = nullptr;
    mOutputPicStream = nullptr;
    mOutputPicFormat = nullptr;
}

CSPicture::~CSPicture() {
    if (mSrcPicMediaFile)
        av_freep(&mSrcPicMediaFile);
    if (mTrgPicMediaFile)
        av_freep(&mTrgPicMediaFile);
}

int CSPicture::_checkPicMediaValid() {
    if (mTrgPicDataType == PIC_D_TYPE_UNKNOWN || mSrcPicDataType == PIC_D_TYPE_UNKNOWN) {
        LOGE("Picture data type is invalid !");
        return HB_ERROR;
    }
    
    if ((mSrcPicParam.mPixFmt == AV_PIX_FMT_YUVJ420P && mSrcPicParam.mAlign == 0) \
        || (mTrgPicParam.mPixFmt == AV_PIX_FMT_YUVJ420P && mTrgPicParam.mAlign == 0)) {
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
    
    if (HB_OK != picEncoderOpen()) {
        return HB_ERROR;
    }
    
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
    
    if (HB_OK != _checkPicMediaValid()) {
        LOGE("Picture check picture value invalid !");
        return HB_ERROR;
    }
    
    if (HB_OK != globalInitial()) {
        LOGF("Picture common initial faile !");
        return HB_ERROR;
    }
    
    if (mTrgPicDataType != PIC_D_TYPE_COMPRESS) {
        if (mTrgPicMediaFile && !mTrgPicFileHandle) {
            /**
             * 如果输入文件是裸流，则需要打开文件，获取文件句柄
             */
            mTrgPicFileHandle = fopen(mTrgPicMediaFile, "rb");
            if (!mTrgPicFileHandle) {
                LOGE("Picture encoder open failed !");
                return HB_ERROR;
            }
        }
        else {
            /** 说明文件是以内存数据或者别的方式传入 */
        }
    }
    
    if (mSrcPicDataType != PIC_D_TYPE_COMPRESS) {
        if (mSrcPicMediaFile && !mSrcPicFileHandle) {
            /**
             * 如果输入文件是裸流，则需要打开文件，获取文件句柄
             */
            mSrcPicFileHandle = fopen(mSrcPicMediaFile, "rb");
            if (!mSrcPicFileHandle) {
                LOGE("Picture encoder open failed !");
                return HB_ERROR;
            }
        }
        else {
            /** 说明文件是以内存数据或者别的方式传入 */
        }
    }
    
    return HB_OK;
}
    
int  CSPicture::picEncoderInitial() {
    /**
     * 针对输出文件
     */
    if (mTrgPicDataType == PIC_D_TYPE_COMPRESS) {
        if (!mTrgPicMediaFile) {
            LOGE("Pictue encoder initial, can't use the output media file !");
            return HB_ERROR;
        }
        
#if 0
        /** 直接通过文件名，开辟封装格式空间 */
        int HbErr = avformat_alloc_output_context2(&mOutputPicFormat, NULL, NULL, mTrgPicMediaFile);
        if (HbErr < 0){
            LOGE("Picture allock output format failed, <%s>!", makeErrorStr(HbErr));
            return HB_ERROR;
        }
#else
        /** 外部进行初始化，分两步进行 */
        mOutputPicFormat = avformat_alloc_context();
        if (!mOutputPicFormat) {
            LOGE("Picture encoder initial, alloc avformat context failed !");
            return HB_ERROR;
        }
        mOutputPicFormat->oformat = av_guess_format(mTrgPicParam.mCodecType, NULL, NULL);
        if (!mOutputPicFormat->oformat) {
            LOGE("Picture can't find the valid muxer !");
            return HB_ERROR;
        }
#endif
        mOutputPicStream = avformat_new_stream(mOutputPicFormat, NULL);
        if (!mOutputPicStream) {
            LOGE("Pic encoder initial failed, new stream failed !");
            return HB_ERROR;
        }
        
        mOutputPicCodec = avcodec_find_encoder(mOutputPicFormat->oformat->video_codec);
        if (!mOutputPicCodec) {
            LOGE("Picture encoder initial, find avcodec encoder failed !");
            return HB_ERROR;
        }
        
        mOutputPicCodecCtx = avcodec_alloc_context3(mOutputPicCodec);
        mOutputPicCodecCtx->codec_id = mOutputPicFormat->oformat->video_codec;
        mOutputPicCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
        mOutputPicCodecCtx->pix_fmt = mTrgPicParam.mPixFmt;
        mOutputPicCodecCtx->width = mTrgPicParam.mWidth;
        mOutputPicCodecCtx->height = mTrgPicParam.mHeight;

        mOutputPicCodecCtx->time_base.num = 1;
        mOutputPicCodecCtx->time_base.den = 30;
        
        avcodec_parameters_from_context(mOutputPicStream->codecpar, mOutputPicCodecCtx);
        av_dump_format(mOutputPicFormat, 0, mTrgPicMediaFile, 1);
    }
    return HB_OK;
}
    
int  CSPicture::picEncoderOpen() {
    /**
     * 针对输出文件
     */
    if (mTrgPicDataType == PIC_D_TYPE_COMPRESS) {
        int HbErr = 0;
        if ((HbErr = avio_open(&mOutputPicFormat->pb, mTrgPicMediaFile, AVIO_FLAG_READ_WRITE)) < 0) {
            LOGE("Could't open output file, %s !", makeErrorStr(HbErr));
            return HB_ERROR;
        }
        
        if ((HbErr = avcodec_open2(mOutputPicCodecCtx, mOutputPicCodec, NULL)) < 0) {
            LOGE("Picture encoder open failed, %s!", makeErrorStr(HbErr));
            return HB_ERROR;
        }
        
        if ((HbErr = avformat_write_header(mOutputPicFormat, NULL)) < 0) {
            LOGE("Avformat write header failed, %s!", makeErrorStr(HbErr));
            return HB_ERROR;
        }
    }
    
    return HB_OK;
}

int  CSPicture::picEncoderClose() {
    /** 写入图片的尾部格式 */
    av_write_trailer(mOutputPicFormat);
    
    /** 关闭编解码器 */
    if (mOutputPicCodecCtx)
        avcodec_close(mOutputPicCodecCtx);
    
    /** 关闭文件IO */
    avio_close(mOutputPicFormat->pb);
    
    /** 如果是非压缩数据，需要关闭文件 */
    if (mSrcPicDataType != PIC_D_TYPE_COMPRESS && mSrcPicFileHandle) {
        fclose(mSrcPicFileHandle);
        mSrcPicFileHandle = nullptr;
    }
    return HB_OK;
}

int  CSPicture::picEncoderRelease() {

    avcodec_free_context(&mOutputPicCodecCtx);
    avformat_free_context(mOutputPicFormat);
    
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
    
    if (mSrcPicDataType != PIC_D_TYPE_COMPRESS) {
        if (mSrcPicMediaFile && mSrcPicFileHandle) {
            pictureDataBufferSize = av_image_get_buffer_size(mSrcPicParam.mPixFmt, mSrcPicParam.mWidth, mSrcPicParam.mHeight, mSrcPicParam.mAlign);
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
        av_image_fill_arrays(pNewFrame->data, pNewFrame->linesize, pData, mTrgPicParam.mPixFmt, mTrgPicParam.mWidth, mTrgPicParam.mHeight, mTrgPicParam.mAlign);
        pNewFrame->width = mTrgPicParam.mWidth;
        pNewFrame->height = mTrgPicParam.mHeight;
        pNewFrame->format = mTrgPicParam.mPixFmt;
        
        int HbError = avcodec_send_frame(mOutputPicCodecCtx, pNewFrame);
        if (HbError<0 && HbError != AVERROR(EAGAIN) && HbError != AVERROR_EOF) {
            av_frame_unref(pNewFrame);
            LOGE("Send packet failed !\n");
            return HB_ERROR;
        }
        
        av_frame_unref(pNewFrame);
    
        while(true) {
            HbError = avcodec_receive_packet(mOutputPicCodecCtx, pNewPacket);
            if (HbError == 0) {
                pNewPacket->stream_index = mOutputPicStream->index;
                HbError = av_write_frame(mOutputPicFormat, pNewPacket);
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
    
    int HbError = avcodec_send_frame(mOutputPicCodecCtx, NULL);
    if (HbError<0 && HbError != AVERROR(EAGAIN) && HbError != AVERROR_EOF) {
        LOGE("Picture flush encoder, send frame failed <%s> !\n", makeErrorStr(HbError));
        return HB_ERROR;
    }
    
    while(true) {
        HbError = avcodec_receive_packet(mOutputPicCodecCtx, pNewPacket);
        if (HbError == 0) {
            pNewPacket->stream_index = mOutputPicStream->index;
            HbError = av_write_frame(mOutputPicFormat, pNewPacket);
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
    
    SwsContext *pictureConvertCtx = sws_getContext(srcParam->mWidth, srcParam->mHeight, srcParam->mPixFmt, \
                                                   dstParam->mWidth, dstParam->mHeight, dstParam->mPixFmt, \
                                                   SWS_BICUBIC, NULL, NULL, NULL \
                                                   );
    if (!pictureConvertCtx) {
        LOGE("Create picture sws context failed !");
        return HB_ERROR;
    }
    
    pictureDataBufferSize = av_image_get_buffer_size(dstParam->mPixFmt, dstParam->mWidth, dstParam->mHeight, dstParam->mAlign);
    if (!pictureDataBufferSize) {
        LOGE("Picture get buffer size failed<%d> ! ", pictureDataBufferSize);
        goto SWS_PIC_DATA_END_LABEL;
    }
    
    pictureDataBuffer = (uint8_t *)av_mallocz(pictureDataBufferSize);
    if (!pictureDataBuffer) {
        LOGE("Picture create data buffer failed !");
        goto SWS_PIC_DATA_END_LABEL;
    }
    
    av_image_fill_arrays(pictureSrcData, pictureSrcDataLineSize, *pData, srcParam->mPixFmt, srcParam->mWidth, srcParam->mHeight, srcParam->mAlign);
    av_image_fill_arrays(pictureDstData, pictureDstDataLineSize, pictureDataBuffer, dstParam->mPixFmt, dstParam->mWidth, dstParam->mHeight, dstParam->mAlign);
    
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
    if (mTrgPicDataType == PIC_D_TYPE_UNKNOWN \
        || mSrcPicDataType == PIC_D_TYPE_UNKNOWN) {
        LOGE("Picture encoder initial failed, pciture data is invalid!");
        return HB_ERROR;
    }
    {
        /**
         * 针对输入文件
         */
        if (mSrcPicDataType == PIC_D_TYPE_COMPRESS) {
            
        }
        else {
            
        }
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
