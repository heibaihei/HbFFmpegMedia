//
//  HBPickPicture.hpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/7/6.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef HBPickPicture_hpp
#define HBPickPicture_hpp

#include <stdio.h>

/**
 *  相关功能描述
 *
 *  截取一张352x240尺寸大小的，格式为jpg的图片
        ffmpeg -i test.asf -y -f image2 -t 0.001 -s 352x240 a.jpg
 
 *  把视频的前３０帧转换成一个Animated Gif
        ffmpeg -i test.asf -vframes 30 -y -f gif a.gif
 
 */
#ifdef __cplusplus
extern "C" {
#endif

/**
 *  从视频流序列中处理帧数据，提供解封装、转码、等相关操作
 *  中途可插入而外操作。
 */
int HBPickImageFrameFromVideo();

int PictureCSpictureDemo();
    
#ifdef __cplusplus
};
#endif
    
#endif /* HBPickPicture_hpp */
