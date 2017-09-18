/*******************************
 **FileName：			convert_yuv.h
 **Creator：			yzh
 **Date：				2013-12-31
 **Modifier：			yzh
 **ModifyDate：		2013-12-31
 **Feature：			RGB/YUV颜色空间转换
 **Version：			1.0.0
 ********************************/
#ifndef _INCLUDE_MTXX_CONVERT_YUV_H_
#define _INCLUDE_MTXX_CONVERT_YUV_H_

#ifdef __cplusplus
extern "C" {
#endif
#ifndef byte
    typedef unsigned char byte;
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
    //////////////////////////////////////////////////////////////////////////
    /*			--!!Warning!!--			   */
    /*--！！修改成自己平台的ARGB字节序！！--*/
    /*--！！这份代码neon版只支持RGBA字节序！！--*/
    //////////////////////////////////////////////////////////////////////////
#ifndef MT_RED
#define	MT_BLUE   2
#define	MT_GREEN  1
#define MT_RED    0
#define MT_ALPHA  3
#endif
    /*******************************
     **FunctionName：				ARGBToNV21
     **@Param src_argb:			ARGB数据流
     **@Param src_stride_argb：	ARGB每行数据宽度
     **@Param dst_y：				NV21 Y 通道数据流
     **@Param dst_stride_y：		NV21 Y 通道每行数据宽度
     **@Param dst_uv：			NV21 UV 通道数据流
     **@Param dst_stride_uv：		NV21 UV 通道每行数据宽度
     **@Param width：				图像宽
     **@Param height：			图像高
     **Feature：					ARGB转NV21
     **return：					< 0 ?失败 :成功
     *******************************/
    int ARGBToNV21(const uint8* src_argb, int src_stride_argb,
                   uint8* dst_y, int dst_stride_y,
                   uint8* dst_uv, int dst_stride_uv,
                   int width, int height);
    /*******************************
     **FunctionName：				NV21ToARGB
     **@Param src_y：				NV21 Y 通道数据流
     **@Param src_stride_y：		NV21 Y 通道每行数据宽度
     **@Param src_uv：			NV21 UV 通道数据流
     **@Param src_stride_uv：		NV21 UV 通道每行数据宽度
     **@Param dst_argb:			ARGB数据流
     **@Param dst_stride_argb：	ARGB每行数据宽度
     **@Param width：				图像宽
     **@Param height：			图像高
     **Feature：					NV21转ARGB
     **return：					< 0 ?失败 :成功
     *******************************/
    int NV21ToARGB(const uint8* src_y, int src_stride_y,
                   const uint8* src_uv, int src_stride_uv,
                   uint8* dst_argb, int dst_stride_argb,
                   int width, int height);
    /*******************************
     **FunctionName：				ARGBToYUY2
     **@Param src_argb：			ARGB数据流
     **@Param src_stride_argb：	ARGB每行数据宽度
     **@Param dst_yuy2：			YUY2数据流
     **@Param dst_stride_yuy2：	YUY2每行数据宽度
     **@Param width：				图像宽
     **@Param height：			图像高
     **Feature：					ARGB转YUY2
     **return：					< 0 ?失败 :成功
     *******************************/
    int ARGBToYUY2(const uint8* src_argb, int src_stride_argb,
                   uint8* dst_yuy2, int dst_stride_yuy2,
                   int width, int height);
    /*******************************
     **FunctionName：				YUY2ToARGB
     **@Param src_yuy2：			YUY2数据流
     **@Param src_stride_yuy2：	YUY2每行数据宽度
     **@Param dst_argb：			ARGB数据流
     **@Param dst_stride_argb：	ARGB每行数据宽度
     **@Param width：				图像宽
     **@Param height：			图像高
     **Feature：					YUY2转ARGB
     **return：					< 0 ?失败 :成功
     *******************************/
    int YUY2ToARGB(const uint8* src_yuy2, int src_stride_yuy2,
                   uint8* dst_argb, int dst_stride_argb,
                   int width, int height);
    /*******************************
     **FunctionName：				ARGBToI420
     **@Param src_argb：			ARGB数据流
     **@Param src_stride_argb：	ARGB每行数据宽度
     **@Param dst_y：				I420 Y 通道数据流
     **@Param dst_stride_y：		I420 Y 通道每行数据宽度
     **@Param dst_u：				I420 U 通道数据流
     **@Param dst_stride_u：		I420 U 通道每行数据宽度
     **@Param dst_v：				I420 V 通道数据流
     **@Param dst_stride_v：		I420 V 通道每行数据宽度
     **@Param width：				图像宽
     **@Param height：			图像高
     **Feature：					ARGB转I420
     **return：					< 0 ?失败 :成功
     *******************************/
    int ARGBToI420(const uint8* src_argb, int src_stride_argb,
                   uint8* dst_y, int dst_stride_y,
                   uint8* dst_u, int dst_stride_u,
                   uint8* dst_v, int dst_stride_v,
                   int width, int height);
    /*******************************
     **FunctionName：				I420ToARGB
     **@Param src_y：				I420 Y 通道数据流
     **@Param src_stride_y：		I420 Y 通道每行数据宽度
     **@Param src_u：				I420 U 通道数据流
     **@Param src_stride_u：		I420 U 通道每行数据宽度
     **@Param src_v：				I420 V 通道数据流
     **@Param src_stride_v：		I420 V 通道每行数据宽度
     **@Param dst_argb：			ARGB数据流
     **@Param dst_stride_argb：	ARGB每行数据宽度
     **@Param width：				图像宽
     **@Param height：			图像高
     **Feature：					I420转ARGB
     **return：					< 0 ?失败 :成功
     *******************************/
    int I420ToARGB(const uint8* src_y, int src_stride_y,
                   const uint8* src_u, int src_stride_u,
                   const uint8* src_v, int src_stride_v,
                   uint8* dst_argb, int dst_stride_argb,
                   int width, int height);
    /*******************************
     **FunctionName：				YUY2ToI422
     **@Param src_yuy2：			YUY2数据流
     **@Param src_stride_yuy2：	ARGB每行数据宽度
     **@Param dst_y：				I422 Y 通道数据流
     **@Param dst_stride_y：		I422 Y 通道每行数据宽度
     **@Param dst_u：				I422 U 通道数据流
     **@Param dst_stride_u：		I422 U 通道每行数据宽度
     **@Param dst_v：				I422 V 通道数据流
     **@Param dst_stride_v：		I422 V 通道每行数据宽度
     **@Param width：				图像宽
     **@Param height：			图像高
     **Feature：					YUY2转I422
     **return：					< 0 ?失败 :成功
     *******************************/
    int YUY2ToI422(const uint8* src_yuy2, int src_stride_yuy2,
                   uint8* dst_y, int dst_stride_y,
                   uint8* dst_u, int dst_stride_u,
                   uint8* dst_v, int dst_stride_v,
                   int width, int height);
    /*******************************
     **FunctionName：				I422ToYUY2
     **@Param src_y：				I422 Y 通道数据流
     **@Param src_stride_y：		I422 Y 通道每行数据宽度
     **@Param src_u：				I422 U 通道数据流
     **@Param src_stride_u：		I422 U 通道每行数据宽度
     **@Param src_v：				I422 V 通道数据流
     **@Param src_stride_v：		I422 V 通道每行数据宽度
     **@Param src_yuy2：			YUY2数据流
     **@Param src_stride_yuy2：	ARGB每行数据宽度
     **@Param width：				图像宽
     **@Param height：			图像高
     **Feature：					I422转YUY2
     **return：					< 0 ?失败 :成功
     *******************************/
    int I422ToYUY2(const uint8* src_y, int src_stride_y,
                   const uint8* src_u, int src_stride_u,
                   const uint8* src_v, int src_stride_v,
                   uint8* dst_yuy2, int dst_stride_yuy2,
                   int width, int height);
    /*******************************
     **FunctionName：				I444ToYUY2
     **@Param src_y：				I444 Y 通道数据流
     **@Param src_stride_y：		I444 Y 通道每行数据宽度
     **@Param src_u：				I444 U 通道数据流
     **@Param src_stride_u：		I444 U 通道每行数据宽度
     **@Param src_v：				I444 V 通道数据流
     **@Param src_stride_v：		I444 V 通道每行数据宽度
     **@Param src_yuy2：			YUY2数据流
     **@Param src_stride_yuy2：	ARGB每行数据宽度
     **@Param width：				图像宽
     **@Param height：			图像高
     **Feature：					I444转YUY2
     **return：					< 0 ?失败 :成功
     *******************************/
    int I444ToYUY2(const uint8* src_y, int src_stride_y,
                   const uint8* src_u, int src_stride_u,
                   const uint8* src_v, int src_stride_v,
                   uint8* dst_yuy2, int dst_stride_yuy2,
                   int width, int height);
    /*******************************
     **FunctionName：				YUY2ToI444
     **@Param src_yuy2：			YUY2数据流
     **@Param src_stride_yuy2：	ARGB每行数据宽度
     **@Param dst_y：				I444 Y 通道数据流
     **@Param dst_stride_y：		I444 Y 通道每行数据宽度
     **@Param dst_u：				I444 U 通道数据流
     **@Param dst_stride_u：		I444 U 通道每行数据宽度
     **@Param dst_v：				I444 V 通道数据流
     **@Param dst_stride_v：		I444 V 通道每行数据宽度
     **@Param width：				图像宽
     **@Param height：			图像高
     **Feature：					YUY2转I444
     **return：					< 0 ?失败 :成功
     *******************************/
    int YUY2ToI444(const uint8* src_yuy2, int src_stride_yuy2,
                   uint8* dst_y, int dst_stride_y,
                   uint8* dst_u, int dst_stride_u,
                   uint8* dst_v, int dst_stride_v,
                   int width, int height);
    /*******************************
     **FunctionName：				ARGBToRGB565
     **@Param src_argb：			ARGB数据流
     **@Param src_stride_argb：	ARGB每行数据宽度
     **@Param dst_rgb565：		RGB565数据流
     **@Param dst_stride_rgb565：	RGB565每行数据宽度
     **@Param width：				图像宽
     **@Param height：			图像高
     **Feature：					ARGB转RGB565
     **return：					< 0 ?失败 :成功
     *******************************/
    int ARGBToRGB565(const uint8* src_argb, int src_stride_argb,
                     uint8* dst_rgb565, int dst_stride_rgb565,
                     int width, int height);
    /*******************************
     **FunctionName：				RGB565ToARGB
     **@Param src_rgb565：		RGB565数据流
     **@Param src_stride_rgb565：	RGB565每行数据宽度
     **@Param dst_argb：			ARGB数据流
     **@Param dst_stride_argb：	ARGB每行数据宽度
     **@Param width：				图像宽
     **@Param height：			图像高
     **Feature：					RGB565转ARGB
     **return：					< 0 ?失败 :成功
     *******************************/
    int RGB565ToARGB(const uint8* src_rgb565, int src_stride_rgb565,
                     uint8* dst_argb, int dst_stride_argb,
                     int width, int height);
    /*******************************
     **FunctionName：				ARGBToI444
     **@Param src_argb：			ARGB数据流
     **@Param src_stride_argb：	ARGB每行数据宽度
     **@Param dst_y：				I444 Y 通道数据流
     **@Param dst_stride_y：		I444 Y 通道每行数据宽度
     **@Param dst_u：				I444 U 通道数据流
     **@Param dst_stride_u：		I444 U 通道每行数据宽度
     **@Param dst_v：				I444 V 通道数据流
     **@Param dst_stride_v：		I444 V 通道每行数据宽度
     **@Param width：				图像宽
     **@Param height：			图像高
     **Feature：					ARGB转I444
     **return：					< 0 ?失败 :成功
     *******************************/
    int ARGBToI444(const uint8* src_argb, int src_stride_argb,
                   uint8* dst_y, int dst_stride_y,
                   uint8* dst_u, int dst_stride_u,
                   uint8* dst_v, int dst_stride_v,
                   int width, int height);
    /*******************************
     **FunctionName：				I444ToARGB
     **@Param src_y：				I444 Y 通道数据流
     **@Param src_stride_y：		I444 Y 通道每行数据宽度
     **@Param src_u：				I444 U 通道数据流
     **@Param src_stride_u：		I444 U 通道每行数据宽度
     **@Param src_v：				I444 V 通道数据流
     **@Param src_stride_v：		I444 V 通道每行数据宽度
     **@Param dst_argb：			ARGB数据流
     **@Param dst_stride_argb：	ARGB每行数据宽度
     **@Param width：				图像宽
     **@Param height：			图像高
     **Feature：					I444转ARGB
     **return：					< 0 ?失败 :成功
     *******************************/
    int I444ToARGB(const uint8* src_y, int src_stride_y,
                   const uint8* src_u, int src_stride_u,
                   const uint8* src_v, int src_stride_v,
                   uint8* dst_argb, int dst_stride_argb,
                   int width, int height);
    /*******************************
     **FunctionName：				ARGBToUYVY
     **@Param src_argb：			ARGB数据流
     **@Param src_stride_argb：	ARGB每行数据宽度
     **@Param dst_uyvy：			UYVY数据流
     **@Param dst_stride_uyvy：	UYVY每行数据宽度
     **@Param width：				图像宽
     **@Param height：			图像高
     **Feature：					ARGB转UYVY
     **return：					< 0 ?失败 :成功
     *******************************/
    int ARGBToUYVY(const uint8* src_argb, int src_stride_argb,
                   uint8* dst_uyvy, int dst_stride_uyvy,
                   int width, int height);
    /*******************************
     **FunctionName：				UYVYToARGB
     **@Param src_uyvy：			UYVY数据流
     **@Param src_stride_uyvy：	UYVY每行数据宽度
     **@Param dst_argb：			ARGB数据流
     **@Param dst_stride_argb：	ARGB每行数据宽度
     **@Param width：				图像宽
     **@Param height：			图像高
     **Feature：					UYVY转ARGB
     **return：					< 0 ?失败 :成功
     *******************************/
    int UYVYToARGB(const uint8* src_uyvy, int src_stride_uyvy,
                   uint8* dst_argb, int dst_stride_argb,
                   int width, int height);
    
#ifdef __cplusplus
}//extern "C" {
#endif
#endif //#ifndef _INCLUDE_MTXX_CONVERT_YUV_H_