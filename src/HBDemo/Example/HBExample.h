//
//  HBExample.h
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/19.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef HBExample_h
#define HBExample_h

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  展示 av_file_map 接口的使用，将文件与内存接口进行映射
 *  实现媒体数据的读取
 */
//int demo_avio_reading(int argc, const char *argv[]);
//
//int demo_decode_encode(int argc, char **argv);

int examples_filtering_video(int argc, char **argv);
int demo_avio_reading(int argc, const char *argv[]);
    
    int demo_transcod_main(int argc, char **argv);

#ifdef __cplusplus
};
#endif

#endif /* HBExample_h */
