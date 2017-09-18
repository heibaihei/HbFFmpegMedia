#include "FormatCvt.h"
#include "FormatCvt_row.h"
#include "FormatCvt_row_define.h"

static __inline int32 clamp0(int32 v) {
  return ((-(v) >> 31) & (v));
}

static __inline int32 clamp255(int32 v) {
  return (((255 - (v)) >> 31) | (v)) & 255;
}

static __inline uint32 Clamp(int32 val) {
  int v = clamp0(val);
  return (uint32)(clamp255(v));
}
// C reference code that mimics the YUV assembly.
#define YGJ 16320 /* round(1.000 * 64 * 256 * 256 / 257) */
#define YGBJ 32  /* 64 / 2 */

// U and V contributions to R,G,B.
#define UBJ -113 /* round(-1.77200 * 64) */
#define UGJ 22 /* round(0.34414 * 64) */
#define VGJ 46 /* round(0.71414  * 64) */
#define VRJ -90 /* round(-1.40200 * 64) */

// Bias values to subtract 16 from Y and 128 from U and V.
#define BBJ (UBJ * 128 + YGBJ)
#define BGJ (UGJ * 128 + VGJ * 128 + YGBJ)
#define BRJ (VRJ * 128 + YGBJ)

#define YG 18997 /* round(1.164 * 64 * 256 * 256 / 257) */
#define YGB -1160 /* 1.164 * 64 * -16 + 64 / 2 */

// U and V contributions to R,G,B.
#define UB -128 /* max(-128, round(-2.018 * 64)) */
#define UG 25 /* round(0.391 * 64) */
#define VG 52 /* round(0.813 * 64) */
#define VR -102 /* round(-1.596 * 64) */

// Bias values to subtract 16 from Y and 128 from U and V.
#define BB (UB * 128 + YGB)
#define BG (UG * 128 + VG * 128 + YGB)
#define BR (VR * 128 + YGB)

static __inline void YuvJPixel(uint8 y, uint8 u, uint8 v,
                               uint8* b, uint8* g, uint8* r) {
  uint32 y1 = (uint32)(y * 0x0101 * YGJ) >> 16;
  *b = Clamp((int32)(-(u * UBJ) + y1 + BBJ) >> 6);
  *g = Clamp((int32)(-(v * VGJ + u * UGJ) + y1 + BGJ) >> 6);
  *r = Clamp((int32)(-(v * VRJ) + y1 + BRJ) >> 6);
}

// C reference code that mimics the YUV assembly.
static __inline void YuvPixel(uint8 y, uint8 u, uint8 v,
                              uint8* b, uint8* g, uint8* r) {
  uint32 y1 = (uint32)(y * 0x0101 * YG) >> 16;
  *b = Clamp((int32)(-(u * UB) + y1 + BB) >> 6);
  *g = Clamp((int32)(-(v * VG + u * UG) + y1 + BG) >> 6);
  *r = Clamp((int32)(-(v * VR)+ y1 + BR) >> 6);
}

static __inline int RGBToYJ(uint8 r, uint8 g, uint8 b) {
  return (38 * r + 75 * g +  15 * b + 64) >> 7;
}

static __inline int RGBToUJ(uint8 r, uint8 g, uint8 b) {
  return (127 * b - 84 * g - 43 * r + 0x8080) >> 8;
}

static __inline int RGBToVJ(uint8 r, uint8 g, uint8 b) {
  return (127 * r - 107 * g - 20 * b + 0x8080) >> 8;
}

static __inline int RGBToY(uint8 r, uint8 g, uint8 b) {
  return (66 * r + 129 * g +  25 * b + 0x1080) >> 8;
}

static __inline int RGBToU(uint8 r, uint8 g, uint8 b) {
  return (112 * b - 74 * g - 38 * r + 0x8080) >> 8;
}
static __inline int RGBToV(uint8 r, uint8 g, uint8 b) {
  return (112 * r - 94 * g - 18 * b + 0x8080) >> 8;
}

void ARGBToUV422Row_C(const uint8* src_argb,
                      uint8* dst_u, uint8* dst_v, int width) {
      int x;
      for (x = 0; x < width - 1; x += 2) {
        uint8 ab = (src_argb[MT_BLUE] + src_argb[4+MT_BLUE]) >> 1;
        uint8 ag = (src_argb[MT_GREEN] + src_argb[4+MT_GREEN]) >> 1;
        uint8 ar = (src_argb[MT_RED] + src_argb[4+MT_RED]) >> 1;
        dst_u[0] = RGBToU(ar, ag, ab);
        dst_v[0] = RGBToV(ar, ag, ab);
        src_argb += 8;
        dst_u += 1;
        dst_v += 1;
      }
      if (width & 1) {
        uint8 ab = src_argb[MT_BLUE];
        uint8 ag = src_argb[MT_GREEN];
        uint8 ar = src_argb[MT_RED];
        dst_u[0] = RGBToU(ar, ag, ab);
        dst_v[0] = RGBToV(ar, ag, ab);
      }
  }


