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
    memset(&mSrcPicParam, 0x00, sizeof(PictureParams));
    
    /**
     * 输出媒体信息
     */
    mTargetPicMediaFile = nullptr;
    mTargetPicFileHandle = nullptr;
    mTargetPicDataType = PIC_D_TYPE_UNKNOWN;
    memset(&mTargetPicParam, 0x00, sizeof(PictureParams));
    
    mOutputPicCodec = nullptr;
    mOutputPicCodecCtx = nullptr;
    mOutputPicStream = nullptr;
    mOutputPicFormat = nullptr;
}

CSPicture::~CSPicture() {
    if (mSrcPicMediaFile)
        av_freep(mSrcPicMediaFile);
    if (mTargetPicMediaFile)
        av_freep(mTargetPicMediaFile);
}

int CSPicture::_checkPicMediaValid() {
    if (mTargetPicDataType == PIC_D_TYPE_UNKNOWN || mSrcPicDataType == PIC_D_TYPE_UNKNOWN) {
        LOGE("Picture data type is invalid !");
        return HB_ERROR;
    }
    return HB_OK;
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
    return HB_OK;
}
    
int  CSPicture::picEncoderInitial() {
    /**
     * 针对输出文件
     */
    if (mTargetPicDataType == PIC_D_TYPE_COMPRESS) {
        if (!mTargetPicMediaFile) {
            LOGE("Pictue encoder initial, can't use the output media file !");
            return HB_ERROR;
        }
        
        mOutputPicFormat = avformat_alloc_context();
        if (!mOutputPicFormat) {
            LOGE("Picture encoder initial, alloc avformat context failed !");
            return HB_ERROR;
        }
        mOutputPicFormat->oformat = av_guess_format(mTargetPicParam.mCodecType, NULL, NULL);
        if (!mOutputPicFormat->oformat) {
            LOGE("Picture can't find the valid muxer !");
            return HB_ERROR;
        }
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
        mOutputPicCodecCtx->pix_fmt = mTargetPicParam.mPixFmt;
        mOutputPicCodecCtx->width = mTargetPicParam.mWidth;
        mOutputPicCodecCtx->height = mTargetPicParam.mHeight;
        
        avcodec_parameters_from_context(mOutputPicStream->codecpar, mOutputPicCodecCtx);
        av_dump_format(mOutputPicFormat, 0, mTargetPicMediaFile, 1);
    }
    return HB_OK;
}
    
