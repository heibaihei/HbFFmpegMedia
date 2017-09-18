//
//  ThumbGenerater.h
//  sdl-ffplay
//
//  Created by Javan on 14-5-26.
//  Copyright (c) 2014年 Javan. All rights reserved.
//

#ifndef __sdl_ffplay__ThumbGenerater__
#define __sdl_ffplay__ThumbGenerater__

/**
 *  生成缩略图的函数
 *
 *  @param src  需要生成缩略图文件的路径
 *  @param save_path 保存的路径
 *  @param times     时间数组
 *  @param length    时间数组长度
 *
 *  @return 返回实际生成缩略图的长度, 负数表示错误
 */
int GenerateThumb(const char *src, const char* save_path, double *times, int length);

#endif /* defined(__sdl_ffplay__ThumbGenerater__) */
