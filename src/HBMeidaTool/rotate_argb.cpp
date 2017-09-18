
#include "rotate_argb.h"
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

static void CopyRow_C(const uint8* src, uint8* dst, int count) {
	memcpy(dst, src, count);
}

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

		// Copy plane
		for (int y = 0; y < height; ++y) {
			CopyRow(src_y, dst_y, width);
			src_y += src_stride_y;
			dst_y += dst_stride_y;
		}
}
static int ARGBCopy(const uint8* src_argb, int src_stride_argb,
	uint8* dst_argb, int dst_stride_argb,
	int width, int height) {
		if (!src_argb || !dst_argb ||
			width <= 0 || height == 0) {
				return -1;
		}
		// Negative height means invert the image.
		if (height < 0) {
			height = -height;
			src_argb = src_argb + (height - 1) * src_stride_argb;
			src_stride_argb = -src_stride_argb;
		}

		CopyPlane(src_argb, src_stride_argb, dst_argb, dst_stride_argb,
			width * 4, height);
		return 0;
}

static void ARGBMirrorRow_C(const uint8* src, uint8* dst, int width) {
	const uint32* src32 = reinterpret_cast<const uint32*>(src);
	uint32* dst32 = reinterpret_cast<uint32*>(dst);
	src32 += width - 1;
	for (int x = 0; x < width - 1; x += 2) {
		dst32[x] = src32[0];
		dst32[x + 1] = src32[-1];
		src32 -= 2;
	}
	if (width & 1) {
		dst32[width - 1] = src32[0];
	}
}

static void ScaleARGBRowDownEven_C(const uint8* src_argb, int /* src_stride */,
	int src_stepx,
	uint8* dst_argb, int dst_width) {
		const uint32* src = reinterpret_cast<const uint32*>(src_argb);
		uint32* dst = reinterpret_cast<uint32*>(dst_argb);

		for (int x = 0; x < dst_width - 1; x += 2) {
			dst[0] = src[0];
			dst[1] = src[src_stepx];
			src += src_stepx * 2;
			dst += 2;
		}
		if (dst_width & 1) {
			dst[0] = src[0];
		}
}

static void ARGBTranspose(const uint8* src, int src_stride,
	uint8* dst, int dst_stride,
	int width, int height) {
		int src_pixel_step = src_stride >> 2;
		void (*ScaleARGBRowDownEven)(const uint8* src_ptr, int src_stride,
			int src_step, uint8* dst_ptr, int dst_width) = ScaleARGBRowDownEven_C;

		for (int i = 0; i < width; ++i) {  // column of source to row of dest.
			ScaleARGBRowDownEven(src, 0, src_pixel_step, dst, height);
			dst += dst_stride;
			src += 4;
		}
}
void ARGBRotate90(const uint8* src, int src_stride,
	uint8* dst, int dst_stride,
	int width, int height) {
		// Rotate by 90 is a ARGBTranspose with the source read
		// from bottom to top. So set the source pointer to the end
		// of the buffer and flip the sign of the source stride.
		src += src_stride * (height - 1);
		src_stride = -src_stride;
		ARGBTranspose(src, src_stride, dst, dst_stride, width, height);
}

void ARGBRotate180(const uint8* src, int src_stride,
	uint8* dst, int dst_stride,
	int width, int height) {
		void (*ARGBMirrorRow)(const uint8* src, uint8* dst, int width) =
			ARGBMirrorRow_C;

		void (*CopyRow)(const uint8* src, uint8* dst, int width) = CopyRow_C;

		// Swap first and last row and mirror the content. Uses a temporary row.
		align_buffer_64(row, width * 4);
		const uint8* src_bot = src + src_stride * (height - 1);
		uint8* dst_bot = dst + dst_stride * (height - 1);
		int half_height = (height + 1) >> 1;
		// Odd height will harmlessly mirror the middle row twice.
		for (int y = 0; y < half_height; ++y) {
			ARGBMirrorRow(src, row, width);  // Mirror first row into a buffer
			ARGBMirrorRow(src_bot, dst, width);  // Mirror last row into first row
			CopyRow(row, dst_bot, width * 4);  // Copy first mirrored row into last
			src += src_stride;
			dst += dst_stride;
			src_bot -= src_stride;
			dst_bot -= dst_stride;
		}
		free_aligned_buffer_64(row);
}
void ARGBRotate270(const uint8* src, int src_stride,
	uint8* dst, int dst_stride,
	int width, int height) {
		// Rotate by 270 is a ARGBTranspose with the destination written
		// from bottom to top. So set the destination pointer to the end
		// of the buffer and flip the sign of the destination stride.
		dst += dst_stride * (width - 1);
		dst_stride = -dst_stride;
		ARGBTranspose(src, src_stride, dst, dst_stride, width, height);
}

int ARGBRotate(const uint8* src_argb, int src_stride_argb,
	uint8* dst_argb, int dst_stride_argb,
	int width, int height,
	RotationMode mode) {
		if (!src_argb || width <= 0 || height == 0 || !dst_argb) {
			return -1;
		}

		// Negative height means invert the image.
		if (height < 0) {
			height = -height;
			src_argb = src_argb + (height - 1) * src_stride_argb;
			src_stride_argb = -src_stride_argb;
		}

		switch (mode) {
		case kRotate0:
			// copy frame
			return ARGBCopy(src_argb, src_stride_argb,
				dst_argb, dst_stride_argb,
				width, height);
		case kRotate90:
			ARGBRotate90(src_argb, src_stride_argb,
				dst_argb, dst_stride_argb,
				width, height);
			return 0;
		case kRotate270:
			ARGBRotate270(src_argb, src_stride_argb,
				dst_argb, dst_stride_argb,
				width, height);
			return 0;
		case kRotate180:
			ARGBRotate180(src_argb, src_stride_argb,
				dst_argb, dst_stride_argb,
				width, height);
			return 0;
		default:
			break;
		}
		return -1;
}