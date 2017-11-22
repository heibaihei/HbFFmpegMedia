//
//  CSVideoReverse.hpp
//  Sample
//
//  Created by zj-db0519 on 2017/11/21.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSVideoReverse_h
#define CSVideoReverse_h

#include <stdio.h>
#include <vector>
#include "CSVideo.h"

typedef struct FrameInfo{
    AVFrame *pMediaFrame;
    enum AVMediaType MediaType;
    int StreamIndex;
} FrameInfo;

class CSVideoFilter {
public:
    CSVideoFilter();
    ~CSVideoFilter();
    
    /**
     *  配置输入的音频文件
     */
    void setInputVideoMediaFile(char *file);
    char *getInputVideoMediaFile();
    
    /**
     *  配置输出的音频文件
     */
    void setOutputVideoMediaFile(char *file);
    char *getOutputVideoMediaFile();
    
    int prepare();
    
protected:
    /** 收集关键帧 pts 时间 */
    int _collectKeyFramePts();
    /** 进行反转 */
    int _doReverse();
    
    int _partReverse(int streamIndex, int64_t StartTime, int64_t EndTime);
    
    int _prepareInMedia();
    
    int insertFrameQueue(std::vector<FrameInfo *> &frameQueue, AVFrame *pFrame, int mediaStreamIndex, enum AVMediaType mediaType);

private:
    
    /** 待定 */
    int64_t mEndVideoPts;
    
    AVFormatContext *mInFmtCtx;
    AVStream *mInVideoStream;
    AVCodecContext *mInVideoCodecCtx;
    
    std::vector<KeyFramePts *> mKeyFramePtsVector;
    double mStartTime, mEndTime;
    
    char *mVideoOutputFile;
    FILE *mVideoOutputFileHandle;
    
    char *mVideoInputFile;
    FILE *mVideoInputFileHandle;
} CSVideoReverse;

#endif /* CSVideoReverse_h */
