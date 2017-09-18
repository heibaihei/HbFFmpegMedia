/*******************************
 **FileName：			rotate_yuv.h
 **Creator：			yzh
 **Date：				2014-01-01
 **Modifier：			yzh
 **ModifyDate：		2014-01-01
 **Feature：			YUV图像整90°旋转
 **Version：			1.0.0
 *******************************/
#ifndef _INCLUDE_MTXX_ROTATE_YUV_H_
#define _INCLUDE_MTXX_ROTATE_YUV_H_

#ifdef __cplusplus
extern "C" {
#endif
    
#ifndef ROTATATIONMODE_DECLARATION
#define ROTATATIONMODE_DECLARATION
    // Supported rotation.
    typedef enum RotationMode {
        kRotate0 = 0,  // No rotation.
        kRotate90 = 90,  // Rotate 90 degrees clockwise.
        kRotate180 = 180,  // Rotate 180 degrees.
        kRotate270 = 270,  // Rotate 270 degrees clockwise.
        
        // Deprecated.
        kRotateNone = 0,
        kRotateClockwise = 90,
        kRotateCounterClockwise = 270,
    }RotationMode;
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
    
    /*******************************
     **FunctionName：			RotatePlane180
     **@Param src:			源单通道数据流
     **@Param src_stride：	源单通道每行数据宽度
     **@Param dst：			目标单通道数据流
     **@Param dst_stride：	目标单通道每行数据宽度
     **@Param width：			图像宽
     **@Param height：		图像高
     **@Param mode：			旋转参数
     **Feature：				单通道图像旋转180°
     **return：				< 0 ?失败 :成功
     *******************************/
    void RotatePlane180(const uint8* src, int src_stride,
                        uint8* dst, int dst_stride,
                        int width, int height);
    /*******************************
     **FunctionName：			RotatePlane270
     **@Param src:			源单通道数据流
     **@Param src_stride：	源单通道每行数据宽度
     **@Param dst：			目标单通道数据流
     **@Param dst_stride：	目标单通道每行数据宽度
     **@Param width：			图像宽
     **@Param height：		图像高
     **@Param mode：			旋转参数
     **Feature：				单通道图像旋转270°
     **return：				< 0 ?失败 :成功
     *******************************/
    void RotatePlane270(const uint8* src, int src_stride,
                        uint8* dst, int dst_stride,
                        int width, int height);
    /*******************************
     **FunctionName：			RotatePlane90
     **@Param src:			源单通道数据流
     **@Param src_stride：	源单通道每行数据宽度
     **@Param dst：			目标单通道数据流
     **@Param dst_stride：	目标单通道每行数据宽度
     **@Param width：			图像宽
     **@Param height：		图像高
     **@Param mode：			旋转参数
     **Feature：				单通道图像旋转90°
     **return：				< 0 ?失败 :成功
     *******************************/
    void RotatePlane90(const uint8* src, int src_stride,
                       uint8* dst, int dst_stride,
                       int width, int height);
    /*******************************
     **FunctionName：			I420Rotate
     **@Param src_y:			源I420 Y 通道数据流
     **@Param src_stride_y：	源I420 Y 通道每行数据宽度
     **@Param src_u：			源I420 U 通道数据流
     **@Param src_stride_u：	源I420 U 通道每行数据宽度
     **@Param src_v：			源I420 V 通道数据流
     **@Param src_stride_v：	源I420 V 通道每行数据宽度
     **@Param dst_y:			目标I420 Y 通道数据流
     **@Param dst_stride_y：	目标I420 Y 通道每行数据宽度
     **@Param dst_u：			目标I420 U 通道数据流
     **@Param dst_stride_u：	目标I420 U 通道每行数据宽度
     **@Param dst_v：			目标I420 V 通道数据流
     **@Param dst_stride_v：	目标I420 V 通道每行数据宽度
     **@Param width：			图像宽
     **@Param height：		图像高
     **@Param mode：			旋转参数
     **Feature：				I420图像整90°旋转
     **return：				< 0 ?失败 :成功
     *******************************/
    int I420Rotate(const uint8* src_y, int src_stride_y,
                   const uint8* src_u, int src_stride_u,
                   const uint8* src_v, int src_stride_v,
                   uint8* dst_y, int dst_stride_y,
                   uint8* dst_u, int dst_stride_u,
                   uint8* dst_v, int dst_stride_v,
                   int width, int height,
                   RotationMode mode);
    
#ifdef __cplusplus
}//extern "C" {
#endif
#endif//#ifndef _INCLUDE_MTXX_ROTATE_YUV_H_