void ARGBToYRow_C(const uint8* src_argb0, uint8* dst_y, int width) {       
  int x;                                                                       
  for (x = 0; x < width; ++x) {                                                
    dst_y[0] = RGBToY(src_argb0[MT_RED], src_argb0[MT_GREEN], src_argb0[MT_BLUE]);               
    src_argb0 += 4;                                                          
    dst_y += 1;                                                                
  }                                                                            
}        

void ARGBToUVRow_C(const uint8* src_rgb0, int src_stride_rgb,              
                       uint8* dst_u, uint8* dst_v, int width) {                
  const uint8* src_rgb1 = src_rgb0 + src_stride_rgb;                           
  int x;                                                                       
  for (x = 0; x < width - 1; x += 2) {                                         
    uint8 ab = (src_rgb0[MT_BLUE] + src_rgb0[MT_BLUE + 4] +                              
               src_rgb1[MT_BLUE] + src_rgb1[MT_BLUE + 4]) >> 2;                          
    uint8 ag = (src_rgb0[MT_GREEN] + src_rgb0[MT_GREEN + 4] +                              
               src_rgb1[MT_GREEN] + src_rgb1[MT_GREEN+ 4]) >> 2;                          
    uint8 ar = (src_rgb0[MT_RED] + src_rgb0[MT_RED + 4] +                              
               src_rgb1[MT_RED] + src_rgb1[MT_RED+ 4]) >> 2;                          
    dst_u[0] = RGBToU(ar, ag, ab);                                             
    dst_v[0] = RGBToV(ar, ag, ab);                                             
    src_rgb0 += 4 * 2;                                                       
    src_rgb1 += 4 * 2;                                                       
    dst_u += 1;                                                                
    dst_v += 1;                                                                
  }                                                                            
  if (width & 1) {                                                             
    uint8 ab = (src_rgb0[MT_BLUE] + src_rgb1[MT_BLUE]) >> 1;                               
    uint8 ag = (src_rgb0[MT_GREEN] + src_rgb1[MT_GREEN]) >> 1;                               
    uint8 ar = (src_rgb0[MT_RED] + src_rgb1[MT_RED]) >> 1;                               
    dst_u[0] = RGBToU(ar, ag, ab);                                             
    dst_v[0] = RGBToV(ar, ag, ab);                                             
  }                                                                            
}



void I422ToARGBRow_C(const uint8* src_y,
                     const uint8* src_u,
                     const uint8* src_v,
                     uint8* rgb_buf,
                     int width) {
  int x;
  for (x = 0; x < width - 1; x += 2) {
    YuvPixel(src_y[0], src_u[0], src_v[0],
             rgb_buf + MT_BLUE, rgb_buf + MT_GREEN, rgb_buf + MT_RED);
    rgb_buf[MT_ALPHA] = 255;
    YuvPixel(src_y[1], src_u[0], src_v[0],
             rgb_buf + 4+MT_BLUE, rgb_buf +4+MT_GREEN, rgb_buf+4+MT_RED);
    rgb_buf[4+MT_ALPHA] = 255;
    src_y += 2;
    src_u += 1;
    src_v += 1;
    rgb_buf += 8;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel(src_y[0], src_u[0], src_v[0],
             rgb_buf +MT_BLUE, rgb_buf +MT_GREEN, rgb_buf + MT_RED);
    rgb_buf[MT_ALPHA] = 255;
  }
}

void MergeUVRow_C(const uint8* src_u, const uint8* src_v, uint8* dst_uv,
                  int width) {
  int x;
  for (x = 0; x < width - 1; x += 2) {
    dst_uv[0] = src_u[x];
    dst_uv[1] = src_v[x];
    dst_uv[2] = src_u[x + 1];
    dst_uv[3] = src_v[x + 1];
    dst_uv += 4;
  }
  if (width & 1) {
    dst_uv[0] = src_u[width - 1];
    dst_uv[1] = src_v[width - 1];
  }
}

