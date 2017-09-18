#include "FormatCvt.h"
#include "FormatCvt_row.h"
#include "FormatCvt_row_define.h"


#if defined(HAVE_NEON)&&defined(HAVE_NEON64)

	#ifdef HAS_MERGEUVROW_NEON
	void MergeUVRow_NEON(const uint8* src_u, const uint8* src_v, uint8* dst_uv,
	                     int width) {
	  asm volatile (
	  "1:                                          \n"
	    MEMACCESS(0)
	    "ld1        {v0.16b}, [%0], #16            \n"  // load U
	    MEMACCESS(1)
	    "ld1        {v1.16b}, [%1], #16            \n"  // load V
	    "subs       %w3, %w3, #16                  \n"  // 16 processed per loop
	    MEMACCESS(2)
	    "st2        {v0.16b,v1.16b}, [%2], #32     \n"  // store 16 pairs of UV
	    "b.gt       1b                             \n"
	    :
	      "+r"(src_u),   // %0
	      "+r"(src_v),   // %1
	      "+r"(dst_uv),  // %2
	      "+r"(width)    // %3  // Output registers
	    :                       // Input registers
	    : "cc", "memory", "v0", "v1"  // Clobber List
	  );
	}
	#endif  // HAS_MERGEUVROW_NEON

	#define RGBTOUV_SETUP_REG                                                      \
    "movi       v20.8h, #56, lsl #0  \n"  /* UB/VR coefficient (0.875) / 2 */  \
    "movi       v21.8h, #37, lsl #0  \n"  /* UG coefficient (-0.5781) / 2  */  \
    "movi       v22.8h, #19, lsl #0  \n"  /* UR coefficient (-0.2969) / 2  */  \
    "movi       v23.8h, #9,  lsl #0  \n"  /* VB coefficient (-0.1406) / 2  */  \
    "movi       v24.8h, #47, lsl #0  \n"  /* VG coefficient (-0.7344) / 2  */  \
    "movi       v25.16b, #0x80       \n"  /* 128.5 (0x8080 in 16-bit)      */

  // 16x2 pixels -> 8x1.  pix is number of argb pixels. e.g. 16.
  #define RGBTOUV(QB, QG, QR) \
    "mul        v3.8h, " #QB ",v20.8h          \n"  /* B                    */ \
    "mul        v4.8h, " #QR ",v20.8h          \n"  /* R                    */ \
    "mls        v3.8h, " #QG ",v21.8h          \n"  /* G                    */ \
    "mls        v4.8h, " #QG ",v24.8h          \n"  /* G                    */ \
    "mls        v3.8h, " #QR ",v22.8h          \n"  /* R                    */ \
    "mls        v4.8h, " #QB ",v23.8h          \n"  /* B                    */ \
    "add        v3.8h, v3.8h, v25.8h           \n"  /* +128 -> unsigned     */ \
    "add        v4.8h, v4.8h, v25.8h           \n"  /* +128 -> unsigned     */ \
    "uqshrn     v0.8b, v3.8h, #8               \n"  /* 16 bit to 8 bit U    */ \
    "uqshrn     v1.8b, v4.8h, #8               \n"  /* 16 bit to 8 bit V    */

   void ARGBToUVRow_NEON(const uint8* src_argb, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int pix) {
			  const uint8* src_argb_1 = src_argb + src_stride_argb;
			  asm volatile (
			    RGBTOUV_SETUP_REG
			  "1:                                          \n"
			    MEMACCESS(0)
			    "ld4        {v0.16b,v1.16b,v2.16b,v3.16b}, [%0], #64 \n"  // load 16 pixels.
			    "uaddlp     v0.8h, v0.16b                  \n"  // B 16 bytes -> 8 shorts.
			    "uaddlp     v1.8h, v1.16b                  \n"  // G 16 bytes -> 8 shorts.
			    "uaddlp     v2.8h, v2.16b                  \n"  // R 16 bytes -> 8 shorts.

			    MEMACCESS(1)
			    "ld4        {v4.16b,v5.16b,v6.16b,v7.16b}, [%1], #64 \n"  // load next 16
			    "uadalp     v0.8h, v4.16b                  \n"  // B 16 bytes -> 8 shorts.
			    "uadalp     v1.8h, v5.16b                  \n"  // G 16 bytes -> 8 shorts.
			    "uadalp     v2.8h, v6.16b                  \n"  // R 16 bytes -> 8 shorts.

			    "urshr      v0.8h, v0.8h, #1               \n"  // 2x average
			    "urshr      v1.8h, v1.8h, #1               \n"
			    "urshr      v2.8h, v2.8h, #1               \n"

			    "subs       %w4, %w4, #16                  \n"  // 32 processed per loop.
                #if MT_RED==2
			    RGBTOUV(v0.8h, v1.8h, v2.8h)
                #endif
                #if MT_RED==0
                RGBTOUV(v2.8h, v1.8h, v0.8h)
                #endif
			    MEMACCESS(2)
			    "st1        {v0.8b}, [%2], #8              \n"  // store 8 pixels U.
			    MEMACCESS(3)
			    "st1        {v1.8b}, [%3], #8              \n"  // store 8 pixels V.
			    "b.gt       1b                             \n"
			  : "+r"(src_argb),  // %0
			    "+r"(src_argb_1),  // %1
			    "+r"(dst_u),     // %2
			    "+r"(dst_v),     // %3
			    "+r"(pix)        // %4
			  :
			  : "cc", "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7",
			    "v20", "v21", "v22", "v23", "v24", "v25"
			  );
	}
    void ARGBToYJRow_NEON(const uint8* src_argb, uint8* dst_y, int pix) {
          asm volatile (
            "movi       v4.8b, #15                     \n"  // B * 0.11400 coefficient
            "movi       v5.8b, #75                     \n"  // G * 0.58700 coefficient
            "movi       v6.8b, #38                     \n"  // R * 0.29900 coefficient
          "1:                                          \n"
            MEMACCESS(0)
            "ld4        {v0.8b,v1.8b,v2.8b,v3.8b}, [%0], #32 \n"  // load 8 ARGB pixels.
            "subs       %w2, %w2, #8                   \n"  // 8 processed per loop.
            #if MT_RED==2
            "umull      v3.8h, v0.8b, v4.8b            \n"  // B
            "umlal      v3.8h, v1.8b, v5.8b            \n"  // G
            "umlal      v3.8h, v2.8b, v6.8b            \n"  // R
            #endif
            #if MT_RED==0
            "umull      v3.8h, v2.8b, v4.8b            \n"  // B
            "umlal      v3.8h, v1.8b, v5.8b            \n"  // G
            "umlal      v3.8h, v0.8b, v6.8b            \n"  // R
            #endif
            "sqrshrun   v0.8b, v3.8h, #7               \n"  // 15 bit to 8 bit Y
            MEMACCESS(1)
            "st1        {v0.8b}, [%1], #8              \n"  // store 8 pixels Y.
            "b.gt       1b                             \n"
          : "+r"(src_argb),  // %0
            "+r"(dst_y),     // %1
            "+r"(pix)        // %2
          :
          : "cc", "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6"
          );
    }
    void ARGBToUV422Row_NEON(const uint8* src_argb,
                              uint8* dst_u, 
                              uint8* dst_v,
                              int pix) {
          asm volatile (
            RGBTOUV_SETUP_REG
          "1:                                          \n"
            MEMACCESS(0)
            "ld4        {v0.16b,v1.16b,v2.16b,v3.16b}, [%0], #64 \n"  // load 16 pixels.

            "uaddlp     v0.8h, v0.16b                  \n"  // B 16 bytes -> 8 shorts.
            "uaddlp     v1.8h, v1.16b                  \n"  // G 16 bytes -> 8 shorts.
            "uaddlp     v2.8h, v2.16b                  \n"  // R 16 bytes -> 8 shorts.

            "subs       %w3, %w3, #16                  \n"  // 16 processed per loop.
            #if MT_RED==2
            "mul        v3.8h, v0.8h, v20.8h           \n"  // B
            "mls        v3.8h, v1.8h, v21.8h           \n"  // G
            "mls        v3.8h, v2.8h, v22.8h           \n"  // R
            #endif
            #if MT_RED==0
            "mul        v3.8h, v2.8h, v20.8h           \n"  // B
            "mls        v3.8h, v1.8h, v21.8h           \n"  // G
            "mls        v3.8h, v0.8h, v22.8h           \n"  // R
            #endif
            "add        v3.8h, v3.8h, v25.8h           \n"  // +128 -> unsigned

            #if MT_RED==2
            "mul        v4.8h, v2.8h, v20.8h           \n"  // R
            "mls        v4.8h, v1.8h, v24.8h           \n"  // G
            "mls        v4.8h, v0.8h, v23.8h           \n"  // B
            #endif
            #if MT_RED==0
            "mul        v4.8h, v0.8h, v20.8h           \n"  // R
            "mls        v4.8h, v1.8h, v24.8h           \n"  // G
            "mls        v4.8h, v2.8h, v23.8h           \n"  // B
            #endif
            "add        v4.8h, v4.8h, v25.8h           \n"  // +128 -> unsigned

            "uqshrn     v0.8b, v3.8h, #8               \n"  // 16 bit to 8 bit U
            "uqshrn     v1.8b, v4.8h, #8               \n"  // 16 bit to 8 bit V

            MEMACCESS(1)
            "st1        {v0.8b}, [%1], #8              \n"  // store 8 pixels U.
            MEMACCESS(2)
            "st1        {v1.8b}, [%2], #8              \n"  // store 8 pixels V.
            "b.gt       1b                             \n"
          : "+r"(src_argb),  // %0
            "+r"(dst_u),     // %1
            "+r"(dst_v),     // %2
            "+r"(pix)        // %3
          :
          : "cc", "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7",
            "v20", "v21", "v22", "v23", "v24", "v25"
          );
    }

    #define YUV422TORGB_SETUP_REG                                                  \
    "ld1r       {v24.8h}, [%[kUVBiasBGR]], #2  \n"                             \
    "ld1r       {v25.8h}, [%[kUVBiasBGR]], #2  \n"                             \
    "ld1r       {v26.8h}, [%[kUVBiasBGR]]      \n"                             \
    "ld1r       {v31.4s}, [%[kYToRgb]]         \n"                             \
    "movi       v27.8h, #128                   \n"                             \
    "movi       v28.8h, #102                   \n"                             \
    "movi       v29.8h, #25                    \n"                             \
    "movi       v30.8h, #52                    \n"

    #define READYUV422                                                             \
    MEMACCESS(0)                                                               \
    "ld1        {v0.8b}, [%0], #8              \n"                             \
    MEMACCESS(1)                                                               \
    "ld1        {v1.s}[0], [%1], #4            \n"                             \
    MEMACCESS(2)                                                               \
    "ld1        {v1.s}[1], [%2], #4            \n"
    
    #define YUV422TORGB(vR, vG, vB)                                                \
    "uxtl       v0.8h, v0.8b                   \n" /* Extract Y    */          \
    "shll       v2.8h, v1.8b, #8               \n" /* Replicate UV */          \
    "ushll2     v3.4s, v0.8h, #0               \n" /* Y */                     \
    "ushll      v0.4s, v0.4h, #0               \n"                             \
    "mul        v3.4s, v3.4s, v31.4s           \n"                             \
    "mul        v0.4s, v0.4s, v31.4s           \n"                             \
    "sqshrun    v0.4h, v0.4s, #16              \n"                             \
    "sqshrun2   v0.8h, v3.4s, #16              \n" /* Y */                     \
    "uaddw      v1.8h, v2.8h, v1.8b            \n" /* Replicate UV */          \
    "mov        v2.d[0], v1.d[1]               \n" /* Extract V */             \
    "uxtl       v2.8h, v2.8b                   \n"                             \
    "uxtl       v1.8h, v1.8b                   \n" /* Extract U */             \
    "mul        v3.8h, v1.8h, v27.8h           \n"                             \
    "mul        v5.8h, v1.8h, v29.8h           \n"                             \
    "mul        v6.8h, v2.8h, v30.8h           \n"                             \
    "mul        v7.8h, v2.8h, v28.8h           \n"                             \
    "sqadd      v6.8h, v6.8h, v5.8h            \n"                             \
    "sqadd      " #vB ".8h, v24.8h, v0.8h      \n" /* B */                     \
    "sqadd      " #vG ".8h, v25.8h, v0.8h      \n" /* G */                     \
    "sqadd      " #vR ".8h, v26.8h, v0.8h      \n" /* R */                     \
    "sqadd      " #vB ".8h, " #vB ".8h, v3.8h  \n" /* B */                     \
    "sqsub      " #vG ".8h, " #vG ".8h, v6.8h  \n" /* G */                     \
    "sqadd      " #vR ".8h, " #vR ".8h, v7.8h  \n" /* R */                     \
    "sqshrun    " #vB ".8b, " #vB ".8h, #6     \n" /* B */                     \
    "sqshrun    " #vG ".8b, " #vG ".8h, #6     \n" /* G */                     \
    "sqshrun    " #vR ".8b, " #vR ".8h, #6     \n" /* R */                     

    // YUV to RGB conversion constants.
    // Y contribution to R,G,B.  Scale and bias.
    #define YG 18997 /* round(1.164 * 64 * 256 * 256 / 257) */
    #define YGB 1160 /* 1.164 * 64 * 16 - adjusted for even error distribution */

    // U and V contributions to R,G,B.
    #define UB -128 /* -min(128, round(2.018 * 64)) */
    #define UG 25 /* -round(-0.391 * 64) */
    #define VG 52 /* -round(-0.813 * 64) */
    #define VR -102 /* -round(1.596 * 64) */

    // Bias values to subtract 16 from Y and 128 from U and V.
    #define BB (UB * 128            - YGB)
    #define BG (UG * 128 + VG * 128 - YGB)
    #define BR            (VR * 128 - YGB)

    static vec16 kUVBiasBGR = { BB, BG, BR, 0, 0, 0, 0, 0 };
    static vec32 kYToRgb = { 0x0101 * YG, 0, 0, 0 };

    #undef YG
    #undef YGB
    #undef UB
    #undef UG
    #undef VG
    #undef VR
    #undef BB
    #undef BG
    #undef BR

    void I422ToARGBRow_NEON(const uint8* src_y,
                        const uint8* src_u,
                        const uint8* src_v,
                        uint8* dst_argb,
                        int width) {
          asm volatile (
            YUV422TORGB_SETUP_REG
          "1:                                          \n"
            READYUV422
            YUV422TORGB(v22, v21, v20)
            "subs       %w4, %w4, #8                   \n"
            "movi       v23.8b, #255                   \n" /* A */
            MEMACCESS(3)
            #if MT_RED==0
                "mov v30.8b,v22.8b\n"
                "mov v22.8b,v20.8b\n"
                "mov v20.8b,v30.8b\n"
            #endif
            "st4        {v20.8b,v21.8b,v22.8b,v23.8b}, [%3], #32     \n"
            "b.gt       1b                             \n"
            : "+r"(src_y),     // %0
              "+r"(src_u),     // %1
              "+r"(src_v),     // %2
              "+r"(dst_argb),  // %3
              "+r"(width)      // %4
            : [kUVBiasBGR]"r"(&kUVBiasBGR),
              [kYToRgb]"r"(&kYToRgb)
            : "cc", "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v20",
              "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29", "v30"
          );
  }
  // Read 8 Y and 4 UV from NV12
	#define READNV12                                                               \
    MEMACCESS(0)                                                               \
    "ld1        {v0.8b}, [%0], #8              \n"                             \
    MEMACCESS(1)                                                               \
    "ld1        {v2.8b}, [%1], #8              \n"                             \
    "uzp1       v1.8b, v2.8b, v2.8b            \n"                             \
    "uzp2       v3.8b, v2.8b, v2.8b            \n"                             \
    "ins        v1.s[1], v3.s[0]               \n"
	#ifdef HAS_NV12TOARGBROW_NEON
	void NV12ToARGBRow_NEON(const uint8* src_y,
	                        const uint8* src_uv,
	                        uint8* dst_argb,
	                        int width) {
	  asm volatile (
	    YUV422TORGB_SETUP_REG
	  "1:                                          \n"
	    READNV12
	    YUV422TORGB(v22, v21, v20)
	    "subs       %w3, %w3, #8                   \n"
	    "movi       v23.8b, #255                   \n"
	    MEMACCESS(2)
         #if MT_RED==0
            "mov v30.8b,v22.8b\n"
            "mov v22.8b,v20.8b\n"
            "mov v20.8b,v30.8b\n"
        #endif
	    "st4        {v20.8b,v21.8b,v22.8b,v23.8b}, [%2], #32     \n"
	    "b.gt       1b                             \n"
	    : "+r"(src_y),     // %0
	      "+r"(src_uv),    // %1
	      "+r"(dst_argb),  // %2
	      "+r"(width)      // %3
	    : [kUVBiasBGR]"r"(&kUVBiasBGR),
	      [kYToRgb]"r"(&kYToRgb)
	    : "cc", "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v20",
	      "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29", "v30"
	  );
	}
	#endif  // HAS_NV12TOARGBROW_NEON


	#define READNV21                                                               \
    MEMACCESS(0)                                                               \
    "ld1        {v0.8b}, [%0], #8              \n"                             \
    MEMACCESS(1)                                                               \
    "ld1        {v2.8b}, [%1], #8              \n"                             \
    "uzp1       v3.8b, v2.8b, v2.8b            \n"                             \
    "uzp2       v1.8b, v2.8b, v2.8b            \n"                             \
    "ins        v1.s[1], v3.s[0]               \n"

	#ifdef HAS_NV21TOARGBROW_NEON
		void NV21ToARGBRow_NEON(const uint8* src_y,
		                        const uint8* src_uv,
		                        uint8* dst_argb,
		                        int width) {
		  asm volatile (
		    YUV422TORGB_SETUP_REG
		  "1:                                          \n"
		    READNV21
		    YUV422TORGB(v22, v21, v20)
		    "subs       %w3, %w3, #8                   \n"
		    "movi       v23.8b, #255                   \n"
		    MEMACCESS(2)
             #if MT_RED==0
                "mov v30.8b,v22.8b\n"
                "mov v22.8b,v20.8b\n"
                "mov v20.8b,v30.8b\n"
            #endif
		    "st4        {v20.8b,v21.8b,v22.8b,v23.8b}, [%2], #32     \n"
		    "b.gt       1b                             \n"
		    : "+r"(src_y),     // %0
		      "+r"(src_uv),    // %1
		      "+r"(dst_argb),  // %2
		      "+r"(width)      // %3
		    : [kUVBiasBGR]"r"(&kUVBiasBGR),
		      [kYToRgb]"r"(&kYToRgb)
		    : "cc", "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v20",
		      "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29", "v30"
		  );
		}
	#endif  // HAS_NV21TOARGBROW_NEON

	 




    void ARGBToUVJRow_NEON(const uint8* src_argb, int src_stride_argb,
                       uint8* dst_u, uint8* dst_v, int pix) {
        const uint8* src_argb_1 = src_argb + src_stride_argb;
        asm volatile (
          "movi       v20.8h, #127, lsl #0            \n"  // UB/VR coeff (0.500) / 2
          "movi       v21.8h, #84, lsl #0            \n"  // UG coeff (-0.33126) / 2
          "movi       v22.8h, #43, lsl #0            \n"  // UR coeff (-0.16874) / 2
          "movi       v23.8h, #20, lsl #0            \n"  // VB coeff (-0.08131) / 2
          "movi       v24.8h, #107, lsl #0            \n"  // VG coeff (-0.41869) / 2
          "movi       v25.16b, #0x80                 \n"  // 128.5 (0x8080 in 16-bit)
        "1:                                          \n"
          "ld4        {v0.16b,v1.16b,v2.16b,v3.16b}, [%0], #64 \n"  // load 16 pixels.
          "uaddlp     v0.8h, v0.16b                  \n"  // B 16 bytes -> 8 shorts.
          "uaddlp     v1.8h, v1.16b                  \n"  // G 16 bytes -> 8 shorts.
          "uaddlp     v2.8h, v2.16b                  \n"  // R 16 bytes -> 8 shorts.

          "ld4        {v4.16b,v5.16b,v6.16b,v7.16b}, [%1], #64  \n"  // load next 16
          "uadalp     v0.8h, v4.16b                  \n"  // B 16 bytes -> 8 shorts.
          "uadalp     v1.8h, v5.16b                  \n"  // G 16 bytes -> 8 shorts.
          "uadalp     v2.8h, v6.16b                  \n"  // R 16 bytes -> 8 shorts.

          "urshr      v0.8h, v0.8h, #2               \n"  // 2x average
          "urshr      v1.8h, v1.8h, #2               \n"
          "urshr      v2.8h, v2.8h, #2               \n"

          "subs       %w4, %w4, #16                  \n"  // 32 processed per loop.
          #if MT_RED==2
          RGBTOUV(v0.8h, v1.8h, v2.8h)
          #endif
          #if MT_RED==0
           RGBTOUV(v2.8h, v1.8h, v0.8h)
          #endif
          "st1        {v0.8b}, [%2], #8              \n"  // store 8 pixels U.

          "st1        {v1.8b}, [%3], #8              \n"  // store 8 pixels V.
          "b.gt       1b                             \n"
        : "+r"(src_argb),  // %0
          "+r"(src_argb_1),  // %1
          "+r"(dst_u),     // %2
          "+r"(dst_v),     // %3
          "+r"(pix)        // %4
        :
        : "cc", "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7",
          "v20", "v21", "v22", "v23", "v24", "v25"
        );
    }
  void ARGBToUVJ422Row_NEON(const uint8* src_argb,
                       uint8* dst_u, uint8* dst_v, int pix) {
      int x=1;
      asm volatile (
        
        "movi       v20.8h, #127, lsl #0            \n"  // UB/VR coeff (0.500) / 2
        "movi       v21.8h, #84, lsl #0            \n"  // UG coeff (-0.33126) / 2
        "movi       v22.8h, #43, lsl #0            \n"  // UR coeff (-0.16874) / 2
        "movi       v23.8h, #20, lsl #0            \n"  // VB coeff (-0.08131) / 2
        "movi       v24.8h, #107, lsl #0            \n"  // VG coeff (-0.41869) / 2
        "movi       v25.16b, #0x80                 \n"  // 128.5 (0x8080 in 16-bit)
      "1:                                          \n"

        "ld4        {v0.16b,v1.16b,v2.16b,v3.16b}, [%0], #64 \n"  // load 16 pixels.
        "uaddlp     v0.8h, v0.16b                  \n"  // B 16 bytes -> 8 shorts.
        "uaddlp     v1.8h, v1.16b                  \n"  // G 16 bytes -> 8 shorts.
        "uaddlp     v2.8h, v2.16b                  \n"  // R 16 bytes -> 8 shorts.
     
        "urshr      v0.8h, v0.8h, #1               \n"  // 2x average
        "urshr      v1.8h, v1.8h, #1               \n"
        "urshr      v2.8h, v2.8h, #1               \n"

        "subs       %w4, %w4, #16                  \n"  // 32 processed per loop.
        #if MT_RED==2
         RGBTOUV(v0.8h, v1.8h, v2.8h)
        #endif
        #if MT_RED==0
         RGBTOUV(v2.8h, v1.8h, v0.8h)
        #endif
        "st1        {v0.8b}, [%2], #8              \n"  // store 8 pixels U.

        "st1        {v1.8b}, [%3], #8              \n"  // store 8 pixels V.
        "b.gt       1b                             \n"
      : "+r"(src_argb),  // %0
        "+r"(x),  // %1
        "+r"(dst_u),     // %2
        "+r"(dst_v),     // %3
        "+r"(pix)        // %4
      :
      : "cc", "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7","v20", "v21", "v22", "v23", "v24", "v25"
      );
  }

 
  void J422ToARGBRow_NEON(const uint8* src_y,
                        const uint8* src_u,
                        const uint8* src_v,
                        uint8* dst_argb,
                        int width) {
      asm volatile (
          "movi       v20.8b,#128                     \n"
        "1:                                           \n"
          "ld1 {v0.8b},[%0],#8                        \n"   //Y

          "ld1      {v1.s}[0],[%1],#4                 \n"   //U
          "ld1      {v1.s}[1],[%2],#4                 \n"   //V
       

          "shll     v2.8h, v1.8b,#8                   \n"   /* Replicate UV */  
          "uaddw    v1.8h, v2.8h, v1.8b               \n"   /* Replicate UV */  
          "mov      v2.d[0], v1.d[1]                  \n"   /* Extract V */  
        
          //---------------------------R---------------------------------
          //R = Y - V * -1.40200
          "movi     v3.8b,#104                        \n"   //#104->d3
          "umull    v4.8h,v2.8b,v3.8b                 \n"   //0.402v->q3
          "uqshrn   v3.8b,v4.8h,#8                    \n"   //v
          "ushll    v3.8h,v3.8b,#0                    \n"   // 0.402v
          "ushll    v4.8h,v2.8b,#0                    \n"   //1v
          "uqadd    v3.8h,v4.8h,v3.8h                 \n"   //1.402V


          "movi     v30.8b,#104                       \n"   //#104->d3
          "umull    v5.8h,v20.8b,v30.8b               \n"   //#104*128
          "uqshrn   v5.8b,v5.8h,#8                    \n"   //>>8

          "ushll    v5.8h,v5.8b,#0                    \n"   //vmovl
          "ushll    v4.8h,v20.8b,#0                   \n"   //vmovl

          "uqadd    v4.8h,v4.8h,v5.8h                 \n"   //1.402*128

          "ushll    v5.8h,v0.8b,#0                    \n"   //Y->v5.8h

          "uqadd    v3.8h,v3.8h,v5.8h                 \n"   //Y+1.402*V
          "uqsub    v3.8h,v3.8h,v4.8h                 \n"   //Y+1.402*V-1.402*128
          #if MT_RED==2
          "uqxtn    v24.8b,v3.8h                      \n"   //TO .8B
          #endif
          #if MT_RED==0
          "uqxtn    v22.8b,v3.8h                      \n"
          #endif
          //---------------------------G-----------------------------

          //G = Y - U *  0.34414 - V *  0.71414
          //0.34414*256=88     0.71414*256=183
          "movi     v3.8b,#88                         \n"
          "movi     v4.8b,#183                        \n"
          "umull    v5.8h,v3.8b,v20.8b                \n"   //128*#88
          "uqshrn   v5.8b,v5.8h,#8                    \n"   //128*#88>>8=128*0.34414
          "ushll    v5.8h,v5.8b,#0                    \n"   //vmovl
          "umull    v6.8h,v4.8b,v20.8b                \n"   //128*#183
          "uqshrn   v6.8b,v6.8h,#8                    \n"   //128*#183>>8
          "ushll    v6.8h,v6.8b,#0                    \n"   //vmovl
          "uqadd    v5.8h,v5.8h,v6.8h                 \n"   //(0.34414+0.71414)*128


          "umull    v6.8h,v3.8b,v1.8b                 \n"   //U*#88
          "uqshrn   v6.8b,v6.8h,#8                    \n"   //U*#88>>8=U*0.34414
          "ushll    v6.8h,v6.8b,#0                    \n"   //U->vmovl
          "umull    v7.8h,v4.8b,v2.8b                 \n"   //V*#183
          "uqshrn   v7.8b,v7.8h,#8                    \n"   //V*#183>>8
          "ushll    v7.8h,v7.8b,#0                    \n"   //V->vmovl
          "uqadd    v6.8h,v6.8h,v7.8h                 \n"   //0.34414*U+0.71414*V


          "ushll    v3.8h,v0.8b,#0                    \n"   //Y
          "uqadd    v3.8h,v3.8h,v5.8h                 \n"   //Y+128' 
          "uqsub    v3.8h,v3.8h,v6.8h                 \n"   //Y+128'-U'-Y' 
          "uqxtn    v23.8b,v3.8h                      \n"   //RES->v23.8b
        //----------------------------B-----------------------------
          //B = Y - U * -1.77200  
          "movi     v3.8b,#197                        \n"   //#104->d3
          "umull    v4.8h,v3.8b,v1.8b                 \n"   //U*#104
          "uqshrn   v4.8b,v4.8h,#8                    \n"   //U*#104>>8
          "ushll    v4.8h,v4.8b,#0                    \n"   //U*0.772->vmovl
          "ushll    v5.8h,v1.8b,#0                    \n"   //U->vmovl
          "uqadd    v4.8h,v4.8h,v5.8h                 \n"   //1.772U


          "umull    v5.8h,v3.8b,v20.8b                \n"   //#104*128
          "uqshrn   v5.8b,v5.8h,#8                    \n"   //#104*128>>8
          "ushll    v5.8h,v5.8b,#0                    \n"   //0.772*128->vmovl
          "ushll    v6.8h,v20.8b,#0                   \n"   //128->vmovl
          "uqadd    v5.8h,v5.8h,v6.8h                 \n"   //1.772*128



          "ushll    v3.8h,v0.8b,#0                    \n"   //Y
          "uqadd    v3.8h,v3.8h,v4.8h                 \n"   //Y+1.772*U
          "uqsub    v3.8h,v3.8h,v5.8h                 \n"   //Y+1.772*U-1.772*128
          #if MT_RED==2
          "uqxtn    v22.8b,v3.8h                      \n"   //RES->v22.8b
          #endif
          #if MT_RED==0
          "uqxtn    v24.8b,v3.8h                      \n"
          #endif
          //------------------------SUMMARY---------------------------

          "subs       %w4, %w4, #8                    \n"   //
          "movi       v25.8b,#255                     \n"   //
          "st4        {v22.8b,v23.8b,v24.8b,v25.8b}, [%3], #32 \n"  // store 8 ARGB pixels
          "b.gt       1b                              \n"   //
          : "+r"(src_y),     // %0
            "+r"(src_u),     // %1
            "+r"(src_v),     // %2
            "+r"(dst_argb),  // %3
            "+r"(width)      // %4
          : 
          : "cc", "memory", "v0", "v1", "v2", "v3", "v4",
            "v5", "v6","v7","v22", "v23", "v24", "v25", "v20"
      );
  }
	#ifdef HAS_ARGBTOYROW_NEON
	void ARGBToYRow_NEON(const uint8* src_argb, uint8* dst_y, int pix) {
	  asm volatile (
	    "movi       v4.8b, #13                     \n"  // B * 0.1016 coefficient
	    "movi       v5.8b, #65                     \n"  // G * 0.5078 coefficient
	    "movi       v6.8b, #33                     \n"  // R * 0.2578 coefficient
	    "movi       v7.8b, #16                     \n"  // Add 16 constant
	  "1:                                          \n"
	    MEMACCESS(0)
	    "ld4        {v0.8b,v1.8b,v2.8b,v3.8b}, [%0], #32 \n"  // load 8 ARGB pixels.
	    "subs       %w2, %w2, #8                   \n"  // 8 processed per loop.
        #if MT_RED==2
	    "umull      v3.8h, v0.8b, v4.8b            \n"  // B
	    "umlal      v3.8h, v1.8b, v5.8b            \n"  // G
	    "umlal      v3.8h, v2.8b, v6.8b            \n"  // R
        #endif
        #if MT_RED==0
        "umull      v3.8h, v2.8b, v4.8b            \n"  // B
        "umlal      v3.8h, v1.8b, v5.8b            \n"  // G
        "umlal      v3.8h, v0.8b, v6.8b            \n"  // R
        #endif
	    "sqrshrun   v0.8b, v3.8h, #7               \n"  // 16 bit to 8 bit Y
	    "uqadd      v0.8b, v0.8b, v7.8b            \n"
	    MEMACCESS(1)
	    "st1        {v0.8b}, [%1], #8              \n"  // store 8 pixels Y.
	    "b.gt       1b                             \n"
	  : "+r"(src_argb),  // %0
	    "+r"(dst_y),     // %1
	    "+r"(pix)        // %2
	  :
	  : "cc", "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7"
	  );
	}
	#endif  // HAS_ARGBTOYROW_NEON

	#ifdef HAS_I422TOYUY2JROW_NEON
	void I422ToYUY2JRow_NEON(const uint8* src_y,
                        const uint8* src_u,
                        const uint8* src_v,
                        uint8* dst_yuy2, int width) {
			  asm volatile (
			  "1:                                          \n"
			    MEMACCESS(0)
			    "ld2        {v0.8b, v1.8b}, [%0], #16      \n"  // load 16 Ys
			    "orr        v2.8b, v1.8b, v1.8b            \n"
			    MEMACCESS(1)
			    "ld1        {v1.8b}, [%1], #8              \n"  // load 8 Us
			    MEMACCESS(2)
			    "ld1        {v3.8b}, [%2], #8              \n"  // load 8 Vs
			    "subs       %w4, %w4, #16                  \n"  // 16 pixels
			    MEMACCESS(3)
			    "st4        {v0.8b,v1.8b,v2.8b,v3.8b}, [%3], #32 \n"  // Store 16 pixels.
			    "b.gt       1b                             \n"
			  : "+r"(src_y),     // %0
			    "+r"(src_u),     // %1
			    "+r"(src_v),     // %2
			    "+r"(dst_yuy2),  // %3
			    "+r"(width)      // %4
			  :
			  : "cc", "memory", "v0", "v1", "v2", "v3"
			  );
		}
	#endif  // HAS_I422TOYUY2JROW_NEON

	#ifdef HAS_YUY2JTOARGBROW_NEON
	void YUY2JToARGBRow_NEON(const uint8* src_yuy2,
	                        uint8* dst_argb,
	                        int width) {
		  int64 width64 = (int64)(width);
		  asm volatile (
		    "movi       v20.8b,#128                     \n"   
		  "1:                                          \n"
			MEMACCESS(0)                                                             
		    "ld2        {v0.8b, v1.8b}, [%0], #16      \n"  //V[0].8b->Y                            
		    "uzp2       v3.8b, v1.8b, v1.8b            \n"                             
		    "uzp1       v1.8b, v1.8b, v1.8b            \n"                             
		    "ins        v1.s[1], v3.s[0]               \n" //v1.s[0]->U v1.s[1]->V

		     

		  "shll     v2.8h, v1.8b,#8                   \n"   /* Replicate UV */  
          "uaddw    v1.8h, v2.8h, v1.8b               \n"   /* Replicate UV */  
          "mov      v2.d[0], v1.d[1]                  \n"   /* Extract V */  
        
          //---------------------------R---------------------------------
          //R = Y - V * -1.40200
          "movi     v3.8b,#104                        \n"   //#104->d3
          "umull    v4.8h,v2.8b,v3.8b                 \n"   //0.402v->q3
          "uqshrn   v3.8b,v4.8h,#8                    \n"   //v
          "ushll    v3.8h,v3.8b,#0                    \n"   // 0.402v
          "ushll    v4.8h,v2.8b,#0                    \n"   //1v
          "uqadd    v3.8h,v4.8h,v3.8h                 \n"   //1.402V


          "movi     v30.8b,#104                       \n"   //#104->d3
          "umull    v5.8h,v20.8b,v30.8b               \n"   //#104*128
          "uqshrn   v5.8b,v5.8h,#8                    \n"   //>>8

          "ushll    v5.8h,v5.8b,#0                    \n"   //vmovl
          "ushll    v4.8h,v20.8b,#0                   \n"   //vmovl

          "uqadd    v4.8h,v4.8h,v5.8h                 \n"   //1.402*128

          "ushll    v5.8h,v0.8b,#0                    \n"   //Y->v5.8h

          "uqadd    v3.8h,v3.8h,v5.8h                 \n"   //Y+1.402*V
          "uqsub    v3.8h,v3.8h,v4.8h                 \n"   //Y+1.402*V-1.402*128
           #if MT_RED==2
          "uqxtn    v24.8b,v3.8h                      \n"   //TO .8B
          #endif
          #if MT_RED==0
          "uqxtn    v22.8b,v3.8h                      \n"
          #endif

          //---------------------------V-----------------------------

          //G = Y - U *  0.34414 - V *  0.71414
          //0.34414*256=88     0.71414*256=183
          "movi     v3.8b,#88                         \n"
          "movi     v4.8b,#183                        \n"
          "umull    v5.8h,v3.8b,v20.8b                \n"   //128*#88
          "uqshrn   v5.8b,v5.8h,#8                    \n"   //128*#88>>8=128*0.34414
          "ushll    v5.8h,v5.8b,#0                    \n"   //vmovl
          "umull    v6.8h,v4.8b,v20.8b                \n"   //128*#183
          "uqshrn   v6.8b,v6.8h,#8                    \n"   //128*#183>>8
          "ushll    v6.8h,v6.8b,#0                    \n"   //vmovl
          "uqadd    v5.8h,v5.8h,v6.8h                 \n"   //(0.34414+0.71414)*128


          "umull    v6.8h,v3.8b,v1.8b                 \n"   //U*#88
          "uqshrn   v6.8b,v6.8h,#8                    \n"   //U*#88>>8=U*0.34414
          "ushll    v6.8h,v6.8b,#0                    \n"   //U->vmovl
          "umull    v7.8h,v4.8b,v2.8b                 \n"   //V*#183
          "uqshrn   v7.8b,v7.8h,#8                    \n"   //V*#183>>8
          "ushll    v7.8h,v7.8b,#0                    \n"   //V->vmovl
          "uqadd    v6.8h,v6.8h,v7.8h                 \n"   //0.34414*U+0.71414*V


          "ushll    v3.8h,v0.8b,#0                    \n"   //Y
          "uqadd    v3.8h,v3.8h,v5.8h                 \n"   //Y+128' 
          "uqsub    v3.8h,v3.8h,v6.8h                 \n"   //Y+128'-U'-Y' 
          "uqxtn    v23.8b,v3.8h                      \n"   //RES->v23.8b
        //----------------------------B-----------------------------
          //B = Y - U * -1.77200  
          "movi     v3.8b,#197                        \n"   //#104->d3
          "umull    v4.8h,v3.8b,v1.8b                 \n"   //U*#104
          "uqshrn   v4.8b,v4.8h,#8                    \n"   //U*#104>>8
          "ushll    v4.8h,v4.8b,#0                    \n"   //U*0.772->vmovl
          "ushll    v5.8h,v1.8b,#0                    \n"   //U->vmovl
          "uqadd    v4.8h,v4.8h,v5.8h                 \n"   //1.772U


          "umull    v5.8h,v3.8b,v20.8b                \n"   //#104*128
          "uqshrn   v5.8b,v5.8h,#8                    \n"   //#104*128>>8
          "ushll    v5.8h,v5.8b,#0                    \n"   //0.772*128->vmovl
          "ushll    v6.8h,v20.8b,#0                   \n"   //128->vmovl
          "uqadd    v5.8h,v5.8h,v6.8h                 \n"   //1.772*128



          "ushll    v3.8h,v0.8b,#0                    \n"   //Y
          "uqadd    v3.8h,v3.8h,v4.8h                 \n"   //Y+1.772*U
          "uqsub    v3.8h,v3.8h,v5.8h                 \n"   //Y+1.772*U-1.772*128
          #if MT_RED==2
          "uqxtn    v22.8b,v3.8h                      \n"   //RES->v22.8b
          #endif
          #if MT_RED==0
          "uqxtn    v24.8b,v3.8h                      \n"   //RES->v22.8b
          #endif
          //------------------------SUMMARY---------------------------

          "subs       %w2, %w2, #8                    \n"   //
          "movi       v25.8b,#255                     \n"   //
          "st4        {v22.8b,v23.8b,v24.8b,v25.8b}, [%1], #32 \n"  // store 8 ARGB pixels

		    "b.gt       1b                             \n"
		    : "+r"(src_yuy2),  // %0
		      "+r"(dst_argb),  // %1
		      "+r"(width64)    // %2
		    : 
		    : "cc", "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v20",
		      "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29", "v30"
		  );
	}
	#endif  // HAS_YUY2JTOARGBROW_NEON

