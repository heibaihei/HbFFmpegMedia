#include "FormatCvt.h"
#include "FormatCvt_row.h"
#include "FormatCvt_row_define.h"
#if defined(HAVE_NEON)
#define YANY(NAMEANY, I420TORGB_SIMD, I420TORGB_C, UV_SHIFT, BPP, MASK)        \
    void NAMEANY(const uint8* y_buf, const uint8* u_buf, const uint8* v_buf,   \
                 uint8* rgb_buf, int width) {                                  \
      int n = width & ~MASK;                                                   \
      if (n > 0) {                                                             \
        I420TORGB_SIMD(y_buf, u_buf, v_buf, rgb_buf, n);                       \
      }                                                                        \
      I420TORGB_C(y_buf + n,                                                   \
                  u_buf + (n >> UV_SHIFT),                                     \
                  v_buf + (n >> UV_SHIFT),                                     \
                  rgb_buf + n * BPP, width & MASK);                            \
    }
YANY(I422ToARGBRow_Any_NEON, I422ToARGBRow_NEON, I422ToARGBRow_C, 1, 4, 7)
#undef YANY

#define YANY(NAMEANY, ARGBTOY_SIMD, ARGBTOY_C, SBPP, BPP, MASK)                \
	void NAMEANY(const uint8* src_argb, uint8* dst_y, int width) {             \
	int n = width & ~MASK;                                                   \
	if (n > 0) {                                                             \
	ARGBTOY_SIMD(src_argb, dst_y, n);                                      \
	}                                                                        \
	ARGBTOY_C(src_argb + n * SBPP,                                           \
	dst_y  + n * BPP, width & MASK);                               \
}


YANY(YUY2ToYUV444Row_Any_NEON, YUY2ToYUV444Row_NEON, YUY2ToYUV444Row_C, 2, 3, 15)
YANY(YUV444ToYUY2Row_Any_NEON, YUV444ToYUY2Row_NEON,YUV444ToYUY2Row_C, 3, 2, 15)
#undef YANY


#define UV422ANY(NAMEANY, ANYTOUV_SIMD, ANYTOUV_C, BPP, SHIFT, MASK)           \
    void NAMEANY(const uint8* src_uv, uint8* dst_u, uint8* dst_v, int width) { \
      int n = width & ~MASK;                                                   \
      if (n > 0) {                                                             \
        ANYTOUV_SIMD(src_uv, dst_u, dst_v, n);                                 \
      }                                                                        \
      ANYTOUV_C(src_uv  + n * BPP,                                             \
                dst_u + (n >> SHIFT),                                          \
                dst_v + (n >> SHIFT),                                          \
                width & MASK);                                                 \
    }
UV422ANY(ARGBToUV422Row_Any_NEON, ARGBToUV422Row_NEON,ARGBToUV422Row_C, 4, 1, 15)
#undef UV422ANY


#define YANY(NAMEANY, ARGBTOY_SIMD, ARGBTOY_C, SBPP, BPP, MASK)                \
    void NAMEANY(const uint8* src_argb, uint8* dst_y, int width) {             \
      int n = width & ~MASK;                                                   \
      if (n > 0) {                                                             \
        ARGBTOY_SIMD(src_argb, dst_y, n);                                      \
      }                                                                        \
      ARGBTOY_C(src_argb + n * SBPP,                                           \
                dst_y  + n * BPP, width & MASK);                               \
    }
#ifdef HAS_ARGBTOYROW_NEON
YANY(ARGBToYRow_Any_NEON, ARGBToYRow_NEON, ARGBToYRow_C, 4, 1, 7)
#endif
#undef YANY



#define UVANY(NAMEANY, ANYTOUV_SIMD, ANYTOUV_C, BPP, MASK)                     \
    void NAMEANY(const uint8* src_argb, int src_stride_argb,                   \
                 uint8* dst_u, uint8* dst_v, int width) {                      \
      int n = width & ~MASK;                                                   \
      if (n > 0) {                                                             \
        ANYTOUV_SIMD(src_argb, src_stride_argb, dst_u, dst_v, n);              \
      }                                                                        \
      ANYTOUV_C(src_argb  + n * BPP, src_stride_argb,                          \
                dst_u + (n >> 1),                                              \
                dst_v + (n >> 1),                                              \
                width & MASK);                                                 \
    }

#ifdef HAS_ARGBTOUVROW_NEON
UVANY(ARGBToUVRow_Any_NEON, ARGBToUVRow_NEON, ARGBToUVRow_C, 4, 15)
#endif
#undef UVANY


#define MERGEUVROW_ANY(NAMEANY, ANYTOUV_SIMD, ANYTOUV_C, MASK)                 \
    void NAMEANY(const uint8* src_u, const uint8* src_v,                       \
                 uint8* dst_uv, int width) {                                   \
      int n = width & ~MASK;                                                   \
      if (n > 0) {                                                             \
        ANYTOUV_SIMD(src_u, src_v, dst_uv, n);                                 \
      }                                                                        \
      ANYTOUV_C(src_u + n,                                                     \
                src_v + n,                                                     \
                dst_uv + n * 2,                                                \
                width & MASK);                                                 \
    }