void NV12ToARGBRow_C(const uint8* src_y,
                     const uint8* usrc_v,
                     uint8* rgb_buf,
                     int width) {
  int x;
  for (x = 0; x < width - 1; x += 2) {
    YuvPixel(src_y[0], usrc_v[0], usrc_v[1],
             rgb_buf +MT_BLUE, rgb_buf +MT_GREEN, rgb_buf +MT_RED);
    rgb_buf[MT_ALPHA] = 255;
    YuvPixel(src_y[1], usrc_v[0], usrc_v[1],
             rgb_buf +4+MT_BLUE, rgb_buf +4+MT_GREEN, rgb_buf +4+MT_RED);
    rgb_buf[4+MT_ALPHA] = 255;
    src_y += 2;
    usrc_v += 2;
    rgb_buf += 8;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel(src_y[0], usrc_v[0], usrc_v[1],
             rgb_buf +MT_BLUE, rgb_buf +MT_GREEN, rgb_buf + MT_RED);
    rgb_buf[MT_ALPHA] = 255;
  }
}


void NV21ToARGBRow_C(const uint8* src_y,
                     const uint8* src_vu,
                     uint8* rgb_buf,
                     int width) {
  int x;
  for (x = 0; x < width - 1; x += 2) {
    YuvPixel(src_y[0], src_vu[1], src_vu[0],
             rgb_buf + MT_BLUE, rgb_buf +MT_GREEN, rgb_buf + MT_RED);
    rgb_buf[MT_ALPHA] = 255;

    YuvPixel(src_y[1], src_vu[1], src_vu[0],
             rgb_buf + 4+MT_BLUE, rgb_buf + 4+MT_GREEN, rgb_buf + 4+MT_RED);
    rgb_buf[4+MT_ALPHA] = 255;

    src_y += 2;
    src_vu += 2;
    rgb_buf += 8;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel(src_y[0], src_vu[1], src_vu[0],
             rgb_buf + MT_BLUE, rgb_buf + MT_GREEN, rgb_buf + MT_RED);
    rgb_buf[MT_ALPHA] = 255;
  }
}


void ARGBToUVJ422Row_C(const uint8* src_argb,
                       uint8* dst_u, uint8* dst_v, int width) {
  int x;
  for (x = 0; x < width - 1; x += 2) {
    uint8 ab = (src_argb[MT_BLUE] + src_argb[4+MT_BLUE]) >> 1;
    uint8 ag = (src_argb[MT_GREEN] + src_argb[4+MT_GREEN]) >> 1;
    uint8 ar = (src_argb[MT_RED] + src_argb[4+MT_RED]) >> 1;
    dst_u[0] = RGBToUJ(ar, ag, ab);
    dst_v[0] = RGBToVJ(ar, ag, ab);
    src_argb += 8;
    dst_u += 1;
    dst_v += 1;
  }
  if (width & 1) {
    uint8 ab = src_argb[MT_BLUE];
    uint8 ag = src_argb[MT_GREEN];
    uint8 ar = src_argb[MT_RED];
    dst_u[0] = RGBToUJ(ar, ag, ab);
    dst_v[0] = RGBToVJ(ar, ag, ab);
  }
}
 void ARGBToYJRow_C(const uint8* src_argb0, uint8* dst_y, int width) {      
  int x;                                                                       
  for (x = 0; x < width; ++x) {                                                
    dst_y[0] = RGBToYJ(src_argb0[MT_RED], src_argb0[MT_GREEN], src_argb0[MT_BLUE]);              
    src_argb0 += 4;                                                          
    dst_y += 1;                                                                
  }                                                                            
}               

void ARGBToUVJRow_C(const uint8* src_rgb0, int src_stride_rgb,             
                        uint8* dst_u, uint8* dst_v, int width) {               
  const uint8* src_rgb1 = src_rgb0 + src_stride_rgb;                           
  int x;                                                                       
  for (x = 0; x < width - 1; x += 2) {                                         
    uint8 ab = AVGB(AVGB(src_rgb0[MT_BLUE], src_rgb1[MT_BLUE]),                            
                    AVGB(src_rgb0[MT_BLUE + 4], src_rgb1[MT_BLUE+ 4]));               
    uint8 ag = AVGB(AVGB(src_rgb0[MT_GREEN], src_rgb1[MT_GREEN]),                            
                    AVGB(src_rgb0[MT_GREEN + 4], src_rgb1[MT_GREEN + 4]));               
    uint8 ar = AVGB(AVGB(src_rgb0[MT_RED], src_rgb1[MT_RED]),                            
                    AVGB(src_rgb0[MT_RED+ 4], src_rgb1[MT_RED+ 4]));               
    dst_u[0] = RGBToUJ(ar, ag, ab);                                            
    dst_v[0] = RGBToVJ(ar, ag, ab);                                            
    src_rgb0 += 4 * 2;                                                       
    src_rgb1 += 4 * 2;                                                       
    dst_u += 1;                                                                
    dst_v += 1;                                                              
  }                                                                            
  if (width & 1) {                                                            
    uint8 ab = AVGB(src_rgb0[MT_BLUE], src_rgb1[MT_BLUE]);                                 
    uint8 ag = AVGB(src_rgb0[MT_GREEN], src_rgb1[MT_GREEN]);                                 
    uint8 ar = AVGB(src_rgb0[MT_RED], src_rgb1[MT_RED]);                                 
    dst_u[0] = RGBToUJ(ar, ag, ab);                                            
    dst_v[0] = RGBToVJ(ar, ag, ab);                                            
  }                                                                            
}
void J422ToARGBRow_C(const uint8* src_y,
                     const uint8* src_u,
                     const uint8* src_v,
                     uint8* rgb_buf,
                     int width) {
  int x;
  for (x = 0; x < width - 1; x += 2) {
    YuvJPixel(src_y[0], src_u[0], src_v[0],
              rgb_buf + MT_BLUE, rgb_buf + MT_GREEN, rgb_buf + MT_RED);
    rgb_buf[MT_ALPHA] = 255;
    YuvJPixel(src_y[1], src_u[0], src_v[0],
              rgb_buf + 4+MT_BLUE, rgb_buf + 4+MT_GREEN, rgb_buf + 4+MT_RED);
    rgb_buf[4+MT_ALPHA] = 255;
    src_y += 2;
    src_u += 1;
    src_v += 1;
    rgb_buf += 8;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvJPixel(src_y[0], src_u[0], src_v[0],
              rgb_buf + MT_BLUE, rgb_buf + MT_GREEN, rgb_buf + MT_RED);
    rgb_buf[MT_ALPHA] = 255;
  }
}


