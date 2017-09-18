/*******************************
 **FileName：			rotate_argb.h
 **Creator：			yzh
 **Date：				2014-01-01
 **Modifier：			yzh
 **ModifyDate：		2014-01-01
 **Feature：			RGB图像整90°旋转
 **Version：			1.0.0
 *******************************/

#ifndef _INCLUDE_MTXX_ROTATE_ARGB_H_
#define _INCLUDE_MTXX_ROTATE_ARGB_H_
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
    
    /*******************************
     **FunctionName：				ARGBRotate
     **@Param src_argb:			源ARGB数据流
     **@Param src_stride_argb：	源ARGB每行数据宽度
     **@Param dst_argb：			目标ARGB数据流
     **@Param dst_stride_argb：	目标ARGB每行数据宽度
     **@Param width：				图像宽
     **@Param height：			图像高
     **@Param mode：				旋转参数
     **Feature：					ARGB图像整90°旋转
     **return：					< 0 ?失败 :成功
     *******************************/
    int ARGBRotate(const uint8* src_argb, int src_stride_argb,
                   uint8* dst_argb, int dst_stride_argb,
                   int width, int height,
                   RotationMode mode);
    
#ifdef __cplusplus
}
#endif
#endif //#ifndef _INCLUDE_MTXX_ROTATE_ARGB_H_