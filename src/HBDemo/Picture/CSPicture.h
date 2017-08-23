//
//  CSPicture.hpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/8/16.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSPicture_h
#define CSPicture_h

#include <stdio.h>

#include "HBPicture.h"
#include "HBLog.h"

/***
 *  代办事项
 *  1、转成特定格式裸数据
 *  2、转成特定编码格式输出
 *  3、转成特定封装格式输出
 *  4、图片旋转
 *  5、图片缩放
 *  6、图片抠图，截取特定区域图像输出
 */

/**
 *  命令行说明：
 ffmpeg 将jpg转为yuv420p：
    ffmpeg -i xxx.jpg -s 1624x1236 -pix_fmt yuvj420p xxx.yuv
 
 */

namespace HBMedia {

typedef enum PIC_MEDIA_DATA_TYPE {
    PIC_D_TYPE_UNKNOWN = 0,
    PIC_D_TYPE_RAW_YUV = 1,
    PIC_D_TYPE_RAW_RGB = 2,
    PIC_D_TYPE_COMPRESS = 3,
} PIC_MEDIA_DATA_TYPE;
    
typedef class CSPicture
{
public:
    CSPicture();
    ~CSPicture();
    
    int setSrcPictureParam(PictureParams* param);
    int setTargetPictureParam(PictureParams* param);

    int setSrcPicDataType(PIC_MEDIA_DATA_TYPE type);
    int setTargetPicDataType(PIC_MEDIA_DATA_TYPE type);
    
    /**
     *  配置输入的音频文件
     */
    void setInputPicMediaFile(char *file);
    char *getInputPicMediaFile();
    
    /**
     *  配置输出的音频文件
     */
    void setOutputPicMediaFile(char *file);
    char *getOutputPicMediaFile();
    
    /**
     *  对图像进行编解码操作，都要调用该接口进行初始化
     */
    int  picCommonInitial();
    
    /** ================================================= >>> Encode **/
    /**
     *  图像编码器初始化、启动、关闭、释放
     */
    int  picEncoderInitial();
    int  picEncoderOpen();
    int  picEncoderClose();
    int  picEncoderRelease();
    
    /**
     *  获取图片像素数据，返回对应的数据缓冲区以及像素大小
     */
    int  getPicRawData(uint8_t** pData, int* dataSizes);
    
    /**
     *  对图片像素进行编码
     */
    int  pictureEncode(uint8_t* pData, int dataSizes);
    
    /**
     *  刷新编码器，清空编码器种的缓存
     */
    int  pictureFlushEncode();
    /** ================================================= <<< **/

    int  pictureSwscale(uint8_t** pData, int* dataSizes, PictureParams* srcParam, PictureParams* dstParam);
    
    /** ================================================= >>> Decode **/
    /**
     *  图像解码器初始化、启动、关闭、释放
     */
    int  picDecoderInitial();
    int  picDecoderOpen();
    int  picDecoderClose();
    int  picDecoderRelease();
    
    /** ================================================= <<< **/
    
private:
    
protected:
    int _checkPicMediaValid();
    
private:
    FILE*  mSrcFileHandle;
    PIC_MEDIA_DATA_TYPE mSrcPicDataType;
    PictureParams mSrcPicParam;
    
    FILE*  mTargetFileHandle;
    PIC_MEDIA_DATA_TYPE mTargetPicDataType;
    PictureParams mTargetPicParam;

    AVFormatContext* mOutputPicFormat;
    AVStream*  mOutputPicStream;
    AVCodecContext* mOutputPicCodecCtx;
    AVCodec* mOutputPicCodec;
    
    char *mInputPicMediaFile;
    char *mOutputPicMediaFile;

} CSPicture;
    
} /** HBMedia */


#endif /* CSPicture_hpp */