void YUY2JToARGBRow_C(const uint8* src_yuy2,
                     uint8* rgb_buf,
                     int width) {
  int x;
  for (x = 0; x < width - 1; x += 2) {
    YuvJPixel(src_yuy2[0], src_yuy2[1], src_yuy2[3],
             rgb_buf + MT_BLUE, rgb_buf + MT_GREEN, rgb_buf + MT_RED);
    rgb_buf[MT_ALPHA] = 255;
    YuvJPixel(src_yuy2[2], src_yuy2[1], src_yuy2[3],
             rgb_buf + 4+MT_BLUE, rgb_buf + 4+MT_GREEN, rgb_buf + 4+MT_RED);
    rgb_buf[4+MT_ALPHA] = 255;
    src_yuy2 += 4;
    rgb_buf += 8;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvJPixel(src_yuy2[0], src_yuy2[1], src_yuy2[3],
             rgb_buf + MT_BLUE, rgb_buf + MT_GREEN, rgb_buf + MT_RED);
    rgb_buf[MT_ALPHA] = 255;
  }
}


void I422ToYUY2JRow_C(const uint8* src_y,
                     const uint8* src_u,
                     const uint8* src_v,
                     uint8* dst_frame, int width) {
  int x;
  for (x = 0; x < width - 1; x += 2) {
    dst_frame[0] = src_y[0];
    dst_frame[1] = src_u[0];
    dst_frame[2] = src_y[1];
    dst_frame[3] = src_v[0];
    dst_frame += 4;
    src_y += 2;
    src_u += 1;
    src_v += 1;
  }
  if (width & 1) {
    dst_frame[0] = src_y[0];
    dst_frame[1] = src_u[0];
    dst_frame[2] = src_y[0];  // duplicate last y
    dst_frame[3] = src_v[0];
  }
}

void YUV444ToYUY2Row_C(const uint8* src_yuv444,uint8* dst_yuy2,int width) {
	// Output a row of UV values.
	int x;
	for (x = 0; x < width; x +=2) {
		dst_yuy2[0]=src_yuv444[0];
		dst_yuy2[2]=src_yuv444[3];
		dst_yuy2[1]=(src_yuv444[1]+src_yuv444[4]+1)>>1;
		dst_yuy2[3]=(src_yuv444[2]+src_yuv444[5]+1)>>1;
		dst_yuy2+=4;
		src_yuv444+=6;
	}
	//once tow dst_yuv444
}

void YUY2ToYUV444Row_C(const uint8* src_yuy2,uint8* dst_yuv444,int width) {
	// Output a row of UV values.
	int x;
	for (x = 0; x < width-1; x +=2) {
		dst_yuv444[0] = src_yuy2[0];
		dst_yuv444[3] = src_yuy2[2];
		dst_yuv444[1] = dst_yuv444[4]= src_yuy2[1];
		dst_yuv444[2] = dst_yuv444[5]= src_yuy2[3];
		src_yuy2 += 4;
		dst_yuv444+=6;
	}
	if(width&1){
		dst_yuv444[0]=src_yuy2[0];
		dst_yuv444[1]=src_yuy2[1];
		dst_yuv444[2]=src_yuy2[3];
	}
	//once tow dst_yuv444
}