#ifdef HAS_MERGEUVROW_NEON
MERGEUVROW_ANY(MergeUVRow_Any_NEON, MergeUVRow_NEON, MergeUVRow_C, 15)
#endif
#undef MERGEUVROW_ANY




#define NV2NY(NAMEANY, NV12TORGB_SIMD, NV12TORGB_C, UV_SHIFT, BPP, MASK)       \
    void NAMEANY(const uint8* y_buf, const uint8* uv_buf,                      \
                 uint8* rgb_buf, int width) {                                  \
      int n = width & ~MASK;                                                   \
      if (n > 0) {                                                             \
        NV12TORGB_SIMD(y_buf, uv_buf, rgb_buf, n);                             \
      }                                                                        \
      NV12TORGB_C(y_buf + n,                                                   \
                  uv_buf + (n >> UV_SHIFT),                                    \
                  rgb_buf + n * BPP, width & MASK);                            \
    }

#ifdef HAS_NV12TOARGBROW_NEON
NV2NY(NV12ToARGBRow_Any_NEON, NV12ToARGBRow_NEON, NV12ToARGBRow_C, 0, 4, 7)
NV2NY(NV21ToARGBRow_Any_NEON, NV21ToARGBRow_NEON, NV21ToARGBRow_C, 0, 4, 7)
#endif




void J422ToARGBRow_Any_NEON(const uint8* y_buf,
                                       const uint8* u_buf, 
                                       const uint8* v_buf, 
                                       uint8* rgb_buf, int width){

    int n = width & ~7;                                                    
      if (n > 0) {                                                             
        J422ToARGBRow_NEON(y_buf, u_buf, v_buf, rgb_buf, n);                       
      }                                                                        
      J422ToARGBRow_C(y_buf + n,                                                   
                  u_buf + (n >> 1),                                     
                  v_buf + (n >> 1),                                     
                  rgb_buf + n * 4, width & 7);                  
}


void ARGBToYJRow_Any_NEON(const uint8* src_argb, uint8* dst_y, int width) {             
      int n = width & ~7;                                                   
      if (n > 0) {                                                             
        ARGBToYJRow_NEON(src_argb, dst_y, n);                                      
      }                                                                        
      ARGBToYJRow_C(src_argb + n * 4,                                           
                dst_y  + n * 1, width & 7);                               
}



                  
void ARGBToUVJRow_Any_NEON(const uint8* src_argb, int src_stride_argb,                   
                 uint8* dst_u, uint8* dst_v, int width) {                      
      int n = width & ~15;                                                   
      if (n > 0) {                                                             
        ARGBToUVJRow_NEON(src_argb, src_stride_argb, dst_u, dst_v, n);              
      }                                                                        
      ARGBToUVJRow_C(src_argb  + n * 4, src_stride_argb,                          
                dst_u + (n >> 1),                                              
                dst_v + (n >> 1),                                              
                width & 15);                                                 
}


void ARGBToUVJ422Row_Any_NEON(const uint8* src_argb,
                 uint8* dst_u, uint8* dst_v, int width) {                      
        int n = width & ~15;                                                   
        if (n > 0) {                                                             
          ARGBToUVJ422Row_NEON(src_argb,dst_u, dst_v, n);
        }                                                                        
        ARGBToUVJ422Row_C(src_argb  + n * 4,
                  dst_u + (n >> 1),                                              
                  dst_v + (n >> 1),                                              
                  width & 15);                                                 
}


#define YANY(NAMEANY, I420TORGB_SIMD, I420TORGB_C, UV_SHIFT, BPP, MASK)        \
    void NAMEANY(const uint8* y_buf, const uint8* u_buf, const uint8* v_buf,   \
                 uint8* rgb_buf, int width) {                                  \
      int n = width & ~MASK;                                                   \
      if (n > 0) {                                                             \
        I420TORGB_SIMD(y_buf, u_buf, v_buf, rgb_buf, n);                       \
      }                                                                        \
      I420TORGB_C(y_buf + n,                                                   \
                  u_buf + (n >> UV_SHIFT),                                     \
                  v_buf + (n >> UV_SHIFT),                                     \
                  rgb_buf + n * BPP, width & MASK);                            \
    }

#ifdef HAS_I422TOYUY2JROW_NEON
YANY(I422ToYUY2JRow_Any_NEON, I422ToYUY2JRow_NEON, I422ToYUY2JRow_C, 1, 2, 15)
#endif
#undef YANY






#define RGBANY(NAMEANY, ARGBTORGB_SIMD, ARGBTORGB_C, SBPP, BPP, MASK)          \
    void NAMEANY(const uint8* src, uint8* dst, int width) {                    \
      int n = width & ~MASK;                                                   \
      if (n > 0) {                                                             \
        ARGBTORGB_SIMD(src, dst, n);                                           \
      }                                                                        \
      ARGBTORGB_C(src + n * SBPP, dst + n * BPP, width & MASK);                \
    }

RGBANY(YUY2JToARGBRow_Any_NEON, YUY2JToARGBRow_NEON, YUY2JToARGBRow_C, 2, 4, 7)
#undef RGBANY
#endif