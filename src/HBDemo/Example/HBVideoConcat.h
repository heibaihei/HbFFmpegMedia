//
//  HBVideoConcat.hpp
//  Sample
//
//  Created by zj-db0519 on 2017/10/10.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef HBVideoConcat_h
#define HBVideoConcat_h

#include <stdio.h>
#include <stdlib.h>
/**
 视频拼接：
 
 方法一：FFmpeg concat 协议
     对于相同的 MPEG 格式的视频，可以直接连接, 部分格式无法使用该方式进行拼接，目前已知的是: ts|mpg：
       ffmpeg -i "concat:pieces1.mpg|pieces2.mpg|pieces3.mpg" -c copy pieces123.mpg
 

 格式转换： mp4文件转成同样编码形式的 ts 流，因为 ts流是可以 concate 的，先把 mp4 封装成 ts ，然后 concate ts 流， 最后再把 ts 流转化为 mp4。
     ffmpeg -i 100.mp4 -vcodec copy -acodec copy -vbsf h264_mp4toannexb pieces1.ts
     ffmpeg -i 100.mp4 -vcodec copy -acodec copy -vbsf h264_mp4toannexb pieces2.ts
     ffmpeg -i "concat:pieces1.ts|pieces2.ts" -acodec copy -vcodec copy -absf aac_adtstoasc output.mp4
 
 格式转换：
     mp4 转 flv : ffmpeg -i 100.mp4 -vcodec copy -acodec copy pieces1.flv
 */

int demo_video_concat_with_same_codec();

int demo_video_concat_with_diffence_codec();

#endif /* HBVideoConcat_h */
