//
//  MTVideoTransToGif.h
//  Sample
//
//  Created by zj-db0519 on 2017/11/2.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef MTVideoTransToGif_h
#define MTVideoTransToGif_h

#include <stdio.h>
#include "LogHelper.h"
#include "CSLog.h"
#include "CSDefine.h"
#include "CSCommon.h"

typedef struct SwsContext SwsContext;

namespace FormatConvert {

#define INVALID_STREAM_INDEX  (-1)
typedef struct MediaCoder {
    /** 解码器相关信息 */
    AVFormatContext* mPVideoFormatCtx;
    AVCodecContext* mPVideoCodecCtx;
    AVCodec* mPVideoCodec;
    int mVideoStreamIndex;
} MediaCoder;

MediaCoder* AllocMediaCoder();
void ImageParamsInitial(ImageParams *pParams);
    
typedef class VideoFormatTranser {
public:
    VideoFormatTranser();
    ~VideoFormatTranser();
    
    /**
     *  配置输入输出视频文件
     */
    void setInputVideoMediaFile(char *pFilePath);
    void setOutputVideoMediaFile(char *pFilePath);
    
    /** 设置输出的视频宽高 */
    void setVideoOutputSize(int width, int height);
    /** 设置输出码率 */
    void setVideoOutputBitrate(int64_t bitrate);
    /** 设置输出帧率 */
    void setVideoOutputFrameRate(float frameRate);
    
    /**
     *  准备工作
     */
    int prepare();
    /**
     *  执行转换
     */
    int doConvert();
    
    /**
     *  释放空间
     */
    int release();
    
protected:
    /** 内部接口 */
    /** 输入 & 输出媒体初始化 */
    int _InputMediaInitial();
    int _OutputMediaInitial();
    
    /** 格式转换初始化 */
    int _SwsMediaInitial();
    
    /**
     *  内部转码接口
     *  @return 0 转码成功; -1 未得到转码帧；-2 发生转码异常;
     *      如果转码成功，则内部会释放外部传入的packet 内存，将转入的报文指向转码后的 packet;
     *      如果转码失败，则需要外部释放传入的 packet 内存空间；
     */
    int _TransMedia(AVPacket** pInPacket);
    
    /**
     *  内部图像转换接口
     *  @return 0 转码成功; -1 表示转码失败;
     *      如果转码成功，则内部会释放外部传入的 frame 内存，将转入的报文指向转码后的 frame;
     *      如果转码失败，则需要外部释放传入的 frame 内存空间；
     */
    int _ImageConvert(AVFrame** pInFrame);
    
private:
    /** 输入相关解码器媒体信息 */
    MediaCoder *mPMediaDecoder;
    MediaCoder *mPMediaEncoder;
    SwsContext *mPVideoConvertCtx;
    
    /** 输出以及输出图像参数 */
    ImageParams mInputImageParams;
    ImageParams mOutputImageParams;
    char       *mInputMediaFile;
    char       *mOutputMediaFile;
    uint32_t   mState;
} VideoFormatTranser;

}

#endif /* MTVideoTransToGif_h */
