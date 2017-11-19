//
//  CSTimeline.hpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/10.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSTimeline_h
#define CSTimeline_h

#include <stdio.h>
#include <vector>
#include "CSDefine.h"
#include "CSLog.h"
#include "CSDefine.h"
#include "CSCommon.h"
#include "CSUtil.h"

namespace HBMedia {

/** 类前置生命处 */
class CSIStream;
class ThreadContext;
class CSWorkContext;

typedef class CSTimeline {
    
    friend class CSPlayer;
    
public:
    CSTimeline();
    ~CSTimeline();
    
    int prepare(void);
    
    int start(void);
    
    int stop(void);
    
    /**
     *  @func setOutputFile & getOutputFile 
     *        设置以及获取输出文件 URL 信息
     */
    void setOutputFile(char *file);
    char *getOutputFile() { return mSaveFilePath; };
    
    /**
     * 获取源图像参数信息
     */
    void setSrcImageParam(ImageParams* pParam) { mSrcImageParams = *pParam; }
    ImageParams* getSrcImageParam() { return &mSrcImageParams; }
    void setDstImageParam(ImageParams* pParam) { mTgtImageParams = *pParam; }
    ImageParams* getDstImageParam() { return &mTgtImageParams; }
    
    /**
     * 获取目标音频参数信息
     */
    void setTgtAudioParam(AudioParams* pParam) { mTgtAudioParams = *pParam; }
    AudioParams* getTgtAudioParam() { return &mTgtAudioParams; }
    void setSrcAudioParam(AudioParams* pParam) { mSrcAudioParams = *pParam; }
    AudioParams* getsrcAudioParam() { return &mSrcAudioParams; }
    
    void setCropParam(int posX, int posY, int cropWidth, int cropHeight);
    
    /** 设置全局播放速度 */
    void setGlobalSpeed(float speed);
    
    /** 测试接口 */
    /**
     *   往 Timeline 中特定流发送裸数据
     *
     */
    int  sendRawData(uint8_t* pData, long DataSize, int StreamIdex, int64_t TimeStamp);
    
    /**
     *  释放 parepare 期间创建的资源；
     */
    int  release(void);
    
protected:
    /** 保存模式下使用到的参数: */
    
    /**
     * @func writeHeader & writeTailer 写文件头 和 文件尾.
     * @return HB_OK 为正常, HB_ERROR 为异常
     */
    int writeHeader();
    int writeTailer();
    
    /** 预览模式下使用到的参数： */
    
    /** 公共接口 */
    
    /**
     *  初始化对象
     */
    int initial();
    
    /**
     * @func addStream 往输出对象中添加流对象
     * @param pNewStream 指向待添加的流对象
     * @return HB_OK 为正常, HB_ERROR 为异常
     */
    int _addStream(CSIStream* pNewStream);

    /** 准备输出媒体资源 */
    int _prepareOutMedia();
    
    /**
     * @func open 打开输出媒体文件
     * @return HB_OK 为正常, HB_ERROR 为异常
     */
    int _ConstructOutputMedia();
private:
    /** 保存模式下使用到的参数: */
    /** 输出文件媒体格式, 在 Prepare 时对该成员进行初始化 */
    AVFormatContext *mFmtCtx;
    char            *mSaveFilePath;
    /** 输出文件中的媒体流信息 */
    std::vector<CSIStream *> mStreamsList;
    
    float mGlobalSpeed;

    /** 公共参数： */
    CSWorkContext* mWorkCtx;
    AudioParams    mSrcAudioParams;
    ImageParams    mSrcImageParams;
    AudioParams    mTgtAudioParams;
    ImageParams    mTgtImageParams;
    CropParam      mCropParms;
} CSTimeline;
    
}

#endif /* CSTimeline_h */
