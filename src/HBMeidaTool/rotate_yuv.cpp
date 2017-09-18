#include "rotate_yuv.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
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

#ifndef IS_ALIGNED
#define IS_ALIGNED(p, a) (!((uintptr_t)(p) & ((a) - 1)))
#endif

static void CopyRow_C(const uint8* src, uint8* dst, int count) {
	memcpy(dst, src, count);
}

//////////////////////////////////////////////////////////////////////////
///  NEON
//////////////////////////////////////////////////////////////////////////


#ifdef HAVE_NEON
#include <arm_neon.h>
// Copy multiple of 32.  vld4.8  allow unaligned and is fastest on a15.
static void CopyRow_NEON(const uint8* src, uint8* dst, int count) {
	asm volatile (
		".p2align   2                              \n"
		"1:                                          \n"
		"vld1.8     {d0, d1, d2, d3}, [%0]!        \n"  // load 32
		"subs       %2, %2, #32                    \n"  // 32 processed per loop
		"vst1.8     {d0, d1, d2, d3}, [%1]!        \n"  // store 32
		"bgt        1b                             \n"
		: "+r"(src),   // %0
		"+r"(dst),   // %1
		"+r"(count)  // %2  // Output registers
		:                     // Input registers
	: "cc", "memory", "q0", "q1"  // Clobber List
		);
}
typedef uint8 uvec8[16];
static uvec8 kVTbl4x4Transpose =
{ 0,  4,  8, 12,  1,  5,  9, 13,  2,  6, 10, 14,  3,  7, 11, 15 };

