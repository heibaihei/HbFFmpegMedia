#ifndef _H_FormatCvtFORARGBTOJ_H_
#define _H_FormatCvtFORARGBTOJ_H_
#include "FormatCvt_row_define.h"

//BGR 格式顺序在FormatCvt_row_define.h中定义，默认为BGRA

class FormatCvt{
	//J422ToARGB
	
	public:
		//-------------------------------------------ARGB TO J/J TO ARGB-------------------------------------------
 		
 		static int ARGBToJ420(const uint8* src_argb, int src_stride_argb,
               uint8* dst_yj, int dst_stride_yj,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height);

		static int J420ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);
    
    	     static int ARGBToJ422(const uint8* src_argb, int src_stride_argb,
               uint8* dst_yj, int dst_stride_yj,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height);

    	
		static int J422ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);
    
		
 
		
	//---------------------------------------------ARGB TO I/I TO ARGB-----------------------------------
		static int ARGBToI422(const uint8* src_argb, int src_stride_argb,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height);

		static int ARGBToI420(const uint8* src_argb, int src_stride_argb,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height);

		static int I422ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);

		static int I420ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);
		//----------------------------------------ARGB TO NV12//NV12 TO ARGB---------------------------------------
		static int ARGBToNV12(const uint8* src_argb, int src_stride_argb,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_uv, int dst_stride_uv,
               int width, int height);

		static int NV12ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_uv, int src_stride_uv,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);
		//--------------------------------------ARGB TO NV21/NV21 TO ARGB-----------------------------------------
		static int NV21ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_uv, int src_stride_uv,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);

		static int ARGBToNV21(const uint8* src_argb, int src_stride_argb,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_uv, int dst_stride_uv,
               int width, int height);
		//---------------------------------------ARGB TO YUYJ/YUYJ TO ARGB-----------------------------------------
		static int ARGBToYUY2J(const uint8* src_argb, int src_stride_argb,
               uint8* dst_yuy2, int dst_stride_yuy2,
               int width, int height);
		static int YUY2JToARGB(const uint8* src_yuy2, int src_stride_yuy2,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height);


		static int YUVFullToYUY2(const uint8* src_yuv444, int src_stride_yuv444,
			uint8* dst_yuy2, int dst_stride_yuy2,
			int width, int height);

		static int YUY2ToYUVFull(const uint8* src_yuy2, int src_stride_yuy2,
			uint8* dst_yuv444, int dst_stride_yuv444,
			int width, int height);

		static int I420ToNV12(const uint8* src_y,int src_stride_y,const uint8* src_u,int src_stride_u,
			const uint8* src_v,int src_stride_v,uint8* dst_y,int dst_stride_y,
			uint8* dst_uv,int dst_stride_uv,int width,int height);
		static int I420ToNV21(const uint8* src_y,int src_stride_y,const uint8* src_u,int src_stride_u,
			const uint8* src_v,int src_stride_v,uint8* dst_y,int dst_stride_y,
			uint8* dst_uv,int dst_stride_uv,int width,int height);
};
#endif