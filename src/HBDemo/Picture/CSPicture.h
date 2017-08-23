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
     *  图片基础组建初始化，比如ffmpeg、等基础组件初始化
     *  [备注] 不管进行怎么样类型的转换，这个都是必须要被调用的，
     *  内部会对即将进行编解码的参数进行校验以及基础组件的初始化
     *  @return HB_ERROR 初始化异常; HB_OK 正常初始化
     */
    int  picBaseInitial();
    
    /** ================================================= >>> Encode **/
    /** 图像编码器初始化、启动、关闭、释放 **/
    /**
     *   图像编码初始化, 主要对编码需要用到的一些内部对象创建空间等必要的初始化操作
     *   @return HB_ERROR 初始化异常; HB_OK 正常初始化
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
    /**
     *  检查当前 CSPicture 对象中相关属性是否支持进行编解码操作，相关参数是否有效
     *  @return HB_ERROR 编解码参数异常; HB_OK 编解码参数正常
     */
    int _checkPicMediaValid();
    
private:
    /**
     *  媒体数据输入相关信息
     */
    char            *mSrcPicMediaFile;
    FILE            *mSrcPicFileHandle;
    /** 输入数据类型，裸数据还是压缩数据 */
    PIC_MEDIA_DATA_TYPE mSrcPicDataType;
    PictureParams    mSrcPicParam;

    /**
     *  媒体数据输出相关信息
     */
    char            *mTargetPicMediaFile;
    FILE            *mTargetPicFileHandle;
    PIC_MEDIA_DATA_TYPE mTargetPicDataType;
    PictureParams    mTargetPicParam;
    AVCodec         *mOutputPicCodec;
    AVCodecContext  *mOutputPicCodecCtx;
    AVStream        *mOutputPicStream;
    AVFormatContext *mOutputPicFormat;

} CSPicture;

} /** HBMedia */

#endif /* CSPicture_hpp */
