//
//  CSVideoAnalysis.hpp
//  Sample
//
//  Created by zj-db0519 on 2017/12/6.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSVideoAnalysis_h
#define CSVideoAnalysis_h

#include <stdio.h>
#include "CSVideoDecoder.h"

namespace HBMedia {

/** 结构功能解析:
 *  视频文件解析
 */
typedef class CSVideoAnalysis : public CSVideoDecoder
{
public:
    CSVideoAnalysis();
    
    ~CSVideoAnalysis();
    
    /**
     *  对外接口，分析输入的上一帧图像数据
     */
    void analysisFrame(AVFrame *pFrame, AVMediaType type);
    
protected:
    
    /**
     *  内部输出当前帧的一些信息
     */
    void EchoFrameInfo(AVFrame *pFrame, AVMediaType type);
    
private:
    
} CSVideoAnalysis;

}

#endif /* CSVideoAnalysis_h */