void YUY2ToYUV444Row_NEON(const uint8* src_yuy2,uint8* dst_yuv444,int pix){
	asm volatile (
		"1:                                          \n"
		//MEMACCESS(0)
		"ld2    {v0.16b,v1.16b}, [%0],32           \n"  // load 16 pixels of YUY2.
		"mov    v3.16b,v1.16b                      \n"
		"trn2   v2.16b,v1.16b,v3.16b              \n" //v
		"trn1   v1.16b,v1.16b,v3.16b              \n" //q1u,q2±ä³Év
		"subs      %2, %2, #16                     \n"  // 16 processed per loop.
		//MEMACCESS(1)
		"st3     {v0.16b,v1.16b,v2.16b},[%1],#48   \n"  // store 16 pixels of Y.
		"b.gt        1b                             \n"
		: "+r"(src_yuy2),  // %0
		"+r"(dst_yuv444),     // %1
		"+r"(pix)        // %2
		:
	: "cc", "memory", "v0", "v1" ,"v2","v4","v5"  // Clobber List
		);
}

void YUV444ToYUY2Row_NEON(const uint8* src_yuv444,uint8* dst_yuy2,int pix){
	asm volatile (
		"1:                                          \n"
		//MEMACCESS(0)
		"ld3    {v0.16b,v1.16b,v2.16b}, [%0],#48              \n"  // d0->Y  d1->U  d2->V
		// "vld3.u8    {d1,d3,d5}, [%0]!            \n"  // d1->Y  d3->U  d5->V
		"mov v4.d[0],v0.d[1]\n"

		"saddlp     v1.8h,v1.16b                    \n"  // U 16 bytes -> 8 shorts.
		"saddlp     v3.8h,v2.16b                    \n"  // U 16 bytes -> 8 shorts.
		"srshr      v1.8h, v1.8h, #1                \n"
		"srshr      v3.8h, v3.8h, #1                \n"
		"sqxtn      v1.8b, v1.8h                    \n"
		"sqxtn      v3.8b, v3.8h                    \n"

		"subs      %2, %2, #16                      \n"  // 16 processed per loop.
		//MEMACCESS(1)
		"uzp2 v2.8b,v0.8b,v4.8b\n"
		"uzp1 v0.8b,v0.8b,v4.8b\n"   //v0,v3->y1,y2

		"st4     {v0.8b,v1.8b,v2.8b,v3.8b}, [%1],#32          \n"  // store 16 pixels of Y.
		"b.gt        1b                             \n"
		: "+r"(src_yuv444),  // %0
		"+r"(dst_yuy2),     // %1
		"+r"(pix)        // %2
		:
	: "cc", "memory", "v0", "v1" ,"v2","v3","v4" // Clobber List
		);
}
#endif