static void TransposeWx8_NEON(const uint8* src, int src_stride,
	uint8* dst, int dst_stride,
	int width) {
		asm volatile (
			// loops are on blocks of 8. loop will stop when
			// counter gets to or below 0. starting the counter
			// at w-8 allow for this
			"sub         %4, #8                        \n"

			// handle 8x8 blocks. this should be the majority of the plane
			".p2align  2                               \n"
			"1:                                        \n"
			"mov         r9, %0                      \n"

			"vld1.8      {d0}, [r9], %1              \n"
			"vld1.8      {d1}, [r9], %1              \n"
			"vld1.8      {d2}, [r9], %1              \n"
			"vld1.8      {d3}, [r9], %1              \n"
			"vld1.8      {d4}, [r9], %1              \n"
			"vld1.8      {d5}, [r9], %1              \n"
			"vld1.8      {d6}, [r9], %1              \n"
			"vld1.8      {d7}, [r9]                  \n"

			"vtrn.8      d1, d0                      \n"
			"vtrn.8      d3, d2                      \n"
			"vtrn.8      d5, d4                      \n"
			"vtrn.8      d7, d6                      \n"

			"vtrn.16     d1, d3                      \n"
			"vtrn.16     d0, d2                      \n"
			"vtrn.16     d5, d7                      \n"
			"vtrn.16     d4, d6                      \n"

			"vtrn.32     d1, d5                      \n"
			"vtrn.32     d0, d4                      \n"
			"vtrn.32     d3, d7                      \n"
			"vtrn.32     d2, d6                      \n"

			"vrev16.8    q0, q0                      \n"
			"vrev16.8    q1, q1                      \n"
			"vrev16.8    q2, q2                      \n"
			"vrev16.8    q3, q3                      \n"

			"mov         r9, %2                      \n"

			"vst1.8      {d1}, [r9], %3              \n"
			"vst1.8      {d0}, [r9], %3              \n"
			"vst1.8      {d3}, [r9], %3              \n"
			"vst1.8      {d2}, [r9], %3              \n"
			"vst1.8      {d5}, [r9], %3              \n"
			"vst1.8      {d4}, [r9], %3              \n"
			"vst1.8      {d7}, [r9], %3              \n"
			"vst1.8      {d6}, [r9]                  \n"

			"add         %0, #8                      \n"  // src += 8
			"add         %2, %2, %3, lsl #3          \n"  // dst += 8 * dst_stride
			"subs        %4,  #8                     \n"  // w   -= 8
			"bge         1b                          \n"

			// add 8 back to counter. if the result is 0 there are
			// no residuals.
			"adds        %4, #8                        \n"
			"beq         4f                            \n"

			// some residual, so between 1 and 7 lines left to transpose
			"cmp         %4, #2                        \n"
			"blt         3f                            \n"

			"cmp         %4, #4                        \n"
			"blt         2f                            \n"

			// 4x8 block
			"mov         r9, %0                        \n"
			"vld1.32     {d0[0]}, [r9], %1             \n"
			"vld1.32     {d0[1]}, [r9], %1             \n"
			"vld1.32     {d1[0]}, [r9], %1             \n"
			"vld1.32     {d1[1]}, [r9], %1             \n"
			"vld1.32     {d2[0]}, [r9], %1             \n"
			"vld1.32     {d2[1]}, [r9], %1             \n"
			"vld1.32     {d3[0]}, [r9], %1             \n"
			"vld1.32     {d3[1]}, [r9]                 \n"

			"mov         r9, %2                        \n"

			"vld1.8      {q3}, [%5]                    \n"

			"vtbl.8      d4, {d0, d1}, d6              \n"
			"vtbl.8      d5, {d0, d1}, d7              \n"
			"vtbl.8      d0, {d2, d3}, d6              \n"
			"vtbl.8      d1, {d2, d3}, d7              \n"

			// TODO(frkoenig): Rework shuffle above to
			// write out with 4 instead of 8 writes.
			"vst1.32     {d4[0]}, [r9], %3             \n"
			"vst1.32     {d4[1]}, [r9], %3             \n"
			"vst1.32     {d5[0]}, [r9], %3             \n"
			"vst1.32     {d5[1]}, [r9]                 \n"

			"add         r9, %2, #4                    \n"
			"vst1.32     {d0[0]}, [r9], %3             \n"
			"vst1.32     {d0[1]}, [r9], %3             \n"
			"vst1.32     {d1[0]}, [r9], %3             \n"
			"vst1.32     {d1[1]}, [r9]                 \n"

			"add         %0, #4                        \n"  // src += 4
			"add         %2, %2, %3, lsl #2            \n"  // dst += 4 * dst_stride
			"subs        %4,  #4                       \n"  // w   -= 4
			"beq         4f                            \n"

			// some residual, check to see if it includes a 2x8 block,
			// or less
			"cmp         %4, #2                        \n"
			"blt         3f                            \n"

			// 2x8 block
			"2:                                        \n"
			"mov         r9, %0                        \n"
			"vld1.16     {d0[0]}, [r9], %1             \n"
			"vld1.16     {d1[0]}, [r9], %1             \n"
			"vld1.16     {d0[1]}, [r9], %1             \n"
			"vld1.16     {d1[1]}, [r9], %1             \n"
			"vld1.16     {d0[2]}, [r9], %1             \n"
			"vld1.16     {d1[2]}, [r9], %1             \n"
			"vld1.16     {d0[3]}, [r9], %1             \n"
			"vld1.16     {d1[3]}, [r9]                 \n"

			"vtrn.8      d0, d1                        \n"

			"mov         r9, %2                        \n"

			"vst1.64     {d0}, [r9], %3                \n"
			"vst1.64     {d1}, [r9]                    \n"

			"add         %0, #2                        \n"  // src += 2
			"add         %2, %2, %3, lsl #1            \n"  // dst += 2 * dst_stride
			"subs        %4,  #2                       \n"  // w   -= 2
			"beq         4f                            \n"

			// 1x8 block
			"3:                                        \n"
			"vld1.8      {d0[0]}, [%0], %1             \n"
			"vld1.8      {d0[1]}, [%0], %1             \n"
			"vld1.8      {d0[2]}, [%0], %1             \n"
			"vld1.8      {d0[3]}, [%0], %1             \n"
			"vld1.8      {d0[4]}, [%0], %1             \n"
			"vld1.8      {d0[5]}, [%0], %1             \n"
			"vld1.8      {d0[6]}, [%0], %1             \n"
			"vld1.8      {d0[7]}, [%0]                 \n"

			"vst1.64     {d0}, [%2]                    \n"

			"4:                                        \n"

			: "+r"(src),               // %0
			"+r"(src_stride),        // %1
			"+r"(dst),               // %2
			"+r"(dst_stride),        // %3
			"+r"(width)              // %4
			: "r"(&kVTbl4x4Transpose)  // %5
			: "memory", "cc", "r9", "q0", "q1", "q2", "q3"
			);
}


static void MirrorRow_NEON(const uint8* src, uint8* dst, int width) {
	asm volatile (
		// Start at end of source row.
		"mov        r3, #-16                       \n"
		"add        %0, %0, %2                     \n"
		"sub        %0, #16                        \n"

		".p2align   2                              \n"
		"1:                                          \n"
		"vld1.8     {q0}, [%0], r3                 \n"  // src -= 16
		"subs       %2, #16                        \n"  // 16 pixels per loop.
		"vrev64.8   q0, q0                         \n"
		"vst1.8     {d1}, [%1]!                    \n"  // dst += 16
		"vst1.8     {d0}, [%1]!                    \n"
		"bgt        1b                             \n"
		: "+r"(src),   // %0
		"+r"(dst),   // %1
		"+r"(width)  // %2
		:
	: "cc", "memory", "r3", "q0"
		);
}