int  CSPicture::picEncoderOpen() {
    /** ############################################## **/
    {
        /**
         * 如果输入文件是裸流，则需要打开文件，获取文件句柄
         */
        if (mSrcPicDataType != PIC_D_TYPE_COMPRESS) {
            if (mSrcPicMediaFile && !mSrcPicFileHandle) {
                /** 说明数据是以存放有裸数据的方式传入 */
                mSrcPicFileHandle = fopen(mSrcPicMediaFile, "rb");
                if (!mSrcPicFileHandle) {
                    LOGE("Picture encoder open failed !");
                    return HB_ERROR;
                }
            }
            else {
                /** 说明文件是以内存数据的方式传入 */
            }
        }
    }
    
    /** ############################################## **/
    {
        /**
         * 针对输出文件
         */
        if (mTargetPicDataType == PIC_D_TYPE_COMPRESS) {
            if (avio_open(&mOutputPicFormat->pb, mTargetPicMediaFile, AVIO_FLAG_READ_WRITE) < 0) {
                LOGE("Could't open output file !");
                return HB_ERROR;
            }
            
            if (avcodec_open2(mOutputPicCodecCtx, mOutputPicCodec, NULL) < 0) {
                LOGE("Picture encoder open failed, can't open avcodec !");
                return HB_ERROR;
            }
            
            if (avformat_write_header(mOutputPicFormat, NULL) < 0) {
                LOGE("Avformat write header failed !");
                return HB_ERROR;
            }
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

int  CSPicture::getPicRawData(uint8_t** pData, int* dataSizes) {

    if (!pData || !dataSizes) {
        LOGE("Picture get raw data the args is invalid !");
        return HB_ERROR;
    }
        
    uint8_t* pictureDataBuffer = nullptr;
    int  pictureDataBufferSize = 0;
    
    if (mSrcPicDataType != PIC_D_TYPE_COMPRESS) {
        if (mSrcPicMediaFile && mSrcPicFileHandle) {
            pictureDataBufferSize = av_image_get_buffer_size(mSrcPicParam.mPixFmt, mSrcPicParam.mWidth, mSrcPicParam.mHeight, mSrcPicParam.mAlign);
            pictureDataBuffer = (uint8_t *)av_malloc(pictureDataBufferSize);
            if (!pictureDataBuffer) {
                LOGE("Picture create data buffer failed !");
                return HB_ERROR;
            }

            if (fread(pictureDataBuffer, 1, pictureDataBufferSize, mSrcPicFileHandle) <= 0) {
                LOGE("Read picture raw data failed !");
                goto READ_PIC_DATA_END_LABEL;
            }
        }
        else
            LOGE("Get data by other ways <memory> !");
        
        *pData = pictureDataBuffer;
        *dataSizes = pictureDataBufferSize;
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
    return HB_ERROR;
}
    
int  CSPicture::pictureEncode(uint8_t* pData, int dataSizes) {
    if (!pData || dataSizes <= 0) {
        LOGE("picture encoder the args is invalid !");
        return HB_ERROR;
    }
    
    if (HB_OK != pictureSwscale(&pData, &dataSizes, &mSrcPicParam, &mTargetPicParam)) {
        LOGE("Picture convert failed !");
        av_free(pData);
        return HB_ERROR;
    }
    
    AVPacket* pNewPacket = av_packet_alloc();
    AVFrame* pNewFrame = av_frame_alloc();
    
//    while (true) {
        av_image_fill_arrays(pNewFrame->data, pNewFrame->linesize, pData, mTargetPicParam.mPixFmt, mTargetPicParam.mWidth, mTargetPicParam.mHeight, 1);
        pNewFrame->width = mTargetPicParam.mWidth;
        pNewFrame->height = mTargetPicParam.mHeight;
        pNewFrame->format = mTargetPicParam.mPixFmt;
        
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
        LOGE("Picture send frame send packet failed !\n");
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
        else if (HbError<0 && HbError!=AVERROR_EOF)
            break;
    }
    return HB_OK;
}
    
int  CSPicture::pictureSwscale(uint8_t** pData, int* dataSizes, PictureParams* srcParam, PictureParams* dstParam) {
    /**
     *  参考链接： http://blog.csdn.net/leixiaohua1020/article/details/14215391
     */
    if (!pData || dataSizes <= 0) {
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
    
    int pictureDataBufferSize = av_image_get_buffer_size(dstParam->mPixFmt, dstParam->mWidth, dstParam->mHeight, dstParam->mAlign);
    uint8_t *pictureDataBuffer = (uint8_t *)av_malloc(pictureDataBufferSize);
    if (!pictureDataBuffer) {
        LOGE("Picture create data buffer failed !");
        return HB_ERROR;
    }
    
    int pictureSrcDataLineSize[4] = {0, 0, 0, 0};
    int pictureDstDataLineSize[4] = {0, 0, 0, 0};
    uint8_t *pictureSrcData[4] = {NULL};
    uint8_t *pictureDstData[4] = {NULL};
    
    av_image_fill_arrays(pictureSrcData, pictureSrcDataLineSize, *pData, srcParam->mPixFmt, srcParam->mWidth, srcParam->mHeight, srcParam->mAlign);
    av_image_fill_arrays(pictureDstData, pictureDstDataLineSize, pictureDataBuffer, dstParam->mPixFmt, dstParam->mWidth, dstParam->mHeight, dstParam->mAlign);
    
    if (0 >= sws_scale(pictureConvertCtx, (const uint8_t* const*)pictureSrcData, pictureSrcDataLineSize, 0, srcParam->mHeight, pictureDstData, pictureDstDataLineSize)) {
        LOGE("Picture sws scale failed !");
        av_freep(pictureDataBuffer);
        return HB_ERROR;
    }
    
    av_free(*pData);
    *pData = pictureDataBuffer;
    *dataSizes = pictureDataBufferSize;
    return HB_OK;
}
    
int  CSPicture::picDecoderInitial() {
    if (mTargetPicDataType == PIC_D_TYPE_UNKNOWN \
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

int CSPicture::setSrcPicDataType(PIC_MEDIA_DATA_TYPE type) {
    mSrcPicDataType = type;
    return HB_OK;
}

int CSPicture::setTargetPicDataType(PIC_MEDIA_DATA_TYPE type) {
    mTargetPicDataType = type;
    return HB_OK;
}
    
/**
 *  配置输入的音频文件
 */
void CSPicture::setInputPicMediaFile(char *file) {
    if (mSrcPicMediaFile)
        av_freep(mSrcPicMediaFile);
    mSrcPicMediaFile = av_strdup(file);
}

char *CSPicture::getInputPicMediaFile() {
    return mSrcPicMediaFile;
}

/**
 *  配置输出的音频文件
 */
void CSPicture::setOutputPicMediaFile(char *file) {
    if (mTargetPicMediaFile)
        av_freep(mTargetPicMediaFile);
    mTargetPicMediaFile = av_strdup(file);
}

char *CSPicture::getOutputPicMediaFile() {
    return mTargetPicMediaFile;
}

int CSPicture::setSrcPictureParam(PictureParams* param) {
    if (!param) {
        LOGE("CSPicture set source picture param failed, arg invalid !");
        return HB_ERROR;
    }
    memcpy(&mSrcPicParam, param, sizeof(PictureParams));
    return HB_OK;
}

int CSPicture::setTargetPictureParam(PictureParams* param) {
    if (!param) {
        LOGE("CSPicture set target picture param failed, arg invalid !");
        return HB_ERROR;
    }
    memcpy(&mTargetPicParam, param, sizeof(PictureParams));
    return HB_OK;
}
    
}
