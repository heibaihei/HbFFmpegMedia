//
//  DebugUtils.h
//  JNIMedia
//
//  Created by Javan on 15/7/1.
//  Copyright (c) 2015年 Javan. All rights reserved.
//

#ifndef __JNIMedia__DebugUtils__
#define __JNIMedia__DebugUtils__

#include <stdio.h>



class DebugUtils {
    
public:
    /**
     *  把YUV420sp数据保存成PPM方便调试
     *
     *  @param filename 保存的文件路径
     *  @param yuv      yuv的数据指针
     *  @param w        输入数据的宽度
     *  @param h        输入数据的高度
     */
    static void writeYUVPPMFile(const char *filename, unsigned char const*yuv, int w, int h);
    
    /**
     *  YUV420的数据保存成PPM的文件
     *
     *  @param filename 保存的文件名
     *  @param yuv      yuv的数据指针
     *  @param w        输入数据的宽度
     *  @param h        输入数据的高度
     */
    static void writeI420PPMFile(const char *filename, unsigned char const*yuv, int w, int h);
    
private:
    DebugUtils(){};
    virtual ~DebugUtils(){};
};


#endif /* defined(__JNIMedia__DebugUtils__) */