#endif

/////////////////////////////////END_NEON////////////////////////////////////

static void CopyPlane(const uint8* src_y, int src_stride_y,
	uint8* dst_y, int dst_stride_y,
	int width, int height) {
		// Coalesce rows.
		if (src_stride_y == width &&
			dst_stride_y == width) {
				width *= height;
				height = 1;
				src_stride_y = dst_stride_y = 0;
		}
		void (*CopyRow)(const uint8* src, uint8* dst, int width) = CopyRow_C;
#if defined(HAVE_NEON)
		if (IS_ALIGNED(width, 32)) {
			CopyRow = CopyRow_NEON;
		}
#endif
		// Copy plane
		for (int y = 0; y < height; ++y) {
			CopyRow(src_y, dst_y, width);
			src_y += src_stride_y;
			dst_y += dst_stride_y;
		}
}
static int I420Copy(const uint8* src_y, int src_stride_y,
	const uint8* src_u, int src_stride_u,
	const uint8* src_v, int src_stride_v,
	uint8* dst_y, int dst_stride_y,
	uint8* dst_u, int dst_stride_u,
	uint8* dst_v, int dst_stride_v,
	int width, int height) {
		if (!src_y || !src_u || !src_v ||
			!dst_y || !dst_u || !dst_v ||
			width <= 0 || height == 0) {
				return -1;
		}
		// Negative height means invert the image.
		if (height < 0) {
			height = -height;
			const int halfheight = (height + 1) >> 1;
			src_y = src_y + (height - 1) * src_stride_y;
			src_u = src_u + (halfheight - 1) * src_stride_u;
			src_v = src_v + (halfheight - 1) * src_stride_v;
			src_stride_y = -src_stride_y;
			src_stride_u = -src_stride_u;
			src_stride_v = -src_stride_v;
		}

		if (dst_y) {
			CopyPlane(src_y, src_stride_y, dst_y, dst_stride_y, width, height);
		}
		// Copy UV planes.
		const int halfwidth = (width + 1) >> 1;
		const int halfheight = (height + 1) >> 1;
		CopyPlane(src_u, src_stride_u, dst_u, dst_stride_u, halfwidth, halfheight);
		CopyPlane(src_v, src_stride_v, dst_v, dst_stride_v, halfwidth, halfheight);
		return 0;
}

static void MirrorRow_C(const uint8* src, uint8* dst, int width) {
	src += width - 1;
	for (int x = 0; x < width - 1; x += 2) {
		dst[x] = src[0];
		dst[x + 1] = src[-1];
		src -= 2;
	}
	if (width & 1) {
		dst[width - 1] = src[0];
	}
}

static void TransposeWx8_C(const uint8* src, int src_stride,
	uint8* dst, int dst_stride,
	int width) {
		for (int i = 0; i < width; ++i) {
			dst[0] = src[0 * src_stride];
			dst[1] = src[1 * src_stride];
			dst[2] = src[2 * src_stride];
			dst[3] = src[3 * src_stride];
			dst[4] = src[4 * src_stride];
			dst[5] = src[5 * src_stride];
			dst[6] = src[6 * src_stride];
			dst[7] = src[7 * src_stride];
			++src;
			dst += dst_stride;
		}
}

static void TransposeWxH_C(const uint8* src, int src_stride,
	uint8* dst, int dst_stride,
	int width, int height) {
		for (int i = 0; i < width; ++i) {
			for (int j = 0; j < height; ++j) {
				dst[i * dst_stride + j] = src[j * src_stride + i];
			}
		}
}

static void TransposePlane(const uint8* src, int src_stride,
	uint8* dst, int dst_stride,
	int width, int height) {
		void (*TransposeWx8)(const uint8* src, int src_stride,
			uint8* dst, int dst_stride,
			int width) = TransposeWx8_C;
#if defined(HAVE_NEON)
			TransposeWx8 = TransposeWx8_NEON;
#endif
		// Work across the source in 8x8 tiles
		int i = height;
		while (i >= 8) {
			TransposeWx8(src, src_stride, dst, dst_stride, width);
			src += 8 * src_stride;    // Go down 8 rows.
			dst += 8;                 // Move over 8 columns.
			i -= 8;
		}

		TransposeWxH_C(src, src_stride, dst, dst_stride, width, i);
}
void RotatePlane90(const uint8* src, int src_stride,
	uint8* dst, int dst_stride,
	int width, int height) {
		// Rotate by 90 is a transpose with the source read
		// from bottom to top. So set the source pointer to the end
		// of the buffer and flip the sign of the source stride.
		src += src_stride * (height - 1);
		src_stride = -src_stride;
		TransposePlane(src, src_stride, dst, dst_stride, width, height);
}

