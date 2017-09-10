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

typedef class CSTimeline {
public:
    CSTimeline();
    ~CSTimeline();
    
    /*
     * @func 打开文件.
     * @arg filename 文件名
     * @return 0 为正常, 其他为异常
     */
    int open(const char *filename);
    
    void setOutputFile(char *file);
    char *getOutputFile() { return mSaveFilePath; };
protected:
    
private:
    /** 保存模式下使用到的参数: */
    /** 输出文件媒体格式 */
    AVFormatContext *mFmtCtx;
    char            *mSaveFilePath;
    /** 输出文件中的媒体流信息 */
    std::vector<CSIStream *> mStreamsList;
    
    /** 预览模式下使用到的参数： */
    
    /** 公共参数： */
    
} CSTimeline;
    
}

#endif /* CSTimeline_h */
