#include "convert_yuv.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#ifdef __cplusplus
extern "C" {
#endif

// C reference code that mimics the YUV assembly.

#define YG 74 /* (int8)(1.164 * 64 + 0.5) */

#define UB 127 /* min(63,(int8)(2.018 * 64)) */
#define UG -25 /* (int8)(-0.391 * 64 - 0.5) */
#define UR 0

#define VB 0
#define VG -52 /* (int8)(-0.813 * 64 - 0.5) */
#define VR 102 /* (int8)(1.596 * 64 + 0.5) */

	// Bias
#define BB UB * 128 + VB * 128
#define BG UG * 128 + VG * 128
#define BR UR * 128 + VR * 128

#ifndef IS_ALIGNED
#define IS_ALIGNED(p, a) (!((uintptr_t)(p) & ((a) - 1)))
#endif

#ifndef align_buffer_64
#define align_buffer_64(var, size)                                             \
	uint8* var;                                                                \
	uint8* var##_mem;                                                          \
	var##_mem = reinterpret_cast<uint8*>(malloc((size) + 63));                 \
	var = reinterpret_cast<uint8*>                                             \
	((reinterpret_cast<intptr_t>(var##_mem) + 63) & ~63)

#define free_aligned_buffer_64(var) \
	free(var##_mem);  \
	var = 0
#endif

	// Visual C x86 or GCC little endian.
#if defined(__x86_64__) || defined(_M_X64) || \
	defined(__i386__) || defined(_M_IX86) || \
	defined(__arm__) || defined(_M_ARM) || \
	(defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define MTXX_LITTLE_ENDIAN
#endif

#ifdef MTXX_LITTLE_ENDIAN
#define WRITEWORD(p, v) *reinterpret_cast<uint32*>(p) = v
#else
	static inline void WRITEWORD(uint8* p, uint32 v) {
		p[0] = (uint8)(v & 255);
		p[1] = (uint8)((v >> 8) & 255);
		p[2] = (uint8)((v >> 16) & 255);
		p[3] = (uint8)((v >> 24) & 255);
	}
#endif

static __inline int RGBToY(uint8 r, uint8 g, uint8 b) {
	return (66 * r + 129 * g +  25 * b + 0x1080) >> 8;
}

static __inline int RGBToU(uint8 r, uint8 g, uint8 b) {
	return (112 * b - 74 * g - 38 * r + 0x8080) >> 8;
}
static __inline int RGBToV(uint8 r, uint8 g, uint8 b) {
	return (112 * r - 94 * g - 18 * b + 0x8080) >> 8;
}


static __inline uint8 Clip(int val) {
	if (val <= 0) {
		return  (uint8)(0);
	} else if (val >= 255) {
		return (uint8)(255);
	}
	return (uint8)(val);
}

static __inline void YuvPixel(uint8 y, uint8 u, uint8 v, uint8* rgb_buf) {


	int y1 = ((int)(y) - 16) * YG;
	rgb_buf[MT_BLUE] = Clip((int)((u * UB + v * VB) - (BB) + y1) >> 6);
	rgb_buf[MT_GREEN] = Clip((int)((u * UG + v * VG) - (BG) + y1) >> 6);
	rgb_buf[MT_RED] = Clip((int)((u * UR + v * VR) - (BR) + y1) >> 6);

	rgb_buf[MT_ALPHA] = 255;
}

#define MAKEROWY(NAME, R, G, B, BPP)											\
static void NAME ## ToYRow_C(const uint8* src_argb0, uint8* dst_y, int width) {     \
	for (int x = 0; x < width; ++x) {                                            \
	dst_y[0] = RGBToY(src_argb0[R], src_argb0[G], src_argb0[B]);               \
	src_argb0 += BPP;                                                          \
	dst_y += 1;                                                                \
	}                                                                            \
}                                                                              \
static void NAME ## ToUVRow_C(const uint8* src_rgb0, int src_stride_rgb,              \
	uint8* dst_u, uint8* dst_v, int width) {									\
	const uint8* src_rgb1 = src_rgb0 + src_stride_rgb;                           \
	for (int x = 0; x < width - 1; x += 2) {                                     \
	uint8 ab = (src_rgb0[B] + src_rgb0[B + BPP] +                              \
	src_rgb1[B] + src_rgb1[B + BPP]) >> 2;                          \
	uint8 ag = (src_rgb0[G] + src_rgb0[G + BPP] +                              \
	src_rgb1[G] + src_rgb1[G + BPP]) >> 2;                          \
	uint8 ar = (src_rgb0[R] + src_rgb0[R + BPP] +                              \
	src_rgb1[R] + src_rgb1[R + BPP]) >> 2;                          \
	dst_u[0] = RGBToU(ar, ag, ab);                                             \
	dst_v[0] = RGBToV(ar, ag, ab);                                             \
	src_rgb0 += BPP * 2;                                                       \
	src_rgb1 += BPP * 2;                                                       \
	dst_u += 1;                                                                \
	dst_v += 1;                                                                \
	}                                                                            \
	if (width & 1) {                                                             \
	uint8 ab = (src_rgb0[B] + src_rgb1[B]) >> 1;                               \
	uint8 ag = (src_rgb0[G] + src_rgb1[G]) >> 1;                               \
	uint8 ar = (src_rgb0[R] + src_rgb1[R]) >> 1;                               \
	dst_u[0] = RGBToU(ar, ag, ab);                                             \
	dst_v[0] = RGBToV(ar, ag, ab);                                             \
	}                                                                            \
}

MAKEROWY(ARGB, MT_RED, MT_GREEN, MT_BLUE, 4)
#undef MAKEROWY


static void MergeUVRow_C(const uint8* src_u, const uint8* src_v, uint8* dst_uv,int width) 
{
	for (int x = 0; x < width - 1; x += 2) {
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
static void NV21ToARGBRow_C(const uint8* src_y,
	const uint8* src_vu,
	uint8* rgb_buf,
	int width) {
		for (int x = 0; x < width - 1; x += 2) {
			YuvPixel(src_y[0], src_vu[1], src_vu[0],rgb_buf);
			YuvPixel(src_y[1], src_vu[1], src_vu[0],rgb_buf + 4);
			src_y += 2;
			src_vu += 2;
			rgb_buf += 8;  // Advance 2 pixels.
		}
		if (width & 1) {
			YuvPixel(src_y[0], src_vu[1], src_vu[0],rgb_buf);
		}
}

static void ARGBToUV422Row_C(const uint8* src_argb,
	uint8* dst_u, uint8* dst_v, int width) {
		for (int x = 0; x < width - 1; x += 2) {
			uint8 ab = (src_argb[MT_BLUE] + src_argb[4 + MT_BLUE]) >> 1;
			uint8 ag = (src_argb[MT_GREEN] + src_argb[4 + MT_GREEN]) >> 1;
			uint8 ar = (src_argb[MT_RED] + src_argb[4 + MT_RED]) >> 1;
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


static void I422ToYUY2Row_C(const uint8* src_y,
	const uint8* src_u,
	const uint8* src_v,
	uint8* dst_frame, int width) {
		for (int x = 0; x < width - 1; x += 2) {
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

static void YUY2ToARGBRow_C(const uint8* src_yuy2,
	uint8* rgb_buf,
	int width) {
		for (int x = 0; x < width - 1; x += 2) {
			YuvPixel(src_yuy2[0], src_yuy2[1], src_yuy2[3],rgb_buf );
			YuvPixel(src_yuy2[2], src_yuy2[1], src_yuy2[3],rgb_buf + 4);

			src_yuy2 += 4;
			rgb_buf += 8;  // Advance 2 pixels.
		}
		if (width & 1) {
			YuvPixel(src_yuy2[0], src_yuy2[1], src_yuy2[3],rgb_buf );
		}
}

// Also used for 420
static void I422ToARGBRow_C(const uint8* src_y,
	const uint8* src_u,
	const uint8* src_v,
	uint8* rgb_buf,
	int width) {
		for (int x = 0; x < width - 1; x += 2) {
			YuvPixel(src_y[0], src_u[0], src_v[0],rgb_buf);

			YuvPixel(src_y[1], src_u[0], src_v[0],rgb_buf + 4);

			src_y += 2;
			src_u += 1;
			src_v += 1;
			rgb_buf += 8;  // Advance 2 pixels.
		}
		if (width & 1) {
			YuvPixel(src_y[0], src_u[0], src_v[0],rgb_buf);

		}
}

// Copy row of YUY2 Y's (422) into Y (420/422).
static void YUY2ToYRow_C(const uint8* src_yuy2, uint8* dst_y, int width) {
	// Output a row of Y values.
	for (int x = 0; x < width - 1; x += 2) {
		dst_y[x] = src_yuy2[0];
		dst_y[x + 1] = src_yuy2[2];
		src_yuy2 += 4;
	}
	if (width & 1) {
		dst_y[width - 1] = src_yuy2[0];
	}
}


// Copy row of YUY2 UV's (422) into U and V (422).
static void YUY2ToUV422Row_C(const uint8* src_yuy2,
	uint8* dst_u, uint8* dst_v, int width) {
		// Output a row of UV values.
		for (int x = 0; x < width; x += 2) {
			dst_u[0] = src_yuy2[1];
			dst_v[0] = src_yuy2[3];
			src_yuy2 += 4;
			dst_u += 1;
			dst_v += 1;
		}
}

static void I444ToYUY2Row_C(const uint8* src_y,
	const uint8* src_u,
	const uint8* src_v,
	uint8* dst_frame, int width)
{
	int x;
	int tW = width - 1;
	for (x = 0; x < tW; x += 2) {
		dst_frame[0] = src_y[0];
		dst_frame[1] = (src_u[0] + src_u[1])>>1;
		dst_frame[2] = src_y[1];
		dst_frame[3] = (src_v[0] + src_v[1])>>1;
		dst_frame += 4;
		src_y += 2;
		src_u += 2;
		src_v += 2;
	}
	if (width & 1) {
		dst_frame[0] = src_y[0];
		dst_frame[1] = src_u[0];
		dst_frame[2] = src_y[0];  // duplicate last y
		dst_frame[3] = src_v[0];
	}
}
static void YUY2ToUV444Row_C(const uint8* src_yuy2,
	uint8* dst_u, uint8* dst_v, int width)
{
	int x;
	for (x = 0; x < width; x += 2) {
		dst_u[0] = dst_u[1] = src_yuy2[1];
		dst_v[0] = dst_v[1] = src_yuy2[3];
		src_yuy2 += 4;
		dst_u += 2;
		dst_v += 2;
	}
}

static void ARGBToRGB565Row_C(const uint8* src_argb, uint8* dst_rgb, int width) {
	for (int x = 0; x < width - 1; x += 2) {
		uint8 b0 = src_argb[0] >> 3;
		uint8 g0 = src_argb[1] >> 2;
		uint8 r0 = src_argb[2] >> 3;
		uint8 b1 = src_argb[4] >> 3;
		uint8 g1 = src_argb[5] >> 2;
		uint8 r1 = src_argb[6] >> 3;
		WRITEWORD(dst_rgb, b0 | (g0 << 5) | (r0 << 11) |
			(b1 << 16) | (g1 << 21) | (r1 << 27));
		dst_rgb += 4;
		src_argb += 8;
	}
	if (width & 1) {
		uint8 b0 = src_argb[0] >> 3;
		uint8 g0 = src_argb[1] >> 2;
		uint8 r0 = src_argb[2] >> 3;
		*reinterpret_cast<uint16*>(dst_rgb) = b0 | (g0 << 5) | (r0 << 11);
	}
}

static void ARGBToUV444Row_C(const uint8* src_argb,
	uint8* dst_u, uint8* dst_v, int width) {
		for (int x = 0; x < width; ++x) {
			uint8 ab = src_argb[0];
			uint8 ag = src_argb[1];
			uint8 ar = src_argb[2];
			dst_u[0] = RGBToU(ar, ag, ab);
			dst_v[0] = RGBToV(ar, ag, ab);
			src_argb += 4;
			dst_u += 1;
			dst_v += 1;
		}
}
static void I444ToARGBRow_C(const uint8* src_y,
	const uint8* src_u,
	const uint8* src_v,
	uint8* rgb_buf,
	int width) {
		for (int x = 0; x < width; ++x) {
			YuvPixel(src_y[0], src_u[0], src_v[0],rgb_buf );
			src_y += 1;
			src_u += 1;
			src_v += 1;
			rgb_buf += 4;  // Advance 1 pixel.
		}
}
static void RGB565ToARGBRow_C(const uint8* src_rgb565, uint8* dst_argb, int width) {
	for (int x = 0; x < width; ++x) {
		uint8 b = src_rgb565[0] & 0x1f;
		uint8 g = (src_rgb565[0] >> 5) | ((src_rgb565[1] & 0x07) << 3);
		uint8 r = src_rgb565[1] >> 3;
		dst_argb[0] = (b << 3) | (b >> 2);
		dst_argb[1] = (g << 2) | (g >> 4);
		dst_argb[2] = (r << 3) | (r >> 2);
		dst_argb[3] = 255u;
		dst_argb += 4;
		src_rgb565 += 2;
	}
}

static void I422ToUYVYRow_C(const uint8* src_y,
	const uint8* src_u,
	const uint8* src_v,
	uint8* dst_frame, int width) {
		for (int x = 0; x < width - 1; x += 2) {
			dst_frame[0] = src_u[0];
			dst_frame[1] = src_y[0];
			dst_frame[2] = src_v[0];
			dst_frame[3] = src_y[1];
			dst_frame += 4;
			src_y += 2;
			src_u += 1;
			src_v += 1;
		}
		if (width & 1) {
			dst_frame[0] = src_u[0];
			dst_frame[1] = src_y[0];
			dst_frame[2] = src_v[0];
			dst_frame[3] = src_y[0];  // duplicate last y
		}
}

static void UYVYToARGBRow_C(const uint8* src_uyvy,
	uint8* rgb_buf,
	int width) {
		for (int x = 0; x < width - 1; x += 2) {
			YuvPixel(src_uyvy[1], src_uyvy[0], src_uyvy[2],rgb_buf);
			YuvPixel(src_uyvy[3], src_uyvy[0], src_uyvy[2],rgb_buf + 4);
			src_uyvy += 4;
			rgb_buf += 8;  // Advance 2 pixels.
		}
		if (width & 1) {
			YuvPixel(src_uyvy[1], src_uyvy[0], src_uyvy[2],rgb_buf);
		}
}
//////////////////////////////////////////////////////////////////////////
///////////////////////////////FOR NEON///////////////////////////////////
#ifdef HAVE_NEON
#include <arm_neon.h>

// Read 8 Y and 4 VU from NV21
#define READNV21                                                               \
	"vld1.u8    {d0}, [%0]!                    \n"                             \
	"vld1.u8    {d2}, [%1]!                    \n"                             \
	"vmov.u8    d3, d2                         \n"/* split odd/even uv apart */\
	"vuzp.u8    d3, d2                         \n"                             \
	"vtrn.u32   d2, d3                         \n"
// Read 8 YUY2
#define READYUY2                                                               \
	"vld2.8     {d0, d2}, [%0]!                \n"                             \
	"vmov.u8    d3, d2                         \n"                             \
	"vuzp.u8    d2, d3                         \n"                             \
	"vtrn.u32   d2, d3                         \n"
// Read 8 Y, 4 U and 4 V from 422
#define READYUV422                                                             \
	"vld1.u8    {d0}, [%0]!                    \n"                             \
	"vld1.u32   {d2[0]}, [%1]!                 \n"                             \
	"vld1.u32   {d2[1]}, [%2]!                 \n"

#define YUV422TORGB                                                            \
	"veor.u8    d2, d26                        \n"/*subtract 128 from u and v*/\
	"vmull.s8   q8, d2, d24                    \n"/*  u/v B/R component      */\
	"vmull.s8   q9, d2, d25                    \n"/*  u/v G component        */\
	"vmov.u8    d1, #0                         \n"/*  split odd/even y apart */\
	"vtrn.u8    d0, d1                         \n"                             \
	"vsub.s16   q0, q0, q15                    \n"/*  offset y               */\
	"vmul.s16   q0, q0, q14                    \n"                             \
	"vadd.s16   d18, d19                       \n"                             \
	"vqadd.s16  d22, d0, d16                   \n" /* B */                     \
	"vqadd.s16  d21, d1, d16                   \n"                             \
	"vqadd.s16  d20, d0, d17                   \n" /* R */                     \
	"vqadd.s16  d23, d1, d17                   \n"                             \
	"vqadd.s16  d16, d0, d18                   \n" /* G */                     \
	"vqadd.s16  d17, d1, d18                   \n"                             \
	"vqshrun.s16 d0, q10, #6                   \n" /* B */                     \
	"vqshrun.s16 d1, q11, #6                   \n" /* G */                     \
	"vqshrun.s16 d2, q8, #6                    \n" /* R */                     \
	"vmovl.u8   q10, d0                        \n"/*  set up for reinterleave*/\
	"vmovl.u8   q11, d1                        \n"                             \
	"vmovl.u8   q8, d2                         \n"                             \
	"vtrn.u8    d22, d21                       \n"                             \
	"vtrn.u8    d20, d23                       \n"                             \
	"vtrn.u8    d16, d17                       \n"                             \
	"vmov.u8    d21, d16                       \n"

// 16x2 pixels -> 8x1.  pix is number of argb pixels. e.g. 16.
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

#define ARGBTORGB565                                                           \
	"vshr.u8    d20, d20, #3                   \n"  /* B                    */ \
	"vshr.u8    d21, d21, #2                   \n"  /* G                    */ \
	"vshr.u8    d22, d22, #3                   \n"  /* R                    */ \
	"vmovl.u8   q8, d22                        \n"  /* R                    */ \
	"vmovl.u8   q9, d21                        \n"  /* G                    */ \
	"vmovl.u8   q10, d20                       \n"  /* B                    */ \
	"vshl.u16   q9, q9, #5                     \n"  /* G                    */ \
	"vshl.u16   q10, q10, #11                  \n"  /* R                    */ \
	"vorr       q0, q8, q9                     \n"  /* BG                   */ \
	"vorr       q0, q0, q10                    \n"  /* BGR                  */

#define RGB565TOARGB                                                           \
	"vshrn.u16  d6, q0, #5                     \n"  /* G xxGGGGGG           */ \
	"vuzp.u8    d0, d1                         \n"  /* d0 xxxBBBBB RRRRRxxx */ \
	"vshl.u8    d6, d6, #2                     \n"  /* G GGGGGG00 upper 6   */ \
	"vshr.u8    d1, d1, #3                     \n"  /* R 000RRRRR lower 5   */ \
	"vshl.u8    q0, q0, #3                     \n"  /* B,R BBBBB000 upper 5 */ \
	"vshr.u8    q2, q0, #5                     \n"  /* B,R 00000BBB lower 3 */ \
	"vorr.u8    d0, d0, d4                     \n"  /* B                    */ \
	"vshr.u8    d4, d6, #6                     \n"  /* G 000000GG lower 2   */ \
	"vorr.u8    d2, d1, d5                     \n"  /* R                    */ \
	"vorr.u8    d1, d4, d6                     \n"  /* G                    */


// RGB/YUV to Y does multiple of 16 with SIMD and last 16 with SIMD.
#define YANY(NAMEANY, ARGBTOY_SIMD, SBPP, BPP, NUM)                            \
	static void NAMEANY(const uint8* src_argb, uint8* dst_y, int width) {             \
	ARGBTOY_SIMD(src_argb, dst_y, width - NUM);                              \
	ARGBTOY_SIMD(src_argb + (width - NUM) * SBPP,                            \
	dst_y + (width - NUM) * BPP, NUM);                          \
}
//Edit by YZH
static void ARGBToYRow_NEON(const uint8* src_argb, uint8* dst_y, int pix) {
	asm volatile (
		"vmov.u8    d24, #33                       \n"  // R * 0.1016 coefficient
		"vmov.u8    d25, #65                       \n"  // G * 0.5078 coefficient
		"vmov.u8    d26, #13                       \n"  // B * 0.2578 coefficient
		"vmov.u8    d27, #16                       \n"  // Add 16 constant
		".p2align   2                              \n"
		"1:                                          \n"
		"vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  // load 8 ARGB pixels.
		"subs       %2, %2, #8                     \n"  // 8 processed per loop.
		"vmull.u8   q2, d0, d24                    \n"  // R
		"vmlal.u8   q2, d1, d25                    \n"  // G
		"vmlal.u8   q2, d2, d26                    \n"  // B
		"vqrshrun.s16 d0, q2, #7                   \n"  // 16 bit to 8 bit Y
		"vqadd.u8   d0, d27                        \n"
		"vst1.8     {d0}, [%1]!                    \n"  // store 8 pixels Y.
		"bgt        1b                             \n"
		: "+r"(src_argb),  // %0
		"+r"(dst_y),     // %1
		"+r"(pix)        // %2
		:
	: "cc", "memory", "q0", "q1", "q2", "q12", "q13"
		);
}

static void YUY2ToYRow_NEON(const uint8* src_yuy2, uint8* dst_y, int pix) {
	asm volatile (
		".p2align   2                              \n"
		"1:                                          \n"
		"vld2.8     {q0, q1}, [%0]!                \n"  // load 16 pixels of YUY2.
		"subs       %2, %2, #16                    \n"  // 16 processed per loop.
		"vst1.8     {q0}, [%1]!                    \n"  // store 16 pixels of Y.
		"bgt        1b                             \n"
		: "+r"(src_yuy2),  // %0
		"+r"(dst_y),     // %1
		"+r"(pix)        // %2
		:
	: "cc", "memory", "q0", "q1"  // Clobber List
		);
}

static void RGB565ToARGBRow_NEON(const uint8* src_rgb565, uint8* dst_argb, int pix) {
	asm volatile (
		"vmov.u8    d3, #255                       \n"  // Alpha
		".p2align   2                              \n"
		"1:                                          \n"
		"vld1.8     {q0}, [%0]!                    \n"  // load 8 RGB565 pixels.
		"subs       %2, %2, #8                     \n"  // 8 processed per loop.
		RGB565TOARGB
		"vst4.8     {d0, d1, d2, d3}, [%1]!        \n"  // store 8 pixels of ARGB.
		"bgt        1b                             \n"
		: "+r"(src_rgb565),  // %0
		"+r"(dst_argb),    // %1
		"+r"(pix)          // %2
		:
	: "cc", "memory", "q0", "q1", "q2", "q3"  // Clobber List
		);
}
YANY(ARGBToYRow_Any_NEON, ARGBToYRow_NEON, 4, 1, 8)
YANY(YUY2ToYRow_Any_NEON, YUY2ToYRow_NEON, 2, 1, 16)
YANY(RGB565ToARGBRow_Any_NEON, RGB565ToARGBRow_NEON, 2, 4, 8)
#undef YANY

	// RGB/YUV to UV does multiple of 16 with SIMD and remainder with C.
#define UVANY(NAMEANY, ANYTOUV_SIMD, ANYTOUV_C, BPP, MASK)                     \
	static void NAMEANY(const uint8* src_argb, int src_stride_argb,                   \
	uint8* dst_u, uint8* dst_v, int width) {                      \
	int n = width & ~MASK;                                                   \
	ANYTOUV_SIMD(src_argb, src_stride_argb, dst_u, dst_v, n);                \
	ANYTOUV_C(src_argb  + n * BPP, src_stride_argb,                          \
	dst_u + (n >> 1),                                              \
	dst_v + (n >> 1),                                              \
	width & MASK);                                                 \
}
// TODO(fbarchard): Consider vhadd vertical, then vpaddl horizontal, avoid shr.
static void ARGBToUVRow_NEON(const uint8* src_argb, int src_stride_argb,
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
		RGBTOUV(q2, q1, q0)								//swap by yzh
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

UVANY(ARGBToUVRow_Any_NEON, ARGBToUVRow_NEON, ARGBToUVRow_C, 4, 15)

#undef UVANY

#define MERGEUVROW_ANY(NAMEANY, ANYTOUV_SIMD, ANYTOUV_C, MASK)                 \
	static void NAMEANY(const uint8* src_u, const uint8* src_v,                       \
	uint8* dst_uv, int width) {                                   \
	int n = width & ~MASK;                                                   \
	ANYTOUV_SIMD(src_u, src_v, dst_uv, n);                                   \
	ANYTOUV_C(src_u + n,                                                     \
	src_v + n,                                                     \
	dst_uv + n * 2,                                                \
	width & MASK);                                                 \
}
	// Reads 16 U's and V's and writes out 16 pairs of UV.
static void MergeUVRow_NEON(const uint8* src_u, const uint8* src_v, uint8* dst_uv,
	int width) {
	asm volatile (
		".p2align  2                               \n"
		"1:                                        \n"
		"vld1.u8    {q0}, [%0]!                    \n"  // load U
		"vld1.u8    {q1}, [%1]!                    \n"  // load V
		"subs       %3, %3, #16                    \n"  // 16 processed per loop
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
MERGEUVROW_ANY(MergeUVRow_Any_NEON, MergeUVRow_NEON, MergeUVRow_C, 15)

#undef MERGEUVROW_ANY

// Wrappers to handle odd width
#define NV2NY(NAMEANY, NV12TORGB_SIMD, NV12TORGB_C, UV_SHIFT, BPP)             \
	static void NAMEANY(const byte* y_buf,                                           \
	const byte* uv_buf,                                          \
	byte* rgb_buf,                                               \
	int width) {                                                  \
	int n = width & ~7;                                                      \
	NV12TORGB_SIMD(y_buf, uv_buf, rgb_buf, n);                               \
	NV12TORGB_C(y_buf + n,                                                   \
	uv_buf + (n >> UV_SHIFT),                                    \
	rgb_buf + n * BPP, width & 7);                               \
}

typedef char vec8[16];
static vec8 kUVToRB  = { 127, 127, 127, 127, 102, 102, 102, 102,
	0, 0, 0, 0, 0, 0, 0, 0 };
static vec8 kUVToG = { -25, -25, -25, -25, -52, -52, -52, -52,
	0, 0, 0, 0, 0, 0, 0, 0 };

static void NV21ToARGBRow_NEON(const uint8* src_y,
	const uint8* src_uv,
	uint8* dst_argb,
	int width) {
		asm volatile (
			"vld1.8     {d24}, [%4]                    \n"
			"vld1.8     {d25}, [%5]                    \n"
			"vmov.u8    d26, #128                      \n"
			"vmov.u16   q14, #74                       \n"
			"vmov.u16   q15, #16                       \n"
			".p2align   2                              \n"
			"1:                                          \n"
			READNV21
			YUV422TORGB
			"subs       %3, %3, #8                     \n"
			"vmov.u8    d23, #255                      \n"
			"vst4.8     {d20, d21, d22, d23}, [%2]!    \n"
			"bgt        1b                             \n"
			: "+r"(src_y),     // %0
			"+r"(src_uv),    // %1
			"+r"(dst_argb),  // %2
			"+r"(width)      // %3
			: "r"(&kUVToRB),   // %4
			"r"(&kUVToG)     // %5
			: "cc", "memory", "q0", "q1", "q2", "q3",
			"q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
			);
}

NV2NY(NV21ToARGBRow_Any_NEON, NV21ToARGBRow_NEON, NV21ToARGBRow_C, 0, 4)

#undef NV2NY

	// 16x1 pixels -> 8x1.  pix is number of argb pixels. e.g. 16.
static void ARGBToUV422Row_NEON(const uint8* src_argb, uint8* dst_u, uint8* dst_v,
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
			"vld4.8     {d0, d2, d4, d6}, [%0]!        \n"  // load 8 ARGB pixels.
			"vld4.8     {d1, d3, d5, d7}, [%0]!        \n"  // load next 8 ARGB pixels.

			"vpaddl.u8  q0, q0                         \n"  // B 16 bytes -> 8 shorts.
			"vpaddl.u8  q1, q1                         \n"  // G 16 bytes -> 8 shorts.
			"vpaddl.u8  q2, q2                         \n"  // R 16 bytes -> 8 shorts.

			"subs       %3, %3, #16                    \n"  // 16 processed per loop.
			"vmul.s16   q8, q2, q10                    \n"  // B
			"vmls.s16   q8, q1, q11                    \n"  // G
			"vmls.s16   q8, q0, q12                    \n"  // R
			"vadd.u16   q8, q8, q15                    \n"  // +128 -> unsigned

			"vmul.s16   q9, q0, q10                    \n"  // R
			"vmls.s16   q9, q1, q14                    \n"  // G
			"vmls.s16   q9, q2, q13                    \n"  // B
			"vadd.u16   q9, q9, q15                    \n"  // +128 -> unsigned

			"vqshrn.u16  d0, q8, #8                    \n"  // 16 bit to 8 bit U
			"vqshrn.u16  d1, q9, #8                    \n"  // 16 bit to 8 bit V
			"vst1.8     {d0}, [%1]!                    \n"  // store 8 pixels U.
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
#define UV422ANY(NAMEANY, ANYTOUV_SIMD, ANYTOUV_C, BPP, MASK, SHIFT)           \
	static void NAMEANY(const byte* src_uv,                                          \
	byte* dst_u, byte* dst_v, int width) {                      \
	int n = width & ~MASK;                                                   \
	ANYTOUV_SIMD(src_uv, dst_u, dst_v, n);                                   \
	ANYTOUV_C(src_uv  + n * BPP,                                             \
	dst_u + (n >> SHIFT),                                          \
	dst_v + (n >> SHIFT),                                          \
	width & MASK);                                                 \
}

static void YUY2ToUV422Row_NEON(const uint8* src_yuy2, uint8* dst_u, uint8* dst_v,
	int pix) {
		asm volatile (
			".p2align  2                               \n"
			"1:                                          \n"
			"vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  // load 16 pixels of YUY2.
			"subs       %3, %3, #16                    \n"  // 16 pixels = 8 UVs.
			"vst1.u8    {d1}, [%1]!                    \n"  // store 8 U.
			"vst1.u8    {d3}, [%2]!                    \n"  // store 8 V.
			"bgt        1b                             \n"
			: "+r"(src_yuy2),  // %0
			"+r"(dst_u),     // %1
			"+r"(dst_v),     // %2
			"+r"(pix)        // %3
			:
		: "cc", "memory", "d0", "d1", "d2", "d3"  // Clobber List
			);
}
UV422ANY(YUY2ToUV422Row_Any_NEON, YUY2ToUV422Row_NEON,
	YUY2ToUV422Row_C, 2, 15, 1)
UV422ANY(ARGBToUV422Row_Any_NEON, ARGBToUV422Row_NEON,
	ARGBToUV422Row_C, 4, 15, 1)
#undef UV422ANY

// TODO(fbarchard): Consider 'any' functions handling any quantity of pixels.
// TODO(fbarchard): Consider 'any' functions handling odd alignment.
// YUV to RGB does multiple of 8 with SIMD and remainder with C.
#define YANY(NAMEANY, I420TORGB_SIMD, I420TORGB_C, UV_SHIFT, BPP, MASK)        \
	static void NAMEANY(const uint8* y_buf,                                           \
	const uint8* u_buf,                                           \
	const uint8* v_buf,                                           \
	uint8* rgb_buf,                                               \
	int width) {                                                  \
	int n = width & ~MASK;                                                   \
	I420TORGB_SIMD(y_buf, u_buf, v_buf, rgb_buf, n);                         \
	I420TORGB_C(y_buf + n,                                                   \
	u_buf + (n >> UV_SHIFT),                                     \
	v_buf + (n >> UV_SHIFT),                                     \
	rgb_buf + n * BPP, width & MASK);                            \
}

static void I422ToYUY2Row_NEON(const uint8* src_y,
	const uint8* src_u,
	const uint8* src_v,
	uint8* dst_yuy2, int width) {
	asm volatile (
		".p2align   2                              \n"
		"1:                                          \n"
		"vld2.8     {d0, d2}, [%0]!                \n"  // load 16 Ys
		"vld1.8     {d1}, [%1]!                    \n"  // load 8 Us
		"vld1.8     {d3}, [%2]!                    \n"  // load 8 Vs
		"subs       %4, %4, #16                    \n"  // 16 pixels
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
static  void I422ToARGBRow_NEON(const uint8* src_y,
	const uint8* src_u,
	const uint8* src_v,
	uint8* dst_argb,
	int width) {
		asm volatile (
			"vld1.8     {d24}, [%5]                    \n"
			"vld1.8     {d25}, [%6]                    \n"
			"vmov.u8    d26, #128                      \n"
			"vmov.u16   q14, #74                       \n"
			"vmov.u16   q15, #16                       \n"
			".p2align   2                              \n"
			"1:                                          \n"
			READYUV422
			YUV422TORGB
			"subs       %4, %4, #8                     \n"
			"vmov.u8    d23, #255                      \n"
			"vst4.8     {d20, d21, d22, d23}, [%3]!    \n"
			"bgt        1b                             \n"
			: "+r"(src_y),     // %0
			"+r"(src_u),     // %1
			"+r"(src_v),     // %2
			"+r"(dst_argb),  // %3
			"+r"(width)      // %4
			: "r"(&kUVToRB),   // %5
			"r"(&kUVToG)     // %6
			: "cc", "memory", "q0", "q1", "q2", "q3",
			"q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
			);
}

YANY(I422ToARGBRow_Any_NEON, I422ToARGBRow_NEON, I422ToARGBRow_C, 1, 4, 7)
YANY(I422ToYUY2Row_Any_NEON, I422ToYUY2Row_NEON, I422ToYUY2Row_C, 1, 2, 15)
#undef YANY


#define RGBANY(NAMEANY, ARGBTORGB_SIMD, ARGBTORGB_C, MASK, SBPP, BPP)          \
	static void NAMEANY(const uint8* src,                                             \
	uint8* dst,                                                   \
	int width) {                                                  \
	int n = width & ~MASK;                                                   \
	ARGBTORGB_SIMD(src, dst, n);                                             \
	ARGBTORGB_C(src + n * SBPP, dst + n * BPP, width & MASK);                \
}

static void YUY2ToARGBRow_NEON(const uint8* src_yuy2,
uint8* dst_argb,
int width) {
	asm volatile (
		"vld1.8     {d24}, [%3]                    \n"
		"vld1.8     {d25}, [%4]                    \n"
		"vmov.u8    d26, #128                      \n"
		"vmov.u16   q14, #74                       \n"
		"vmov.u16   q15, #16                       \n"
		".p2align   2                              \n"
		"1:                                          \n"
		READYUY2
		YUV422TORGB
		"subs       %2, %2, #8                     \n"
		"vmov.u8    d23, #255                      \n"
		"vst4.8     {d20, d21, d22, d23}, [%1]!    \n"
		"bgt        1b                             \n"
		: "+r"(src_yuy2),  // %0
		"+r"(dst_argb),  // %1
		"+r"(width)      // %2
		: "r"(&kUVToRB),   // %3
		"r"(&kUVToG)     // %4
		: "cc", "memory", "q0", "q1", "q2", "q3",
		"q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
		);
}
static void ARGBToRGB565Row_NEON(const uint8* src_argb, uint8* dst_rgb565, int pix) {
	asm volatile (
		".p2align   2                              \n"
		"1:                                          \n"
		"vld4.8     {d20, d21, d22, d23}, [%0]!    \n"  // load 8 pixels of ARGB.
		"subs       %2, %2, #8                     \n"  // 8 processed per loop.
		ARGBTORGB565
		"vst1.8     {q0}, [%1]!                    \n"  // store 8 pixels RGB565.
		"bgt        1b                             \n"
		: "+r"(src_argb),  // %0
		"+r"(dst_rgb565),  // %1
		"+r"(pix)        // %2
		:
	: "cc", "memory", "q0", "q8", "q9", "q10", "q11"
		);
}

RGBANY(YUY2ToARGBRow_Any_NEON, YUY2ToARGBRow_NEON, YUY2ToARGBRow_C,
	7, 2, 4)
RGBANY(ARGBToRGB565Row_Any_NEON, ARGBToRGB565Row_NEON, ARGBToRGB565Row_C,
	7, 4, 2)

#undef RGBANY



#endif
//////////////////////////////////////////////////////////////////////////
// Same as NV12 but U and V swapped.
int ARGBToNV21(const uint8* src_argb, int src_stride_argb,
	uint8* dst_y, int dst_stride_y,
	uint8* dst_uv, int dst_stride_uv,
	int width, int height) {
		if (!src_argb ||
			!dst_y || !dst_uv ||
			width <= 0 || height == 0) {
				return -1;
		}
		// Negative height means invert the image.
		if (height < 0) {
			height = -height;
			src_argb = src_argb + (height - 1) * src_stride_argb;
			src_stride_argb = -src_stride_argb;
		}
		void (*ARGBToUVRow)(const uint8* src_argb0, int src_stride_argb,
			uint8* dst_u, uint8* dst_v, int width) = ARGBToUVRow_C;
		void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix) =
			ARGBToYRow_C;
#if defined(HAVE_NEON)
		if (width >= 8) {
			ARGBToYRow = ARGBToYRow_Any_NEON;
			if (IS_ALIGNED(width, 8)) {
				ARGBToYRow = ARGBToYRow_NEON;
			}
			if (width >= 16) {
				ARGBToUVRow = ARGBToUVRow_Any_NEON;
				if (IS_ALIGNED(width, 16)) {
					ARGBToUVRow = ARGBToUVRow_NEON;
				}
			}
		}
#endif
		int halfwidth = (width + 1) >> 1;
		void (*MergeUVRow_)(const uint8* src_u, const uint8* src_v, uint8* dst_uv,
			int width) = MergeUVRow_C;
#if defined(HAVE_NEON)
		if (halfwidth >= 16) {
			MergeUVRow_ = MergeUVRow_Any_NEON;
			if (IS_ALIGNED(halfwidth, 16)) {
				MergeUVRow_ = MergeUVRow_NEON;
			}
		}
#endif
		// Allocate a rows of uv.
		align_buffer_64(row_u, ((halfwidth + 15) & ~15) * 2);
		uint8* row_v = row_u + ((halfwidth + 15) & ~15);

		for (int y = 0; y < height - 1; y += 2) {
			ARGBToUVRow(src_argb, src_stride_argb, row_u, row_v, width);
			MergeUVRow_(row_v, row_u, dst_uv, halfwidth);
			ARGBToYRow(src_argb, dst_y, width);
			ARGBToYRow(src_argb + src_stride_argb, dst_y + dst_stride_y, width);
			src_argb += src_stride_argb * 2;
			dst_y += dst_stride_y * 2;
			dst_uv += dst_stride_uv;
		}
		if (height & 1) {
			ARGBToUVRow(src_argb, 0, row_u, row_v, width);
			MergeUVRow_(row_v, row_u, dst_uv, halfwidth);
			ARGBToYRow(src_argb, dst_y, width);
		}
		free_aligned_buffer_64(row_u);
		return 0;
}

int NV21ToARGB(const uint8* src_y, int src_stride_y,
	const uint8* src_uv, int src_stride_uv,
	uint8* dst_argb, int dst_stride_argb,
	int width, int height) {
		if (!src_y || !src_uv || !dst_argb ||
			width <= 0 || height == 0) {
				return -1;
		}
		// Negative height means invert the image.
		if (height < 0) {
			height = -height;
			dst_argb = dst_argb + (height - 1) * dst_stride_argb;
			dst_stride_argb = -dst_stride_argb;
		}
		void (*NV21ToARGBRow)(const uint8* y_buf,
			const uint8* uv_buf,
			uint8* rgb_buf,
			int width) = NV21ToARGBRow_C;
#if defined(HAVE_NEON)
		if (width >= 8) {
			NV21ToARGBRow = NV21ToARGBRow_Any_NEON;
			if (IS_ALIGNED(width, 8)) {
				NV21ToARGBRow = NV21ToARGBRow_NEON;
			}
		}
#endif
		for (int y = 0; y < height; ++y) {
			NV21ToARGBRow(src_y, src_uv, dst_argb, width);
			dst_argb += dst_stride_argb;
			src_y += src_stride_y;
			if (y & 1) {
				src_uv += src_stride_uv;
			}
		}
		return 0;
}

int ARGBToYUY2(const uint8* src_argb, int src_stride_argb,
	uint8* dst_yuy2, int dst_stride_yuy2,
	int width, int height) {
		if (!src_argb || !dst_yuy2 ||
			width <= 0 || height == 0) {
				return -1;
		}
		// Negative height means invert the image.
		if (height < 0) {
			height = -height;
			dst_yuy2 = dst_yuy2 + (height - 1) * dst_stride_yuy2;
			dst_stride_yuy2 = -dst_stride_yuy2;
		}
		// Coalesce rows.
		if (src_stride_argb == width * 4 &&
			dst_stride_yuy2 == width * 2) {
				width *= height;
				height = 1;
				src_stride_argb = dst_stride_yuy2 = 0;
		}
		void (*ARGBToUV422Row)(const uint8* src_argb, uint8* dst_u, uint8* dst_v,
			int pix) = ARGBToUV422Row_C;

		void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix) =
			ARGBToYRow_C;
#if defined(HAVE_NEON)
		if (width >= 8) 
		{
			ARGBToYRow = ARGBToYRow_Any_NEON;
			if (IS_ALIGNED(width, 8)) 
			{
				ARGBToYRow = ARGBToYRow_NEON;
			}
			if (width >= 16) 
			{
				ARGBToUV422Row = ARGBToUV422Row_Any_NEON;
				if (IS_ALIGNED(width, 16)) 
				{
					ARGBToUV422Row = ARGBToUV422Row_NEON;
				}
			}
		}
#endif

		void (*I422ToYUY2Row)(const uint8* src_y, const uint8* src_u,
			const uint8* src_v, uint8* dst_yuy2, int width) =
			I422ToYUY2Row_C;
#if defined(HAVE_NEON)
		if ( width >= 16) {
			I422ToYUY2Row = I422ToYUY2Row_Any_NEON;
			if (IS_ALIGNED(width, 16)) {
				I422ToYUY2Row = I422ToYUY2Row_NEON;
			}
		}
#endif
		// Allocate a rows of yuv.
		align_buffer_64(row_y, ((width + 63) & ~63) * 2);
		uint8* row_u = row_y + ((width + 63) & ~63);
		uint8* row_v = row_u + ((width + 63) & ~63) / 2;

		for (int y = 0; y < height; ++y) {
			ARGBToUV422Row(src_argb, row_u, row_v, width);
			ARGBToYRow(src_argb, row_y, width);
			I422ToYUY2Row(row_y, row_u, row_v, dst_yuy2, width);
			src_argb += src_stride_argb;
			dst_yuy2 += dst_stride_yuy2;
		}

		free_aligned_buffer_64(row_y);
		return 0;
}

int YUY2ToARGB(const uint8* src_yuy2, int src_stride_yuy2,
	uint8* dst_argb, int dst_stride_argb,
	int width, int height) {
		if (!src_yuy2 || !dst_argb ||
			width <= 0 || height == 0) {
				return -1;
		}
		// Negative height means invert the image.
		if (height < 0) {
			height = -height;
			src_yuy2 = src_yuy2 + (height - 1) * src_stride_yuy2;
			src_stride_yuy2 = -src_stride_yuy2;
		}
		// Coalesce rows.
		if (src_stride_yuy2 == width * 2 &&
			dst_stride_argb == width * 4) {
				width *= height;
				height = 1;
				src_stride_yuy2 = dst_stride_argb = 0;
		}
		void (*YUY2ToARGBRow)(const uint8* src_yuy2, uint8* dst_argb, int pix) =
			YUY2ToARGBRow_C;

#if defined(HAVE_NEON)
		if (width >= 8) {
			YUY2ToARGBRow = YUY2ToARGBRow_Any_NEON;
			if (IS_ALIGNED(width, 8)) {
				YUY2ToARGBRow = YUY2ToARGBRow_NEON;
			}
		}
#endif
		for (int y = 0; y < height; ++y) {
			YUY2ToARGBRow(src_yuy2, dst_argb, width);
			src_yuy2 += src_stride_yuy2;
			dst_argb += dst_stride_argb;
		}
		return 0;
}

int ARGBToI420(const uint8* src_argb, int src_stride_argb,
	uint8* dst_y, int dst_stride_y,
	uint8* dst_u, int dst_stride_u,
	uint8* dst_v, int dst_stride_v,
	int width, int height) {
		if (!src_argb ||
			!dst_y || !dst_u || !dst_v ||
			width <= 0 || height == 0) {
				return -1;
		}
		// Negative height means invert the image.
		if (height < 0) {
			height = -height;
			src_argb = src_argb + (height - 1) * src_stride_argb;
			src_stride_argb = -src_stride_argb;
		}
		void (*ARGBToUVRow)(const uint8* src_argb0, int src_stride_argb,
			uint8* dst_u, uint8* dst_v, int width) = ARGBToUVRow_C;
		void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix) =
			ARGBToYRow_C;
#if defined(HAVE_NEON)
		if (width >= 8) {
			ARGBToYRow = ARGBToYRow_Any_NEON;
			if (IS_ALIGNED(width, 8)) {
				ARGBToYRow = ARGBToYRow_NEON;
			}
			if (width >= 16) {
				ARGBToUVRow = ARGBToUVRow_Any_NEON;
				if (IS_ALIGNED(width, 16)) {
					ARGBToUVRow = ARGBToUVRow_NEON;
				}
			}
		}
#endif
		for (int y = 0; y < height - 1; y += 2) {
			ARGBToUVRow(src_argb, src_stride_argb, dst_u, dst_v, width);
			ARGBToYRow(src_argb, dst_y, width);
			ARGBToYRow(src_argb + src_stride_argb, dst_y + dst_stride_y, width);
			src_argb += src_stride_argb * 2;
			dst_y += dst_stride_y * 2;
			dst_u += dst_stride_u;
			dst_v += dst_stride_v;
		}
		if (height & 1) {
			ARGBToUVRow(src_argb, 0, dst_u, dst_v, width);
			ARGBToYRow(src_argb, dst_y, width);
		}
		return 0;
}

int I420ToARGB(const uint8* src_y, int src_stride_y,
	const uint8* src_u, int src_stride_u,
	const uint8* src_v, int src_stride_v,
	uint8* dst_argb, int dst_stride_argb,
	int width, int height) {
		if (!src_y || !src_u || !src_v || !dst_argb ||
			width <= 0 || height == 0) {
				return -1;
		}
		// Negative height means invert the image.
		if (height < 0) {
			height = -height;
			dst_argb = dst_argb + (height - 1) * dst_stride_argb;
			dst_stride_argb = -dst_stride_argb;
		}
		void (*I422ToARGBRow)(const uint8* y_buf,
			const uint8* u_buf,
			const uint8* v_buf,
			uint8* rgb_buf,
			int width) = I422ToARGBRow_C;
#if defined(HAVE_NEON)
		if (width >= 8) {
			I422ToARGBRow = I422ToARGBRow_Any_NEON;
			if (IS_ALIGNED(width, 8)) {
				I422ToARGBRow = I422ToARGBRow_NEON;
			}
		}

#endif
		for (int y = 0; y < height; ++y) {
			I422ToARGBRow(src_y, src_u, src_v, dst_argb, width);
			dst_argb += dst_stride_argb;
			src_y += src_stride_y;
			if (y & 1) {
				src_u += src_stride_u;
				src_v += src_stride_v;
			}
		}
		return 0;
}

int YUY2ToI422(const uint8* src_yuy2, int src_stride_yuy2,
	uint8* dst_y, int dst_stride_y,
	uint8* dst_u, int dst_stride_u,
	uint8* dst_v, int dst_stride_v,
	int width, int height) {
		// Negative height means invert the image.
		if (height < 0) {
			height = -height;
			src_yuy2 = src_yuy2 + (height - 1) * src_stride_yuy2;
			src_stride_yuy2 = -src_stride_yuy2;
		}
		// Coalesce rows.
		if (src_stride_yuy2 == width * 2 &&
			dst_stride_y == width &&
			dst_stride_u * 2 == width &&
			dst_stride_v * 2 == width) {
				width *= height;
				height = 1;
				src_stride_yuy2 = dst_stride_y = dst_stride_u = dst_stride_v = 0;
		}
		void (*YUY2ToUV422Row)(const uint8* src_yuy2,
			uint8* dst_u, uint8* dst_v, int pix);
		void (*YUY2ToYRow)(const uint8* src_yuy2,
			uint8* dst_y, int pix);
		YUY2ToYRow = YUY2ToYRow_C;
		YUY2ToUV422Row = YUY2ToUV422Row_C;
#if defined(HAVE_NEON)
		if (width >= 8) {
			YUY2ToYRow = YUY2ToYRow_Any_NEON;
			if (width >= 16) {
				YUY2ToUV422Row = YUY2ToUV422Row_Any_NEON;
			}
			if (IS_ALIGNED(width, 16)) {
				YUY2ToYRow = YUY2ToYRow_NEON;
				YUY2ToUV422Row = YUY2ToUV422Row_NEON;
			}
		}
#endif
		for (int y = 0; y < height; ++y) {
			YUY2ToUV422Row(src_yuy2, dst_u, dst_v, width);
			YUY2ToYRow(src_yuy2, dst_y, width);
			src_yuy2 += src_stride_yuy2;
			dst_y += dst_stride_y;
			dst_u += dst_stride_u;
			dst_v += dst_stride_v;
		}
		return 0;
}

int I422ToYUY2(const uint8* src_y, int src_stride_y,
	const uint8* src_u, int src_stride_u,
	const uint8* src_v, int src_stride_v,
	uint8* dst_yuy2, int dst_stride_yuy2,
	int width, int height) {
		if (!src_y || !src_u || !src_v || !dst_yuy2 ||
			width <= 0 || height == 0) {
				return -1;
		}
		// Negative height means invert the image.
		if (height < 0) {
			height = -height;
			dst_yuy2 = dst_yuy2 + (height - 1) * dst_stride_yuy2;
			dst_stride_yuy2 = -dst_stride_yuy2;
		}
		// Coalesce rows.
		if (src_stride_y == width &&
			src_stride_u * 2 == width &&
			src_stride_v * 2 == width &&
			dst_stride_yuy2 == width * 2) {
				width *= height;
				height = 1;
				src_stride_y = src_stride_u = src_stride_v = dst_stride_yuy2 = 0;
		}
		void (*I422ToYUY2Row)(const uint8* src_y, const uint8* src_u,
			const uint8* src_v, uint8* dst_yuy2, int width) =
			I422ToYUY2Row_C;
#if defined(HAVE_NEON)
		if (width >= 16) {
			I422ToYUY2Row = I422ToYUY2Row_Any_NEON;
			if (IS_ALIGNED(width, 16)) {
				I422ToYUY2Row = I422ToYUY2Row_NEON;
			}
		}
#endif
		for (int y = 0; y < height; ++y) {
			I422ToYUY2Row(src_y, src_u, src_v, dst_yuy2, width);
			src_y += src_stride_y;
			src_u += src_stride_u;
			src_v += src_stride_v;
			dst_yuy2 += dst_stride_yuy2;
		}
		return 0;
}


int I444ToYUY2(const uint8* src_y, int src_stride_y,
	const uint8* src_u, int src_stride_u,
	const uint8* src_v, int src_stride_v,
	uint8* dst_yuy2, int dst_stride_yuy2,
	int width, int height)
{
	if (!src_y || !src_u || !src_v || !dst_yuy2 ||
		width <= 0 || height == 0) {
			return -1;
	}
	// Negative height means invert the image.
	if (height < 0) {
		height = -height;
		dst_yuy2 = dst_yuy2 + (height - 1) * dst_stride_yuy2;
		dst_stride_yuy2 = -dst_stride_yuy2;
	}
	void (*I422ToYUY2Row)(const uint8* src_y, const uint8* src_u,
		const uint8* src_v, uint8* dst_yuy2, int width) =
		I444ToYUY2Row_C;

	int y;
	for (y = 0; y < height; ++y) {
		I422ToYUY2Row(src_y, src_u, src_v, dst_yuy2, width);
		src_y += src_stride_y;
		src_u += src_stride_u;
		src_v += src_stride_v;
		dst_yuy2 += dst_stride_yuy2;
	}
	return 0;
}

int YUY2ToI444(const uint8* src_yuy2, int src_stride_yuy2,
	uint8* dst_y, int dst_stride_y,
	uint8* dst_u, int dst_stride_u,
	uint8* dst_v, int dst_stride_v,
	int width, int height)
{
	// Negative height means invert the image.
	if (height < 0) {
		height = -height;
		src_yuy2 = src_yuy2 + (height - 1) * src_stride_yuy2;
		src_stride_yuy2 = -src_stride_yuy2;
	}
	void (*YUY2ToUV422Row)(const uint8* src_yuy2,
		uint8* dst_u, uint8* dst_v, int pix);
	void (*YUY2ToYRow)(const uint8* src_yuy2,
		uint8* dst_y, int pix);
	YUY2ToYRow = YUY2ToYRow_C;
	YUY2ToUV422Row = YUY2ToUV444Row_C;

	int y;
	for (y = 0; y < height; ++y) {
		YUY2ToUV422Row(src_yuy2, dst_u, dst_v, width);
		YUY2ToYRow(src_yuy2, dst_y, width);
		src_yuy2 += src_stride_yuy2;
		dst_y += dst_stride_y;
		dst_u += dst_stride_u;
		dst_v += dst_stride_v;
	}

	return 0;	
}

int ARGBToRGB565(const uint8* src_argb, int src_stride_argb,
	uint8* dst_rgb565, int dst_stride_rgb565,
	int width, int height) {
		if (!src_argb || !dst_rgb565 || width <= 0 || height == 0) {
			return -1;
		}
		if (height < 0) {
			height = -height;
			src_argb = src_argb + (height - 1) * src_stride_argb;
			src_stride_argb = -src_stride_argb;
		}
		// Coalesce rows.
		if (src_stride_argb == width * 4 &&
			dst_stride_rgb565 == width * 2) {
				width *= height;
				height = 1;
				src_stride_argb = dst_stride_rgb565 = 0;
		}
		void (*ARGBToRGB565Row)(const uint8* src_argb, uint8* dst_rgb, int pix) =
			ARGBToRGB565Row_C;
#if defined(HAVE_NEON)
	if (width >= 8) {
		ARGBToRGB565Row = ARGBToRGB565Row_Any_NEON;
		if (IS_ALIGNED(width, 8)) {
			ARGBToRGB565Row = ARGBToRGB565Row_NEON;
		}
	}
#endif
		for (int y = 0; y < height; ++y) {
			ARGBToRGB565Row(src_argb, dst_rgb565, width);
			src_argb += src_stride_argb;
			dst_rgb565 += dst_stride_rgb565;
		}
		return 0;
}

int RGB565ToARGB(const uint8* src_rgb565, int src_stride_rgb565,
	uint8* dst_argb, int dst_stride_argb,
	int width, int height) {
		if (!src_rgb565 || !dst_argb ||
			width <= 0 || height == 0) {
				return -1;
		}
		// Negative height means invert the image.
		if (height < 0) {
			height = -height;
			src_rgb565 = src_rgb565 + (height - 1) * src_stride_rgb565;
			src_stride_rgb565 = -src_stride_rgb565;
		}
		// Coalesce rows.
		if (src_stride_rgb565 == width * 2 &&
			dst_stride_argb == width * 4) {
				width *= height;
				height = 1;
				src_stride_rgb565 = dst_stride_argb = 0;
		}
		void (*RGB565ToARGBRow)(const uint8* src_rgb565, uint8* dst_argb, int pix) =
			RGB565ToARGBRow_C;
#if defined(HAVE_NEON)
		if (width >= 8) {
			RGB565ToARGBRow = RGB565ToARGBRow_Any_NEON;
			if (IS_ALIGNED(width, 8)) {
				RGB565ToARGBRow = RGB565ToARGBRow_NEON;
			}
		}
#endif
		for (int y = 0; y < height; ++y) {
			RGB565ToARGBRow(src_rgb565, dst_argb, width);
			src_rgb565 += src_stride_rgb565;
			dst_argb += dst_stride_argb;
		}
		return 0;
}

int ARGBToI444(const uint8* src_argb, int src_stride_argb,
	uint8* dst_y, int dst_stride_y,
	uint8* dst_u, int dst_stride_u,
	uint8* dst_v, int dst_stride_v,
	int width, int height) {
		if (!src_argb || !dst_y || !dst_u || !dst_v || width <= 0 || height == 0) {
			return -1;
		}
		if (height < 0) {
			height = -height;
			src_argb = src_argb + (height - 1) * src_stride_argb;
			src_stride_argb = -src_stride_argb;
		}
		// Coalesce rows.
		if (src_stride_argb == width * 4 &&
			dst_stride_y == width &&
			dst_stride_u == width &&
			dst_stride_v == width) {
				width *= height;
				height = 1;
				src_stride_argb = dst_stride_y = dst_stride_u = dst_stride_v = 0;
		}
		void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix) =
			ARGBToYRow_C;
		void (*ARGBToUV444Row)(const uint8* src_argb, uint8* dst_u, uint8* dst_v,
			int pix) = ARGBToUV444Row_C;

		for (int y = 0; y < height; ++y) {
			ARGBToUV444Row(src_argb, dst_u, dst_v, width);
			ARGBToYRow(src_argb, dst_y, width);
			src_argb += src_stride_argb;
			dst_y += dst_stride_y;
			dst_u += dst_stride_u;
			dst_v += dst_stride_v;
		}
		return 0;
}
int I444ToARGB(const uint8* src_y, int src_stride_y,
	const uint8* src_u, int src_stride_u,
	const uint8* src_v, int src_stride_v,
	uint8* dst_argb, int dst_stride_argb,
	int width, int height) {
		if (!src_y || !src_u || !src_v ||
			!dst_argb ||
			width <= 0 || height == 0) {
				return -1;
		}
		// Negative height means invert the image.
		if (height < 0) {
			height = -height;
			dst_argb = dst_argb + (height - 1) * dst_stride_argb;
			dst_stride_argb = -dst_stride_argb;
		}
		// Coalesce rows.
		if (src_stride_y == width &&
			src_stride_u == width &&
			src_stride_v == width &&
			dst_stride_argb == width * 4) {
				width *= height;
				height = 1;
				src_stride_y = src_stride_u = src_stride_v = dst_stride_argb = 0;
		}
		void (*I444ToARGBRow)(const uint8* y_buf,
			const uint8* u_buf,
			const uint8* v_buf,
			uint8* rgb_buf,
			int width) = I444ToARGBRow_C;

		for (int y = 0; y < height; ++y) {
			I444ToARGBRow(src_y, src_u, src_v, dst_argb, width);
			dst_argb += dst_stride_argb;
			src_y += src_stride_y;
			src_u += src_stride_u;
			src_v += src_stride_v;
		}
		return 0;
}

int ARGBToUYVY(const uint8* src_argb, int src_stride_argb,
	uint8* dst_uyvy, int dst_stride_uyvy,
	int width, int height) {
		if (!src_argb || !dst_uyvy ||
			width <= 0 || height == 0) {
				return -1;
		}
		// Negative height means invert the image.
		if (height < 0) {
			height = -height;
			dst_uyvy = dst_uyvy + (height - 1) * dst_stride_uyvy;
			dst_stride_uyvy = -dst_stride_uyvy;
		}
		// Coalesce rows.
		if (src_stride_argb == width * 4 &&
			dst_stride_uyvy == width * 2) {
				width *= height;
				height = 1;
				src_stride_argb = dst_stride_uyvy = 0;
		}
		void (*ARGBToUV422Row)(const uint8* src_argb, uint8* dst_u, uint8* dst_v,
			int pix) = ARGBToUV422Row_C;

		void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix) =
			ARGBToYRow_C;


		void (*I422ToUYVYRow)(const uint8* src_y, const uint8* src_u,
			const uint8* src_v, uint8* dst_uyvy, int width) =
			I422ToUYVYRow_C;

		// Allocate a rows of yuv.
		align_buffer_64(row_y, ((width + 63) & ~63) * 2);
		uint8* row_u = row_y + ((width + 63) & ~63);
		uint8* row_v = row_u + ((width + 63) & ~63) / 2;

		for (int y = 0; y < height; ++y) {
			ARGBToUV422Row(src_argb, row_u, row_v, width);
			ARGBToYRow(src_argb, row_y, width);
			I422ToUYVYRow(row_y, row_u, row_v, dst_uyvy, width);
			src_argb += src_stride_argb;
			dst_uyvy += dst_stride_uyvy;
		}

		free_aligned_buffer_64(row_y);
		return 0;
}

int UYVYToARGB(const uint8* src_uyvy, int src_stride_uyvy,
	uint8* dst_argb, int dst_stride_argb,
	int width, int height) {
		if (!src_uyvy || !dst_argb ||
			width <= 0 || height == 0) {
				return -1;
		}
		// Negative height means invert the image.
		if (height < 0) {
			height = -height;
			src_uyvy = src_uyvy + (height - 1) * src_stride_uyvy;
			src_stride_uyvy = -src_stride_uyvy;
		}
		// Coalesce rows.
		if (src_stride_uyvy == width * 2 &&
			dst_stride_argb == width * 4) {
				width *= height;
				height = 1;
				src_stride_uyvy = dst_stride_argb = 0;
		}
		void (*UYVYToARGBRow)(const uint8* src_uyvy, uint8* dst_argb, int pix) =
			UYVYToARGBRow_C;

		for (int y = 0; y < height; ++y) {
			UYVYToARGBRow(src_uyvy, dst_argb, width);
			src_uyvy += src_stride_uyvy;
			dst_argb += dst_stride_argb;
		}
		return 0;
}
#ifdef __cplusplus
}
#endif