void RotatePlane270(const uint8* src, int src_stride,
	uint8* dst, int dst_stride,
	int width, int height) {
		// Rotate by 270 is a transpose with the destination written
		// from bottom to top. So set the destination pointer to the end
		// of the buffer and flip the sign of the destination stride.
		dst += dst_stride * (width - 1);
		dst_stride = -dst_stride;
		TransposePlane(src, src_stride, dst, dst_stride, width, height);
}
void RotatePlane180(const uint8* src, int src_stride,
	uint8* dst, int dst_stride,
	int width, int height) {
		void (*MirrorRow)(const uint8* src, uint8* dst, int width) = MirrorRow_C;
#if defined(HAVE_NEON)
		if (IS_ALIGNED(width, 16)) {
			MirrorRow = MirrorRow_NEON;
		}
#endif
		void (*CopyRow)(const uint8* src, uint8* dst, int width) = CopyRow_C;
#if defined(HAVE_NEON)
		if (IS_ALIGNED(width, 32)) {
			CopyRow = CopyRow_NEON;
		}
#endif

		// Swap first and last row and mirror the content. Uses a temporary row.
		align_buffer_64(row, width);
		const uint8* src_bot = src + src_stride * (height - 1);
		uint8* dst_bot = dst + dst_stride * (height - 1);
		int half_height = (height + 1) >> 1;
		// Odd height will harmlessly mirror the middle row twice.
		for (int y = 0; y < half_height; ++y) {
			MirrorRow(src, row, width);  // Mirror first row into a buffer
			src += src_stride;
			MirrorRow(src_bot, dst, width);  // Mirror last row into first row
			dst += dst_stride;
			CopyRow(row, dst_bot, width);  // Copy first mirrored row into last
			src_bot -= src_stride;
			dst_bot -= dst_stride;
		}
		free_aligned_buffer_64(row);
}
int I420Rotate(const uint8* src_y, int src_stride_y,
	const uint8* src_u, int src_stride_u,
	const uint8* src_v, int src_stride_v,
	uint8* dst_y, int dst_stride_y,
	uint8* dst_u, int dst_stride_u,
	uint8* dst_v, int dst_stride_v,
	int width, int height,
	RotationMode mode) {
		if (!src_y || !src_u || !src_v || width <= 0 || height == 0 ||
			!dst_y || !dst_u || !dst_v) {
				return -1;
		}
		int halfwidth = (width + 1) >> 1;
		int halfheight = (height + 1) >> 1;

		// Negative height means invert the image.
		if (height < 0) {
			height = -height;
			halfheight = (height + 1) >> 1;
			src_y = src_y + (height - 1) * src_stride_y;
			src_u = src_u + (halfheight - 1) * src_stride_u;
			src_v = src_v + (halfheight - 1) * src_stride_v;
			src_stride_y = -src_stride_y;
			src_stride_u = -src_stride_u;
			src_stride_v = -src_stride_v;
		}

		switch (mode) {
		case kRotate0:
			// copy frame
			return I420Copy(src_y, src_stride_y,
				src_u, src_stride_u,
				src_v, src_stride_v,
				dst_y, dst_stride_y,
				dst_u, dst_stride_u,
				dst_v, dst_stride_v,
				width, height);
		case kRotate90:
			RotatePlane90(src_y, src_stride_y,
				dst_y, dst_stride_y,
				width, height);
			RotatePlane90(src_u, src_stride_u,
				dst_u, dst_stride_u,
				halfwidth, halfheight);
			RotatePlane90(src_v, src_stride_v,
				dst_v, dst_stride_v,
				halfwidth, halfheight);
			return 0;
		case kRotate270:
			RotatePlane270(src_y, src_stride_y,
				dst_y, dst_stride_y,
				width, height);
			RotatePlane270(src_u, src_stride_u,
				dst_u, dst_stride_u,
				halfwidth, halfheight);
			RotatePlane270(src_v, src_stride_v,
				dst_v, dst_stride_v,
				halfwidth, halfheight);
			return 0;
		case kRotate180:
			RotatePlane180(src_y, src_stride_y,
				dst_y, dst_stride_y,
				width, height);
			RotatePlane180(src_u, src_stride_u,
				dst_u, dst_stride_u,
				halfwidth, halfheight);
			RotatePlane180(src_v, src_stride_v,
				dst_v, dst_stride_v,
				halfwidth, halfheight);
			return 0;
		default:
			break;
		}
		return -1;
}