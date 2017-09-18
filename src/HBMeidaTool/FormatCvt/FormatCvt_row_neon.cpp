#include "FormatCvt.h"
#include "FormatCvt_row.h"
#include "FormatCvt_row_define.h"
#if defined(HAVE_NEON)&&defined(HAVE_NEON32)
		void MergeUVRow_NEON(const uint8* src_u, const uint8* src_v, uint8* dst_uv,
		                     int width) {
		  asm volatile (
		    ".p2align   2                              \n"
		  "1:                                          \n"
		    MEMACCESS(0)
		    "vld1.8     {q0}, [%0]!                    \n"  // load U
		    MEMACCESS(1)
		    "vld1.8     {q1}, [%1]!                    \n"  // load V
		    "subs       %3, %3, #16                    \n"  // 16 processed per loop
		    MEMACCESS(2)
		    "vst2.u8    {q0, q1}, [%2]!                \n"  // store 16 pairs of UV
		    "bgt        1b                             \n"
		    :
		      "+r"(src_u),   // %0
		      "+r"(src_v),   // %1
		      "+r"(dst_uv),  // %2
		      "+r"(width)    // %3  // Output registers
		    :                       // Input registers
		    : "cc", "memory", "q0", "q1"  // Clobber List
		  );
		}


	#ifndef RGBTOUV
	#define RGBTOUV(QB, QG, QR) \
	    "vmul.s16   q8, " #QB ", q10               \n"  /* B                    */ \
	    "vmls.s16   q8, " #QG ", q11               \n"  /* G                    */ \
	    "vmls.s16   q8, " #QR ", q12               \n"  /* R                    */ \
	    "vadd.u16   q8, q8, q15                    \n"  /* +128 -> unsigned     */ \
	    "vmul.s16   q9, " #QR ", q10               \n"  /* R                    */ \
	    "vmls.s16   q9, " #QG ", q14               \n"  /* G                    */ \
	    "vmls.s16   q9, " #QB ", q13               \n"  /* B                    */ \
	    "vadd.u16   q9, q9, q15                    \n"  /* +128 -> unsigned     */ \
	    "vqshrn.u16  d0, q8, #8                    \n"  /* 16 bit to 8 bit U    */ \
	    "vqshrn.u16  d1, q9, #8                    \n"  /* 16 bit to 8 bit V    */
	#endif

 void ARGBToUVRow_NEON(const uint8* src_argb, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "add        %1, %0, %1                     \n"  // src_stride + src_argb
    "vmov.s16   q10, #112 / 2                  \n"  // UB / VR 0.875 coefficient
    "vmov.s16   q11, #74 / 2                   \n"  // UG -0.5781 coefficient
    "vmov.s16   q12, #38 / 2                   \n"  // UR -0.2969 coefficient
    "vmov.s16   q13, #18 / 2                   \n"  // VB -0.1406 coefficient
    "vmov.s16   q14, #94 / 2                   \n"  // VG -0.7344 coefficient
    "vmov.u16   q15, #0x8080                   \n"  // 128.5
    ".p2align   2                              \n"
  "1:                                          \n"
    MEMACCESS(0)
    "vld4.8     {d0, d2, d4, d6}, [%0]!        \n"  // load 8 ARGB pixels.
    MEMACCESS(0)
    "vld4.8     {d1, d3, d5, d7}, [%0]!        \n"  // load next 8 ARGB pixels.
    "vpaddl.u8  q0, q0                         \n"  // B 16 bytes -> 8 shorts.
    "vpaddl.u8  q1, q1                         \n"  // G 16 bytes -> 8 shorts.
    "vpaddl.u8  q2, q2                         \n"  // R 16 bytes -> 8 shorts.
    MEMACCESS(1)
    "vld4.8     {d8, d10, d12, d14}, [%1]!     \n"  // load 8 more ARGB pixels.
    MEMACCESS(1)
    "vld4.8     {d9, d11, d13, d15}, [%1]!     \n"  // load last 8 ARGB pixels.
    "vpadal.u8  q0, q4                         \n"  // B 16 bytes -> 8 shorts.
    "vpadal.u8  q1, q5                         \n"  // G 16 bytes -> 8 shorts.
    "vpadal.u8  q2, q6                         \n"  // R 16 bytes -> 8 shorts.

    "vrshr.u16  q0, q0, #1                     \n"  // 2x average
    "vrshr.u16  q1, q1, #1                     \n"
    "vrshr.u16  q2, q2, #1                     \n"

    "subs       %4, %4, #16                    \n"  // 32 processed per loop.

    #ifdef MT_RED
      #if MT_RED==2
      RGBTOUV(q0, q1, q2)
      #endif
      #if MT_RED==0
      RGBTOUV(q2, q1,  q0)
      #endif
    #else
      RGBTOUV(q0, q1,  q2)
    #endif
    MEMACCESS(2)
    "vst1.8     {d0}, [%2]!                    \n"  // store 8 pixels U.
    MEMACCESS(3)
    "vst1.8     {d1}, [%3]!                    \n"  // store 8 pixels V.
    "bgt        1b                             \n"
  : "+r"(src_argb),  // %0
    "+r"(src_stride_argb),  // %1
    "+r"(dst_u),     // %2
    "+r"(dst_v),     // %3
    "+r"(pix)        // %4
  :
  : "cc", "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7",
    "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}


void ARGBToYRow_NEON(const uint8* src_argb, uint8* dst_y, int pix) {
  asm volatile (
    "vmov.u8    d24, #13                       \n"  // B * 0.1016 coefficient
    "vmov.u8    d25, #65                       \n"  // G * 0.5078 coefficient
    "vmov.u8    d26, #33                       \n"  // R * 0.2578 coefficient
    "vmov.u8    d27, #16                       \n"  // Add 16 constant
    ".p2align   2                              \n"
  "1:                                          \n"
    //MEMACCESS(0)
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  // load 8 ARGB pixels.
    "subs       %2, %2, #8                     \n"  // 8 processed per loop.
    #if MT_RED==2
    "vmull.u8   q2, d0, d24                    \n"  // B
    "vmlal.u8   q2, d1, d25                    \n"  // G
    "vmlal.u8   q2, d2, d26                    \n"  // R
    #endif
    #if MT_RED==0
    "vmull.u8   q2, d2, d24                    \n"  // B
    "vmlal.u8   q2, d1, d25                    \n"  // G
    "vmlal.u8   q2, d0, d26                    \n"  // R
    #endif
    "vqrshrun.s16 d0, q2, #7                   \n"  // 16 bit to 8 bit Y
    "vqadd.u8   d0, d27                        \n"
    //MEMACCESS(1)
    "vst1.8     {d0}, [%1]!                    \n"  // store 8 pixels Y.
    "bgt        1b                             \n"
  : "+r"(src_argb),  // %0
    "+r"(dst_y),     // %1
    "+r"(pix)        // %2
  :
  : "cc", "memory", "q0", "q1", "q2", "q12", "q13"
  );
}



void ARGBToUV422Row_NEON(const uint8* src_argb, uint8* dst_u, uint8* dst_v,
                         int pix) {
  asm volatile (
    "vmov.s16   q10, #112 / 2                  \n"  // UB / VR 0.875 coefficient
    "vmov.s16   q11, #74 / 2                   \n"  // UG -0.5781 coefficient
    "vmov.s16   q12, #38 / 2                   \n"  // UR -0.2969 coefficient
    "vmov.s16   q13, #18 / 2                   \n"  // VB -0.1406 coefficient
    "vmov.s16   q14, #94 / 2                   \n"  // VG -0.7344 coefficient
    "vmov.u16   q15, #0x8080                   \n"  // 128.5
    ".p2align   2                              \n"
  "1:                                          \n"
    //MEMACCESS(0)
    "vld4.8     {d0, d2, d4, d6}, [%0]!        \n"  // load 8 ARGB pixels.
    //MEMACCESS(0)
    "vld4.8     {d1, d3, d5, d7}, [%0]!        \n"  // load next 8 ARGB pixels.

    "vpaddl.u8  q0, q0                         \n"  // B 16 bytes -> 8 shorts.
    "vpaddl.u8  q1, q1                         \n"  // G 16 bytes -> 8 shorts.
    "vpaddl.u8  q2, q2                         \n"  // R 16 bytes -> 8 shorts.

    "subs       %3, %3, #16                    \n"  // 16 processed per loop.
    #if MT_RED==2
    "vmul.s16   q8, q0, q10                    \n"  // B
    "vmls.s16   q8, q1, q11                    \n"  // G
    "vmls.s16   q8, q2, q12                    \n"  // R
    #endif
    #if MT_RED==0
    "vmul.s16   q8, q2, q10                    \n"  // B
    "vmls.s16   q8, q1, q11                    \n"  // G
    "vmls.s16   q8, q0, q12                    \n"  // R
    #endif
    "vadd.u16   q8, q8, q15                    \n"  // +128 -> unsigned
    #if MT_RED==2
    "vmul.s16   q9, q2, q10                    \n"  // R
    "vmls.s16   q9, q1, q14                    \n"  // G
    "vmls.s16   q9, q0, q13                    \n"  // B
    #endif
     #if MT_RED==0
    "vmul.s16   q9, q0, q10                    \n"  // R
    "vmls.s16   q9, q1, q14                    \n"  // G
    "vmls.s16   q9, q2, q13                    \n"  // B
    #endif
    "vadd.u16   q9, q9, q15                    \n"  // +128 -> unsigned

    "vqshrn.u16  d0, q8, #8                    \n"  // 16 bit to 8 bit U
    "vqshrn.u16  d1, q9, #8                    \n"  // 16 bit to 8 bit V

   // MEMACCESS(1)
    "vst1.8     {d0}, [%1]!                    \n"  // store 8 pixels U.
    //MEMACCESS(2)
    "vst1.8     {d1}, [%2]!                    \n"  // store 8 pixels V.
    "bgt        1b                             \n"
  : "+r"(src_argb),  // %0
    "+r"(dst_u),     // %1
    "+r"(dst_v),     // %2
    "+r"(pix)        // %3
  :
  : "cc", "memory", "q0", "q1", "q2", "q3",
    "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

#define READYUV422                                                             \
    MEMACCESS(0)                                                               \
    "vld1.8     {d0}, [%0]!                    \n"                             \
    MEMACCESS(1)                                                               \
    "vld1.32    {d2[0]}, [%1]!                 \n"                             \
    MEMACCESS(2)                                                               \
    "vld1.32    {d2[1]}, [%2]!                 \n"

#define YUV422TORGB_SETUP_REG                                                  \
    MEMACCESS([kUVToRB])                                                       \
    "vld1.8     {d24}, [%[kUVToRB]]            \n"                             \
    MEMACCESS([kUVToG])                                                        \
    "vld1.8     {d25}, [%[kUVToG]]             \n"                             \
    MEMACCESS([kUVBiasBGR])                                                    \
    "vld1.16    {d26[], d27[]}, [%[kUVBiasBGR]]! \n"                           \
    MEMACCESS([kUVBiasBGR])                                                    \
    "vld1.16    {d8[], d9[]}, [%[kUVBiasBGR]]!   \n"                           \
    MEMACCESS([kUVBiasBGR])                                                    \
    "vld1.16    {d28[], d29[]}, [%[kUVBiasBGR]]  \n"                           \
    MEMACCESS([kYToRgb])                                                       \
    "vld1.32    {d30[], d31[]}, [%[kYToRgb]]     \n"

#define YUV422TORGB                                                 			\
    "vmull.u8   q8, d2, d24                    \n" /* u/v B/R component      */\
    "vmull.u8   q9, d2, d25                    \n" /* u/v G component        */\
    "vmovl.u8   q0, d0                         \n" /* Y                      */\
    "vmovl.s16  q10, d1                        \n"                             \
    "vmovl.s16  q0, d0                         \n"                             \
    "vmul.s32   q10, q10, q15                  \n"                             \
    "vmul.s32   q0, q0, q15                    \n"                             \
    "vqshrun.s32 d0, q0, #16                   \n"                             \
    "vqshrun.s32 d1, q10, #16                  \n" /* Y                      */\
    "vadd.s16   d18, d19                       \n"                             \
    "vshll.u16  q1, d16, #16                   \n" /* Replicate u * UB       */\
    "vshll.u16  q10, d17, #16                  \n" /* Replicate v * VR       */\
    "vshll.u16  q3, d18, #16                   \n" /* Replicate (v*VG + u*UG)*/\
    "vaddw.u16  q1, q1, d16                    \n"                             \
    "vaddw.u16  q10, q10, d17                  \n"                             \
    "vaddw.u16  q3, q3, d18                    \n"                             \
    "vqadd.s16  q8, q0, q13                    \n" /* B */                     \
    "vqadd.s16  q9, q0, q14                    \n" /* R */                     \
    "vqadd.s16  q0, q0, q4                     \n" /* G */                     \
    "vqadd.s16  q8, q8, q1                     \n" /* B */                     \
    "vqadd.s16  q9, q9, q10                    \n" /* R */                     \
    "vqsub.s16  q0, q0, q3                     \n" /* G */                     \
    "vqshrun.s16 d20, q8, #6                   \n" /* B */                     \
    "vqshrun.s16 d22, q9, #6                   \n" /* R */                     \
    "vqshrun.s16 d21, q0, #6                   \n" /* G */
    //d20=b d21=g d22=r    

    // YUV to RGB conversion constants.
    // Y contribution to R,G,B.  Scale and bias.
    #define _YG 18997 /* round(1.164 * 64 * 256 * 256 / 257) */
    #define _YGB 1160 /* 1.164 * 64 * 16 - adjusted for even error distribution */

    // U and V contributions to R,G,B.
    #define UB -128 /* -min(128, round(2.018 * 64)) */
    #define UG 25 /* -round(-0.391 * 64) */
    #define VG 52 /* -round(-0.813 * 64) */
    #define VR -102 /* -round(1.596 * 64) */

    // Bias values to subtract 16 from Y and 128 from U and V.
    #define _BB (UB * 128            - _YGB)
    #define _BG (UG * 128 + VG * 128 - _YGB)
    #define _BR            (VR * 128 - _YGB)

    static uvec8 kUVToRB  = { 128, 128, 128, 128, 102, 102, 102, 102,
                              0, 0, 0, 0, 0, 0, 0, 0 };
    static uvec8 kUVToG = { 25, 25, 25, 25, 52, 52, 52, 52,
                            0, 0, 0, 0, 0, 0, 0, 0 };
    static vec16 kUVBiasBGR = { _BB, _BG, _BR, 0, 0, 0, 0, 0 };
    static vec32 kYToRgb = { 0x0101 * _YG, 0, 0, 0 };

    #undef _YG
    #undef _YGB
    #undef UB
    #undef UG
    #undef VG
    #undef VR
    #undef _BB
    #undef _BG
    #undef _BR

void I422ToARGBRow_NEON(const uint8* src_y,
                        const uint8* src_u,
                        const uint8* src_v,
                        uint8* dst_argb,
                        int width) {
  		asm volatile (
		    YUV422TORGB_SETUP_REG
		    ".p2align   2                              \n"
		  "1:                                          \n"
		    READYUV422
		    YUV422TORGB
		    "subs       %4, %4, #8                     \n"
		    "vmov.u8    d23, #255                      \n"
		    MEMACCESS(3)
            #if MT_RED==0
                "vmov.u8 d30,d22    \n"
                "vmov.u8 d22,d20    \n"
                "vmov.u8 d20,d30    \n"
            #endif
		    "vst4.8     {d20, d21, d22, d23}, [%3]!    \n"
		    "bgt        1b                             \n"
		    : "+r"(src_y),     // %0
		      "+r"(src_u),     // %1
		      "+r"(src_v),     // %2
		      "+r"(dst_argb),  // %3
		      "+r"(width)      // %4
		    : [kUVToRB]"r"(&kUVToRB),   // %5
		      [kUVToG]"r"(&kUVToG),     // %6
		      [kUVBiasBGR]"r"(&kUVBiasBGR),
		      [kYToRgb]"r"(&kYToRgb)
		    : "cc", "memory", "q0", "q1", "q2", "q3", "q4",
		      "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
	  	);
	}

 	#define READNV12                                                               \
    MEMACCESS(0)                                                               \
    "vld1.8     {d0}, [%0]!                    \n"                             \
    MEMACCESS(1)                                                               \
    "vld1.8     {d2}, [%1]!                    \n"                             \
    "vmov.u8    d3, d2                         \n"/* split odd/even uv apart */\
    "vuzp.u8    d2, d3                         \n"                             \
    "vtrn.u32   d2, d3                         \n"

	void NV12ToARGBRow_NEON(const uint8* src_y,
	                        const uint8* src_uv,
	                        uint8* dst_argb,
	                        int width) {
	  asm volatile (
	    YUV422TORGB_SETUP_REG
	    ".p2align   2                                     \n"
	  "1:                                                 \n"
	    READNV12
	    YUV422TORGB
	    "subs       %3, %3, #8                            \n"
	    "vmov.u8    d23, #255                             \n"
	    MEMACCESS(2)
         #if MT_RED==0
                "vmov.u8 d30,d22    \n" 
                "vmov.u8 d22,d20    \n"
                "vmov.u8 d20,d30    \n"
        #endif
	    "vst4.8     {d20, d21, d22, d23}, [%2]!           \n"
	    "bgt        1b                                    \n"
	    : "+r"(src_y),     // %0
	      "+r"(src_uv),    // %1
	      "+r"(dst_argb),  // %2
	      "+r"(width)      // %3
	    : [kUVToRB]"r"(&kUVToRB),   // %4
	      [kUVToG]"r"(&kUVToG),     // %5
	      [kUVBiasBGR]"r"(&kUVBiasBGR),
	      [kYToRgb]"r"(&kYToRgb)
	    : "cc", "memory", "q0", "q1", "q2", "q3", "q4",
	      "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
	  );
	}

	#define READNV21                                                           \
    MEMACCESS(0)                                                               \
    "vld1.8     {d0}, [%0]!                    \n"                             \
    MEMACCESS(1)                                                               \
    "vld1.8     {d2}, [%1]!                    \n"                             \
    "vmov.u8    d3, d2                         \n"/* split odd/even uv apart */\
    "vuzp.u8    d3, d2                         \n"                             \
    "vtrn.u32   d2, d3                         \n"

	void NV21ToARGBRow_NEON(const uint8* src_y,
                        const uint8* src_uv,
                        uint8* dst_argb,
                        int width) {
		  asm volatile (
		    YUV422TORGB_SETUP_REG
		    ".p2align   2                              \n"
		  "1:                                          \n"
		    READNV21
		    YUV422TORGB
		    "subs       %3, %3, #8                     \n"
		    "vmov.u8    d23, #255                      \n"
		    MEMACCESS(2)
             #if MT_RED==0
                "vmov.u8 d30,d22    \n"
                "vmov.u8 d22,d20    \n"
                "vmov.u8 d20,d30    \n"
            #endif
		    "vst4.8     {d20, d21, d22, d23}, [%2]!    \n"
		    "bgt        1b                             \n"
		    : "+r"(src_y),     // %0
		      "+r"(src_uv),    // %1
		      "+r"(dst_argb),  // %2
		      "+r"(width)      // %3
		    : [kUVToRB]"r"(&kUVToRB),   // %4
		      [kUVToG]"r"(&kUVToG),     // %5
		      [kUVBiasBGR]"r"(&kUVBiasBGR),
		      [kYToRgb]"r"(&kYToRgb)
		    : "cc", "memory", "q0", "q1", "q2", "q3", "q4",
		      "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
		  );
	}


	void ARGBToYJRow_NEON(const uint8* src_argb, uint8* dst_y, int pix) {
      asm volatile (
        "vmov.u8    d24, #15                       \n"  // B * 0.11400 coefficient
        "vmov.u8    d25, #75                       \n"  // G * 0.58700 coefficient
        "vmov.u8    d26, #38                       \n"  // R * 0.29900 coefficient
        ".p2align   2                              \n"
      "1:                                          \n"

        "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  // load 8 ARGB pixels.
        "subs       %2, %2, #8                     \n"  // 8 processed per loop.
        #if MT_RED==2
        "vmull.u8   q2, d0, d24                    \n"  // B
        "vmlal.u8   q2, d1, d25                    \n"  // G
        "vmlal.u8   q2, d2, d26                    \n"  // R
        #endif
        #if MT_RED==0
        "vmull.u8   q2, d2, d24                    \n"  // B
        "vmlal.u8   q2, d1, d25                    \n"  // G
        "vmlal.u8   q2, d0, d26                    \n"  // R
        #endif
        "vqrshrun.s16 d0, q2, #7                   \n"  // 15 bit to 8 bit Y

        "vst1.8     {d0}, [%1]!                    \n"  // store 8 pixels Y.
        "bgt        1b                             \n"
      : "+r"(src_argb),  // %0
        "+r"(dst_y),     // %1
        "+r"(pix)        // %2
      :
      : "cc", "memory", "q0", "q1", "q2", "q12", "q13"
      );
    }
 
void ARGBToUVJ422Row_NEON(const uint8* src_argb, uint8* dst_u, uint8* dst_v,int pix){
  int x=1;
  asm volatile (
    "add        %1, %0, %1                     \n"  // src_stride + src_argb
    "vmov.s16   q10, #127                   \n"  // UB / VR 0.500 coefficient
    "vmov.s16   q11, #84                    \n"  // UG -0.33126 coefficient
    "vmov.s16   q12, #43                    \n"  // UR -0.16874 coefficient
    "vmov.s16   q13, #20                    \n"  // VB -0.08131 coefficient
    "vmov.s16   q14, #107                   \n"  // VG -0.41869 coefficient
    "vmov.u16   q15, #0x8080                   \n"  // 128.5
    ".p2align   2                              \n"
  "1:                                          \n"
   // MEMACCESS(0)
    "vld4.8     {d0, d2, d4, d6}, [%0]!        \n"  // load 8 ARGB pixels.
   // MEMACCESS(0)
    "vld4.8     {d1, d3, d5, d7}, [%0]!        \n"  // load next 8 ARGB pixels.
    "vpaddl.u8  q0, q0                         \n"  // B 16 bytes -> 8 shorts.
    "vpaddl.u8  q1, q1                         \n"  // G 16 bytes -> 8 shorts.
    "vpaddl.u8  q2, q2                         \n"  // R 16 bytes -> 8 shorts.
    
    "vrshr.u16  q0, q0, #1                     \n"  // 2x average
    "vrshr.u16  q1, q1, #1                     \n"
    "vrshr.u16  q2, q2, #1                     \n"

    "subs       %4, %4, #16                    \n"  // 32 processed per loop.
    #if MT_RED==2
    RGBTOUV(q0, q1, q2)
    #endif
    #if MT_RED==0
    RGBTOUV(q2, q1,  q0)
    #endif
   // MEMACCESS(2)
    "vst1.8     {d0}, [%2]!                    \n"  // store 8 pixels U.
  //  MEMACCESS(3)
    "vst1.8     {d1}, [%3]!                    \n"  // store 8 pixels V.
    "bgt        1b                             \n"
  : "+r"(src_argb),  // %0
    "+r"(x),
    "+r"(dst_u),     // %2
    "+r"(dst_v),     // %3
    "+r"(pix)        // %4
  :
  : "cc", "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7",
    "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}




	void ARGBToUVJRow_NEON(const uint8* src_argb, int src_stride_argb,
	                       uint8* dst_u, uint8* dst_v, int pix) {
	  asm volatile (
	    "add        %1, %0, %1                     \n"  // src_stride + src_argb
	    "vmov.s16   q10, #127 / 2                  \n"  // UB / VR 0.500 coefficient
	    "vmov.s16   q11, #84 / 2                   \n"  // UG -0.33126 coefficient
	    "vmov.s16   q12, #43 / 2                   \n"  // UR -0.16874 coefficient
	    "vmov.s16   q13, #20 / 2                   \n"  // VB -0.08131 coefficient
	    "vmov.s16   q14, #107 / 2                  \n"  // VG -0.41869 coefficient
	    "vmov.u16   q15, #0x8080                   \n"  // 128.5
	    ".p2align   2                              \n"
	  "1:                                          \n"

	    "vld4.8     {d0, d2, d4, d6}, [%0]!        \n"  // load 8 ARGB pixels.

	    "vld4.8     {d1, d3, d5, d7}, [%0]!        \n"  // load next 8 ARGB pixels.
	    "vpaddl.u8  q0, q0                         \n"  // B 16 bytes -> 8 shorts.
	    "vpaddl.u8  q1, q1                         \n"  // G 16 bytes -> 8 shorts.
	    "vpaddl.u8  q2, q2                         \n"  // R 16 bytes -> 8 shorts.

	    "vld4.8     {d8, d10, d12, d14}, [%1]!     \n"  // load 8 more ARGB pixels.

	    "vld4.8     {d9, d11, d13, d15}, [%1]!     \n"  // load last 8 ARGB pixels.
	    "vpadal.u8  q0, q4                         \n"  // B 16 bytes -> 8 shorts.
	    "vpadal.u8  q1, q5                         \n"  // G 16 bytes -> 8 shorts.
	    "vpadal.u8  q2, q6                         \n"  // R 16 bytes -> 8 shorts.

	    "vrshr.u16  q0, q0, #1                     \n"  // 2x average
	    "vrshr.u16  q1, q1, #1                     \n"
	    "vrshr.u16  q2, q2, #1                     \n"

	    "subs       %4, %4, #16                    \n"  // 32 processed per loop.
        #if MT_RED==2
	    RGBTOUV(q0, q1, q2)
        #endif
        #if MT_RED==0
        RGBTOUV(q2, q1, q0)
        #endif
	    "vst1.8     {d0}, [%2]!                    \n"  // store 8 pixels U.

	    "vst1.8     {d1}, [%3]!                    \n"  // store 8 pixels V.
	    "bgt        1b                             \n"
	  : "+r"(src_argb),  // %0
	    "+r"(src_stride_argb),  // %1
	    "+r"(dst_u),     // %2
	    "+r"(dst_v),     // %3
	    "+r"(pix)        // %4
	  :
	  : "cc", "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7",
	    "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
	  );
	}

	void J422ToARGBRow_NEON(const uint8* src_y,
                        const uint8* src_u,
                        const uint8* src_v,
                        uint8* dst_argb,
                        int width) {
      asm volatile (
        "vmov.u8       d31,#128                       \n"//128是YUV的偏移量
        "1:                                           \n"
          "vld1.8 {d0},[%0]!                          \n" //Y

          "vld1.32  {d22[0]},[%1]!                    \n"  //load UV
          "vld1.32  {d22[1]},[%2]!                    \n"
       		
          "vmovl.u8  q10,d22                          \n"
          //d20=16x4
          "vshll.u16  q1, d20, #16                    \n"  //Q1是U
          "vshll.u16  q2, d21, #16                    \n"  //Q2是V

          "vaddw.u16  q1, q1,d20                      \n"  //EXTEND
          "vaddw.u16  q2, q2,d21                      \n"
          "vqmovn.u16 d1,q1                           \n"  //u 
          "vqmovn.u16 d2,q2                           \n"  //v   
          //d0是Y d1是U d2是V

         //------------------------DO R-----------------------------
          "vmov.u8   d3,#104                          \n" //#104->d3
          "vmull.u8  q3,d3,d2                         \n" //0.402v->q3
          "vqshrn.u16 d3,q3,#8                        \n"
          "vmovl.u8 q3,d3                             \n" //0.402V->Q3
          "vmovl.u8  q4,d2                            \n" //1.0v->Q4
          "vadd.u16  q3,q3,q4                         \n" //1.402v->Q3

          "vmov.u8   d30,#104                         \n" //#104->d30
          "vmull.u8  q5,d31,d30                       \n" //0.402*128->q5
          "vqshrn.u16 d4,q5,#8                        \n" //   
          "vmovl.u8 q5,d4                             \n" //q5->0.402v
          "vmovl.u8  q4,d31                           \n" //1.0*128->q4
          "vadd.u16  q4,q4,q5                         \n" //1.402*128

           "vmovl.u8  q2,d0                           \n" //y->q2

          "vqadd.u16 q2,q3,q2                         \n" //Y+(Q3=1.402V)
          "vqsub.u16 q2,q2,q4                         \n" //Y+(Q3=1.402V)-128*1.402
          #if MT_RED==2
          "vqmovn.u16 d24,q2                          \n" //RES
          #endif
          #if MT_RED==0
          "vqmovn.u16 d22,q2                           \n"
          #endif
          //-----------------------DO G-----------------------------

          //G = Y - U *  0.34414 - V *  0.71414
          //0.34414*256=88     0.71414*256=183
          //
          "vmov.u8   d3,#88                           \n" //#104->d3
          "vmov.u8   d4,#183                          \n"
          "vmull.u8  q3,d3,d31                        \n"//0.402v->q3
          "vqshrn.u16 d3,q3,#8                        \n"
          "vmovl.u8 q3,d3\n"
          "vmull.u8  q4,d4,d31                        \n"//0.402v->q3
          "vqshrn.u16 d4,q4,#8                        \n"
          "vmovl.u8 q4,d4                             \n"
          "vqadd.u16  q3,q3,q4                        \n" //q3->128*(0.34414)

          "vmov.u8   d3,#88                           \n"  //#104->d3
          "vmov.u8   d4,#183                          \n"
          "vmull.u8 q4,d1,d3                          \n"
          "vqshrn.u16 d3,q4,#8                        \n"
          "vmovl.u8 q4,d3                             \n"
          "vmull.u8 q5,d2,d4                          \n"
          "vqshrn.u16 d4,q5,#8                        \n"
          "vmovl.u8 q5,d4                             \n"
          "vqadd.u16 q4,q4,q5                         \n" 

          "vmovl.u8 q2,d0                             \n"    
          "vqadd.u16 q2,q3                            \n"
          "vqsub.u16 q2,q2,q4                         \n"
          "vqmovn.u16 d23,q2                          \n"
          //---------------------DO B-------------------------
          //B = Y - U * -1.77200               
          "vmov.u8   d3,#197                          \n"  //#104->d3
          "vmull.u8  q3,d3,d1                         \n"//0.772u->q3
          "vrshrn.u16 d3,q3,#8                        \n"
          "vmovl.u8 q3,d3                             \n"
          "vmovl.u8  q4,d1                            \n"  //1.0u->q2
          "vadd.u16  q3,q3,q4                         \n" //1.772u

          "vmov.u8   d30,#197                         \n"  //#104->d3
          "vmull.u8  q5,d31,d30\n"//0.402v->q5
          "vrshrn.u16 d4,q5,#8\n"    
          "vmovl.u8 q5,d4\n"    //q5->0.402v
          "vmovl.u8  q4,d31 \n"  //1.0*128->q4
          "vadd.u16  q4,q4,q5                         \n" //1.402*128

           "vmovl.u8  q2,d0                           \n" //y->q2
           
          "vqadd.u16 q2,q3,q2                         \n"
          "vqsub.u16 q2,q2,q4                         \n"
          #if MT_RED==2
          "vqmovn.u16 d22,q2                          \n"
          #endif
          #if MT_RED==0
           "vqmovn.u16 d24,q2                          \n"
          #endif
          //--------------------SUMMARY------------------------

          "subs       %4, %4, #8                      \n"
          "vmov.u8    d25, #255                       \n"
          "vst4.8     {d22,d23,d24,d25}, [%3]!        \n"
          "bgt        1b                              \n"
          : "+r"(src_y),     // %0
            "+r"(src_u),     // %1
            "+r"(src_v),     // %2
            "+r"(dst_argb),  // %3
            "+r"(width)      // %4
          : 
          : "cc", "memory", "q0", "q1", "q2", "q3", "q4",
            "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15","r7","r8"
      );
  }


  void I422ToYUY2JRow_NEON(const uint8* src_y,
	                        const uint8* src_u,
	                        const uint8* src_v,
	                        uint8* dst_yuy2, int width) {
		  asm volatile (
		    ".p2align   2                              \n"
		  "1:                                          \n"
		    MEMACCESS(0)
		    "vld2.8     {d0, d2}, [%0]!                \n"  // load 16 Ys
		    MEMACCESS(1)
		    "vld1.8     {d1}, [%1]!                    \n"  // load 8 Us
		    MEMACCESS(2)
		    "vld1.8     {d3}, [%2]!                    \n"  // load 8 Vs
		    "subs       %4, %4, #16                    \n"  // 16 pixels
		    MEMACCESS(3)
		    "vst4.8     {d0, d1, d2, d3}, [%3]!        \n"  // Store 8 YUY2/16 pixels.
		    "bgt        1b                             \n"
		  : "+r"(src_y),     // %0
		    "+r"(src_u),     // %1
		    "+r"(src_v),     // %2
		    "+r"(dst_yuy2),  // %3
		    "+r"(width)      // %4
		  :
		  : "cc", "memory", "d0", "d1", "d2", "d3"
		  );
	}

	void YUY2JToARGBRow_NEON(const uint8* src_yuy2,
                        uint8* dst_argb,
                        int width) {

			  asm volatile (
                   "vmov.u8       d31,#128                       \n"//128是YUV的偏移量
				  "1:                                          \n"
				    MEMACCESS(0)                                                             
    				    "vld2.8     {d0, d2}, [%0]!                \n"                             
    				    "vmov.u8    d3, d2                         \n"                             
    				    "vuzp.u8    d2, d3                         \n"                             
    				    "vtrn.u32   d2, d3                         \n"
    				  	"vmov.u8 	d22,d2						   \n"

				  	 "vmovl.u8  q10,d22                          \n"
			          //d20=16x4
			          "vshll.u16  q1, d20, #16                    \n"  //Q1是U
			          "vshll.u16  q2, d21, #16                    \n"  //Q2是V

			          "vaddw.u16  q1, q1,d20                      \n"  //EXTEND
			          "vaddw.u16  q2, q2,d21                      \n"
			          "vqmovn.u16 d1,q1                           \n"  //u 
			          "vqmovn.u16 d2,q2                           \n"  //v   
			          //d0是Y d1是U d2是V

			         //------------------------DO R-----------------------------
			          "vmov.u8   d3,#104                          \n" //#104->d3
			          "vmull.u8  q3,d3,d2                         \n" //0.402v->q3
			          "vqshrn.u16 d3,q3,#8                        \n"
			          "vmovl.u8 q3,d3                             \n" //0.402V->Q3
			          "vmovl.u8  q4,d2                            \n" //1.0v->Q4
			          "vadd.u16  q3,q3,q4                         \n" //1.402v->Q3

			          "vmov.u8   d30,#104                         \n" //#104->d30
			          "vmull.u8  q5,d31,d30                       \n" //0.402*128->q5
			          "vqshrn.u16 d4,q5,#8                        \n" //   
			          "vmovl.u8 q5,d4                             \n" //q5->0.402v
			          "vmovl.u8  q4,d31                           \n" //1.0*128->q4
			          "vadd.u16  q4,q4,q5                         \n" //1.402*128

			           "vmovl.u8  q2,d0                           \n" //y->q2

			          "vqadd.u16 q2,q3,q2                         \n" //Y+(Q3=1.402V)
			          "vqsub.u16 q2,q2,q4                         \n" //Y+(Q3=1.402V)-128*1.402
			           #if MT_RED==2
                      "vqmovn.u16 d24,q2                          \n" //RES
                      #endif
                      #if MT_RED==0
                      "vqmovn.u16 d22,q2                           \n"
                      #endif
			          //-----------------------DO G-----------------------------

			          //G = Y - U *  0.34414 - V *  0.71414
			          //0.34414*256=88     0.71414*256=183
			          //
			          "vmov.u8   d3,#88                           \n" //#104->d3
			          "vmov.u8   d4,#183                          \n"
			          "vmull.u8  q3,d3,d31                        \n"//0.402v->q3
			          "vqshrn.u16 d3,q3,#8                        \n"
			          "vmovl.u8 q3,d3\n"
			          "vmull.u8  q4,d4,d31                        \n"//0.402v->q3
			          "vqshrn.u16 d4,q4,#8                        \n"
			          "vmovl.u8 q4,d4                             \n"
			          "vqadd.u16  q3,q3,q4                        \n" //q3->128*(0.34414)

			          "vmov.u8   d3,#88                           \n"  //#104->d3
			          "vmov.u8   d4,#183                          \n"
			          "vmull.u8 q4,d1,d3                          \n"
			          "vqshrn.u16 d3,q4,#8                        \n"
			          "vmovl.u8 q4,d3                             \n"
			          "vmull.u8 q5,d2,d4                          \n"
			          "vqshrn.u16 d4,q5,#8                        \n"
			          "vmovl.u8 q5,d4                             \n"
			          "vqadd.u16 q4,q4,q5                         \n" 

			          "vmovl.u8 q2,d0                             \n"    
			          "vqadd.u16 q2,q3                            \n"
			          "vqsub.u16 q2,q2,q4                         \n"
			          "vqmovn.u16 d23,q2                          \n"
			          //---------------------DO B-------------------------
			          //B = Y - U * -1.77200               
			          "vmov.u8   d3,#197                          \n"  //#104->d3
			          "vmull.u8  q3,d3,d1                         \n"//0.772u->q3
			          "vrshrn.u16 d3,q3,#8                        \n"
			          "vmovl.u8 q3,d3                             \n"
			          "vmovl.u8  q4,d1                            \n"  //1.0u->q2
			          "vadd.u16  q3,q3,q4                         \n" //1.772u

			          "vmov.u8   d30,#197                         \n"  //#104->d3
			          "vmull.u8  q5,d31,d30\n"//0.402v->q5
			          "vrshrn.u16 d4,q5,#8\n"    
			          "vmovl.u8 q5,d4\n"    //q5->0.402v
			          "vmovl.u8  q4,d31 \n"  //1.0*128->q4
			          "vadd.u16  q4,q4,q5                         \n" //1.402*128

			           "vmovl.u8  q2,d0                           \n" //y->q2
			           
			          "vqadd.u16 q2,q3,q2                         \n"
			          "vqsub.u16 q2,q2,q4                         \n"
			            #if MT_RED==2
                          "vqmovn.u16 d22,q2                          \n"
                          #endif
                          #if MT_RED==0
                           "vqmovn.u16 d24,q2                          \n"
                          #endif
			          //--------------------SUMMARY------------------------

			          "subs       %2, %2, #8                      \n"
			          "vmov.u8    d25, #255                       \n"
			          "vst4.8     {d22,d23,d24,d25}, [%1]!        \n"	
				    
				    "bgt        1b                             		\n"
				    : "+r"(src_yuy2),  // %0
				      "+r"(dst_argb),  // %1
				      "+r"(width)      // %2
				    :
				    : "cc", "memory", "q0", "q1", "q2", "q3", "q4",
				      "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
			);
	}

void YUV444ToYUY2Row_NEON(const uint8* src_yuv444,uint8* dst_yuy2,int pix){
		asm volatile (
			".p2align   2                                \n"
			"1:                                          \n"
			//MEMACCESS(0)
			"vld3.u8    {d0,d2,d4}, [%0]!              \n"  // d0->Y  d1->U  d2->V
			"vld3.u8    {d1,d3,d5}, [%0]!              \n"  // d1->Y  d3->U  d5->V
			"vuzp.8     d0,d1                          \n"
			"vpaddl.u8  q1, q1                         \n"  // U 16 bytes -> 8 shorts.
			"vpaddl.u8  q2, q2                         \n"  // V 16 bytes -> 8 shorts.
			"vrshr.u16  q1,q1, #1                      \n"
			"vrshr.u16  q2,q2, #1                      \n"
			"vmov.u8    d6,d1                          \n"
			"vqmovn.u16 d1,q1                          \n"  //u
			"vqmovn.u16 d3,q2                          \n"  //v
			"vmov.u8    d2,d6                          \n"
			"subs      %2, %2, #16                     \n"  // 16 processed per loop.
			//MEMACCESS(1)
			"vst4.u8     {d0,d1,d2,d3}, [%1]!          \n"  // store 16 pixels of Y.
			"bgt        1b                             \n"
			: "+r"(src_yuv444),  // %0
			"+r"(dst_yuy2),     // %1
			"+r"(pix)        // %2
			:
		: "cc", "memory", "q0", "q1" ,"q2","q3" // Clobber List
			);
	}

void YUY2ToYUV444Row_NEON(const uint8* src_yuy2,uint8* dst_yuv444,int pix){
	asm volatile (
		".p2align   2                                \n"
		"1:                                          \n"
		//MEMACCESS(0)
		"vld2.8    {q0, q1}, [%0]!                 \n"  // load 16 pixels of YUY2.
		"vmov.u8   q2,q1                           \n"
		"vtrn.8    q1,q2                           \n" //q1u,q2变成v
		"subs      %2, %2, #16                     \n"  // 16 processed per loop.
		//MEMACCESS(1)
		"vst3.u8     {d0,d2,d4}, [%1]!             \n"  // store 16 pixels of Y.
		"vst3.u8     {d1,d3,d5}, [%1]!             \n"  // store 16 pixels of Y.
		"bgt        1b                             \n"
		: "+r"(src_yuy2),  // %0
		"+r"(dst_yuv444),     // %1
		"+r"(pix)        // %2
		:
	: "cc", "memory", "q0", "q1" ,"q2"  // Clobber List
		);
}

#endif



