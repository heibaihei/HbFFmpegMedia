/*******************************
 **FileName：			scale_yuv.h
 **Creator：			yzh
 **Date：				2013-12-31
 **Modifier：			yzh
 **ModifyDate：		2013-12-31
 **Feature：			YUV图片缩放
 **Version：			1.0.0
 *******************************/


#ifndef _INCLUDE_MTXX_SCALE_YUV_H_
#define _INCLUDE_MTXX_SCALE_YUV_H_



#ifdef __cplusplus
extern "C" {
#endif
#ifndef uint8
    typedef unsigned char uint8;
#endif
    
#ifndef uint16
    typedef unsigned short uint16;
#endif
    
#ifndef uint32
    typedef unsigned int uint32;
#endif
    
#ifndef int
#define int int
#endif
    
#ifndef FilterMode
    // Supported filtering.
    typedef enum FilterMode {
        kFilterNone = 0,  // Point sample; Fastest.
        kFilterLinear = 1,  // Filter horizontally only.
        kFilterBilinear = 2,  // Faster than box, but lower quality scaling down.
        kFilterBox = 3  // Highest quality.
    }FilterMode;
#endif
    /*******************************
     **FunctionName：			I420Scale
     **@Param src_y:			源I420 Y 通道数据流
     **@Param src_stride_y：	源I420 Y 通道每行数据宽度
     **@Param src_u：			源I420 U 通道数据流
     **@Param src_stride_u：	源I420 U 通道每行数据宽度
     **@Param src_v：			源I420 V 通道数据流
     **@Param src_stride_v：	源I420 V 通道每行数据宽度
     **@Param src_width：		源YUV图像宽
     **@Param src_height：	源YUV图像高
     **@Param dst_y：			目标I420 Y 通道数据流
     **@Param dst_stride_y：	目标I420 Y 通道每行数据宽度
     **@Param dst_u：			目标I420 U 通道数据流
     **@Param dst_stride_u：	目标I420 U 通道每行数据宽度
     **@Param dst_v：			目标I420 V 通道数据流
     **@Param dst_stride_v：	目标I420 V 通道每行数据宽度
     **@Param dst_width：		目标YUV图像宽
     **@Param dst_height：	目标YUV图像高
     **@Param filtering :	缩放算法
     **Feature：				I420图像缩放
     **return：				< 0 ?失败 :成功
     *******************************/
    int I420Scale(const uint8* src_y, int src_stride_y,
                  const uint8* src_u, int src_stride_u,
                  const uint8* src_v, int src_stride_v,
                  int src_width, int src_height,
                  uint8* dst_y, int dst_stride_y,
                  uint8* dst_u, int dst_stride_u,
                  uint8* dst_v, int dst_stride_v,
                  int dst_width, int dst_height,
                  FilterMode filtering);
    /*******************************
     **FunctionName：			ScalePlane
     **@Param src:			源单通道数据流
     **@Param src_stride：	源单通道每行数据宽度
     **@Param src_width：		源单通道宽
     **@Param src_height：	源单通道高
     **@Param dst：			目标单通道数据流
     **@Param dst_stride：	目标单通道每行数据宽度
     **@Param dst_width：		目标单通道宽
     **@Param dst_height：	目标单通道高
     **@Param filtering：		缩放算法
     **Feature：				单通道图像(mask图)缩放
     **return：				< 0 ?失败 :成功
     *******************************/
    void ScalePlane(const uint8* src, int src_stride,
                    int src_width, int src_height,
                    uint8* dst, int dst_stride,
                    int dst_width, int dst_height,
                    FilterMode filtering);
    
    /*******************************
     **FunctionName：				YUY2Scale
     **@Param src_yuy2:			源YUY2数据流
     **@Param src_stride_yuy2：	源YUY2每行数据宽度
     **@Param src_width：			源YUY2图像宽
     **@Param src_height：		源YUY2图像高
     **@Param dst_yuy2：			目标YUY2数据流
     **@Param dst_stride_yuy2：	目标YUY2每行数据宽度
     **@Param dst_width：			目标YUY2图像宽
     **@Param dst_height：		目标YUY2图像高
     **@Param filtering：			缩放算法
     **Feature：					单通道图像(mask图)缩放
     **return：					< 0 ?失败 :成功
     *******************************/
    int YUY2Scale(uint8* src_yuy2, int src_stride_yuy2,
                  int src_width,int src_height,	
                  uint8* dst_yuy2,int dst_stride_yuy2,
                  int dst_width,int dst_height,FilterMode filterMode);
    
    
#ifdef __cplusplus
}//extern "C" {
#endif
#endif//#ifndef _INCLUDE_MTXX_SCALE_YUV_H_