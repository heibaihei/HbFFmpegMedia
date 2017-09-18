
#include "scale_yuv.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define SUBSAMPLE(v, a, s) (v < 0) ? (-((-v + a) >> s)) : ((v + a) >> s)
static __inline int Abs(int v) {
    return v >= 0 ? v : -v;
}
// Divide num by div and return as 16.16 fixed point result.
int FixedDiv_C(int num, int div) {
    return static_cast<int>((static_cast<long long>(num) << 16) / div);
}
#ifndef FIXEDDIV1
#define FIXEDDIV1(src, dst) FixedDiv((src << 16) - 0x00010001, \
(dst << 16) - 0x00010000);
#endif

#ifndef IS_ALIGNED
#define IS_ALIGNED(p, a) (!((uintptr_t)(p) & ((a) - 1)))
#endif

#ifndef FixedDiv
#define FixedDiv FixedDiv_C
#endif

#ifndef CENTERSTART
#define CENTERSTART(dx, s) (dx < 0) ? -((-dx >> 1) + s) : ((dx >> 1) + s)
#endif

#ifndef SAFE_FREE
#define SAFE_FREE(p) if(p != NULL) {free(p); p = NULL;}
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

// Simplify the filtering based on scale factors.
static FilterMode ScaleFilterReduce(int src_width, int src_height,
                                    int dst_width, int dst_height,
                                    FilterMode filtering) {
    if (src_width < 0) {
        src_width = -src_width;
    }
    if (src_height < 0) {
        src_height = -src_height;
    }
    if (filtering == kFilterBox) {
        // If scaling both axis to 0.5 or larger, switch from Box to Bilinear.
        if (dst_width * 2 >= src_width && dst_height * 2 >= src_height) {
            filtering = kFilterBilinear;
        }
        // If scaling to larger, switch from Box to Bilinear.
        if (dst_width >= src_width || dst_height >= src_height) {
            filtering = kFilterBilinear;
        }
    }
    if (filtering == kFilterBilinear) {
        if (src_height == 1) {
            filtering = kFilterLinear;
        }
        // TODO(fbarchard): Detect any odd scale factor and reduce to Linear.
        if (dst_height == src_height || dst_height * 3 == src_height) {
            filtering = kFilterLinear;
        }
        // TODO(fbarchard): Remove 1 pixel wide filter restriction, which is to
        // avoid reading 2 pixels horizontally that causes memory exception.
        if (src_width == 1) {
            filtering = kFilterNone;
        }
    }
    if (filtering == kFilterLinear) {
        if (src_width == 1) {
            filtering = kFilterNone;
        }
        // TODO(fbarchard): Detect any odd scale factor and reduce to None.
        if (dst_width == src_width || dst_width * 3 == src_width) {
            filtering = kFilterNone;
        }
    }
    return filtering;
}


// Compute slope values for stepping.
static void ScaleSlope(int src_width, int src_height,
                       int dst_width, int dst_height,
                       FilterMode filtering,
                       int* x, int* y, int* dx, int* dy) {
    assert(x != NULL);
    assert(y != NULL);
    assert(dx != NULL);
    assert(dy != NULL);
    assert(src_width != 0);
    assert(src_height != 0);
    assert(dst_width > 0);
    assert(dst_height > 0);
    if (filtering == kFilterBox) {
        // Scale step for point sampling duplicates all pixels equally.
        *dx = FixedDiv(Abs(src_width), dst_width);
        *dy = FixedDiv(src_height, dst_height);
        *x = 0;
        *y = 0;
    } else if (filtering == kFilterBilinear) {
        // Scale step for bilinear sampling renders last pixel once for upsample.
        if (dst_width <= Abs(src_width)) {
            *dx = FixedDiv(Abs(src_width), dst_width);
            *x = CENTERSTART(*dx, -32768);  // Subtract 0.5 (32768) to center filter.
        } else if (dst_width > 1) {
            *dx = FIXEDDIV1(Abs(src_width), dst_width);
            *x = 0;
        }
        if (dst_height <= src_height) {
            *dy = FixedDiv(src_height,  dst_height);
            *y = CENTERSTART(*dy, -32768);  // Subtract 0.5 (32768) to center filter.
        } else if (dst_height > 1) {
            *dy = FIXEDDIV1(src_height, dst_height);
            *y = 0;
        }
    } else if (filtering == kFilterLinear) {
        // Scale step for bilinear sampling renders last pixel once for upsample.
        if (dst_width <= Abs(src_width)) {
            *dx = FixedDiv(Abs(src_width), dst_width);
            *x = CENTERSTART(*dx, -32768);  // Subtract 0.5 (32768) to center filter.
        } else if (dst_width > 1) {
            *dx = FIXEDDIV1(Abs(src_width), dst_width);
            *x = 0;
        }
        *dy = FixedDiv(src_height, dst_height);
        *y = *dy >> 1;
    } else {
        // Scale step for point sampling duplicates all pixels equally.
        *dx = FixedDiv(Abs(src_width), dst_width);
        *dy = FixedDiv(src_height, dst_height);
        *x = CENTERSTART(*dx, 0);
        *y = CENTERSTART(*dy, 0);
    }
    // Negative src_width means horizontally mirror.
    if (src_width < 0) {
        *x += (dst_width - 1) * *dx;
        *dx = -*dx;
        src_width = -src_width;
    }
}

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

static void ScaleRowDown34_C(const uint8* src_ptr, int /* src_stride */,
                             uint8* dst, int dst_width) {
    assert((dst_width % 3 == 0) && (dst_width > 0));
    for (int x = 0; x < dst_width; x += 3) {
        dst[0] = src_ptr[0];
        dst[1] = src_ptr[1];
        dst[2] = src_ptr[3];
        dst += 3;
        src_ptr += 4;
    }
}

// Filter rows 0 and 1 together, 3 : 1
static void ScaleRowDown34_0_Box_C(const uint8* src_ptr, int src_stride,
                                   uint8* d, int dst_width) {
    assert((dst_width % 3 == 0) && (dst_width > 0));
    const uint8* s = src_ptr;
    const uint8* t = src_ptr + src_stride;
    for (int x = 0; x < dst_width; x += 3) {
        uint8 a0 = (s[0] * 3 + s[1] * 1 + 2) >> 2;
        uint8 a1 = (s[1] * 1 + s[2] * 1 + 1) >> 1;
        uint8 a2 = (s[2] * 1 + s[3] * 3 + 2) >> 2;
        uint8 b0 = (t[0] * 3 + t[1] * 1 + 2) >> 2;
        uint8 b1 = (t[1] * 1 + t[2] * 1 + 1) >> 1;
        uint8 b2 = (t[2] * 1 + t[3] * 3 + 2) >> 2;
        d[0] = (a0 * 3 + b0 + 2) >> 2;
        d[1] = (a1 * 3 + b1 + 2) >> 2;
        d[2] = (a2 * 3 + b2 + 2) >> 2;
        d += 3;
        s += 4;
        t += 4;
    }
}

// Filter rows 1 and 2 together, 1 : 1
static void ScaleRowDown34_1_Box_C(const uint8* src_ptr, int src_stride,
                                   uint8* d, int dst_width) {
    assert((dst_width % 3 == 0) && (dst_width > 0));
    const uint8* s = src_ptr;
    const uint8* t = src_ptr + src_stride;
    for (int x = 0; x < dst_width; x += 3) {
        uint8 a0 = (s[0] * 3 + s[1] * 1 + 2) >> 2;
        uint8 a1 = (s[1] * 1 + s[2] * 1 + 1) >> 1;
        uint8 a2 = (s[2] * 1 + s[3] * 3 + 2) >> 2;
        uint8 b0 = (t[0] * 3 + t[1] * 1 + 2) >> 2;
        uint8 b1 = (t[1] * 1 + t[2] * 1 + 1) >> 1;
        uint8 b2 = (t[2] * 1 + t[3] * 3 + 2) >> 2;
        d[0] = (a0 + b0 + 1) >> 1;
        d[1] = (a1 + b1 + 1) >> 1;
        d[2] = (a2 + b2 + 1) >> 1;
        d += 3;
        s += 4;
        t += 4;
    }
}


// CPU agnostic row functions
static void ScaleRowDown2_C(const uint8* src_ptr, int /* src_stride */,
                            uint8* dst, int dst_width) {
    for (int x = 0; x < dst_width - 1; x += 2) {
        dst[0] = src_ptr[1];
        dst[1] = src_ptr[3];
        dst += 2;
        src_ptr += 4;
    }
    if (dst_width & 1) {
        dst[0] = src_ptr[1];
    }
}

static void ScaleRowDown2Linear_C(const uint8* src_ptr, int src_stride,
                                  uint8* dst, int dst_width) {
    const uint8* s = src_ptr;
    for (int x = 0; x < dst_width - 1; x += 2) {
        dst[0] = (s[0] + s[1] + 1) >> 1;
        dst[1] = (s[2] + s[3] + 1) >> 1;
        dst += 2;
        s += 4;
    }
    if (dst_width & 1) {
        dst[0] = (s[0] + s[1] + 1) >> 1;
    }
}

static void ScaleRowDown2Box_C(const uint8* src_ptr, int src_stride,
                               uint8* dst, int dst_width) {
    const uint8* s = src_ptr;
    const uint8* t = src_ptr + src_stride;
    for (int x = 0; x < dst_width - 1; x += 2) {
        dst[0] = (s[0] + s[1] + t[0] + t[1] + 2) >> 2;
        dst[1] = (s[2] + s[3] + t[2] + t[3] + 2) >> 2;
        dst += 2;
        s += 4;
        t += 4;
    }
    if (dst_width & 1) {
        dst[0] = (s[0] + s[1] + t[0] + t[1] + 2) >> 2;
    }
}

static void ScaleRowDown38_C(const uint8* src_ptr, int /* src_stride */,
                             uint8* dst, int dst_width) {
    assert(dst_width % 3 == 0);
    for (int x = 0; x < dst_width; x += 3) {
        dst[0] = src_ptr[0];
        dst[1] = src_ptr[3];
        dst[2] = src_ptr[6];
        dst += 3;
        src_ptr += 8;
    }
}

// 8x3 -> 3x1
static void ScaleRowDown38_3_Box_C(const uint8* src_ptr,
                                   int src_stride,
                                   uint8* dst_ptr, int dst_width) {
    assert((dst_width % 3 == 0) && (dst_width > 0));
    intptr_t stride = src_stride;
    for (int i = 0; i < dst_width; i += 3) {
        dst_ptr[0] = (src_ptr[0] + src_ptr[1] + src_ptr[2] +
                      src_ptr[stride + 0] + src_ptr[stride + 1] +
                      src_ptr[stride + 2] + src_ptr[stride * 2 + 0] +
                      src_ptr[stride * 2 + 1] + src_ptr[stride * 2 + 2]) *
        (65536 / 9) >> 16;
        dst_ptr[1] = (src_ptr[3] + src_ptr[4] + src_ptr[5] +
                      src_ptr[stride + 3] + src_ptr[stride + 4] +
                      src_ptr[stride + 5] + src_ptr[stride * 2 + 3] +
                      src_ptr[stride * 2 + 4] + src_ptr[stride * 2 + 5]) *
        (65536 / 9) >> 16;
        dst_ptr[2] = (src_ptr[6] + src_ptr[7] +
                      src_ptr[stride + 6] + src_ptr[stride + 7] +
                      src_ptr[stride * 2 + 6] + src_ptr[stride * 2 + 7]) *
        (65536 / 6) >> 16;
        src_ptr += 8;
        dst_ptr += 3;
    }
}

// 8x2 -> 3x1
static void ScaleRowDown38_2_Box_C(const uint8* src_ptr, int src_stride,
                                   uint8* dst_ptr, int dst_width) {
    assert((dst_width % 3 == 0) && (dst_width > 0));
    intptr_t stride = src_stride;
    for (int i = 0; i < dst_width; i += 3) {
        dst_ptr[0] = (src_ptr[0] + src_ptr[1] + src_ptr[2] +
                      src_ptr[stride + 0] + src_ptr[stride + 1] +
                      src_ptr[stride + 2]) * (65536 / 6) >> 16;
        dst_ptr[1] = (src_ptr[3] + src_ptr[4] + src_ptr[5] +
                      src_ptr[stride + 3] + src_ptr[stride + 4] +
                      src_ptr[stride + 5]) * (65536 / 6) >> 16;
        dst_ptr[2] = (src_ptr[6] + src_ptr[7] +
                      src_ptr[stride + 6] + src_ptr[stride + 7]) *
        (65536 / 4) >> 16;
        src_ptr += 8;
        dst_ptr += 3;
    }
}


static void ScaleRowDown4Box_C(const uint8* src_ptr, int src_stride,
                               uint8* dst, int dst_width) {
    intptr_t stride = src_stride;
    for (int x = 0; x < dst_width - 1; x += 2) {
        dst[0] = (src_ptr[0] + src_ptr[1] + src_ptr[2] + src_ptr[3] +
                  src_ptr[stride + 0] + src_ptr[stride + 1] +
                  src_ptr[stride + 2] + src_ptr[stride + 3] +
                  src_ptr[stride * 2 + 0] + src_ptr[stride * 2 + 1] +
                  src_ptr[stride * 2 + 2] + src_ptr[stride * 2 + 3] +
                  src_ptr[stride * 3 + 0] + src_ptr[stride * 3 + 1] +
                  src_ptr[stride * 3 + 2] + src_ptr[stride * 3 + 3] +
                  8) >> 4;
        dst[1] = (src_ptr[4] + src_ptr[5] + src_ptr[6] + src_ptr[7] +
                  src_ptr[stride + 4] + src_ptr[stride + 5] +
                  src_ptr[stride + 6] + src_ptr[stride + 7] +
                  src_ptr[stride * 2 + 4] + src_ptr[stride * 2 + 5] +
                  src_ptr[stride * 2 + 6] + src_ptr[stride * 2 + 7] +
                  src_ptr[stride * 3 + 4] + src_ptr[stride * 3 + 5] +
                  src_ptr[stride * 3 + 6] + src_ptr[stride * 3 + 7] +
                  8) >> 4;
        dst += 2;
        src_ptr += 8;
    }
    if (dst_width & 1) {
        dst[0] = (src_ptr[0] + src_ptr[1] + src_ptr[2] + src_ptr[3] +
                  src_ptr[stride + 0] + src_ptr[stride + 1] +
                  src_ptr[stride + 2] + src_ptr[stride + 3] +
                  src_ptr[stride * 2 + 0] + src_ptr[stride * 2 + 1] +
                  src_ptr[stride * 2 + 2] + src_ptr[stride * 2 + 3] +
                  src_ptr[stride * 3 + 0] + src_ptr[stride * 3 + 1] +
                  src_ptr[stride * 3 + 2] + src_ptr[stride * 3 + 3] +
                  8) >> 4;
    }
}

static void ScaleRowDown4_C(const uint8* src_ptr, int /* src_stride */,
                            uint8* dst, int dst_width) {
    for (int x = 0; x < dst_width - 1; x += 2) {
        dst[0] = src_ptr[2];
        dst[1] = src_ptr[6];
        dst += 2;
        src_ptr += 8;
    }
    if (dst_width & 1) {
        dst[0] = src_ptr[2];
    }
}

static __inline uint32 SumBox(int iboxwidth, int iboxheight,
                              int src_stride, const uint8* src_ptr) {
    assert(iboxwidth > 0);
    assert(iboxheight > 0);
    uint32 sum = 0u;
    for (int y = 0; y < iboxheight; ++y) {
        for (int x = 0; x < iboxwidth; ++x) {
            sum += src_ptr[x];
        }
        src_ptr += src_stride;
    }
    return sum;
}

static __inline uint32 SumPixels(int iboxwidth, const uint16* src_ptr) {
    assert(iboxwidth > 0);
    uint32 sum = 0u;
    for (int x = 0; x < iboxwidth; ++x) {
        sum += src_ptr[x];
    }
    return sum;
}

static void ScalePlaneBoxRow_C(int dst_width, int boxheight,
                               int x, int dx, int src_stride,
                               const uint8* src_ptr, uint8* dst_ptr) {
    for (int i = 0; i < dst_width; ++i) {
        int ix = x >> 16;
        x += dx;
        int boxwidth = (x >> 16) - ix;
        *dst_ptr++ = SumBox(boxwidth, boxheight, src_stride, src_ptr + ix) /
        (boxwidth * boxheight);
    }
}

static void ScaleAddCols2_C(int dst_width, int boxheight, int x, int dx,
                            const uint16* src_ptr, uint8* dst_ptr) {
    int scaletbl[2];
    int minboxwidth = (dx >> 16);
    scaletbl[0] = 65536 / (minboxwidth * boxheight);
    scaletbl[1] = 65536 / ((minboxwidth + 1) * boxheight);
    int* scaleptr = scaletbl - minboxwidth;
    for (int i = 0; i < dst_width; ++i) {
        int ix = x >> 16;
        x += dx;
        int boxwidth = (x >> 16) - ix;
        *dst_ptr++ = SumPixels(boxwidth, src_ptr + ix) * scaleptr[boxwidth] >> 16;
    }
}

static void ScaleAddCols1_C(int dst_width, int boxheight, int x, int dx,
                            const uint16* src_ptr, uint8* dst_ptr) {
    int boxwidth = (dx >> 16);
    int scaleval = 65536 / (boxwidth * boxheight);
    for (int i = 0; i < dst_width; ++i) {
        *dst_ptr++ = SumPixels(boxwidth, src_ptr + x) * scaleval >> 16;
        x += boxwidth;
    }
}

static void ScaleAddRows_C(const uint8* src_ptr, int src_stride,
                           uint16* dst_ptr, int src_width, int src_height) {
    assert(src_width > 0);
    assert(src_height > 0);
    for (int x = 0; x < src_width; ++x) {
        const uint8* s = src_ptr + x;
        unsigned int sum = 0u;
        for (int y = 0; y < src_height; ++y) {
            sum += s[0];
            s += src_stride;
        }
        // TODO(fbarchard): Consider limitting height to 256 to avoid overflow.
        dst_ptr[x] = sum < 65535u ? sum : 65535u;
    }
}


// (1-f)a + fb can be replaced with a + f(b-a)
#define BLENDER(a, b, f) (static_cast<int>(a) + \
((f) * (static_cast<int>(b) - static_cast<int>(a)) >> 16))

static void ScaleFilterCols_C(uint8* dst_ptr, const uint8* src_ptr,
                              int dst_width, int x, int dx) {
    for (int j = 0; j < dst_width - 1; j += 2) {
        int xi = x >> 16;
        int a = src_ptr[xi];
        int b = src_ptr[xi + 1];
        dst_ptr[0] = BLENDER(a, b, x & 0xffff);
        x += dx;
        xi = x >> 16;
        a = src_ptr[xi];
        b = src_ptr[xi + 1];
        dst_ptr[1] = BLENDER(a, b, x & 0xffff);
        x += dx;
        dst_ptr += 2;
    }
    if (dst_width & 1) {
        int xi = x >> 16;
        int a = src_ptr[xi];
        int b = src_ptr[xi + 1];
        dst_ptr[0] = BLENDER(a, b, x & 0xffff);
    }
}
#undef BLENDER


// Scales a single row of pixels using point sampling.
static void ScaleCols_C(uint8* dst_ptr, const uint8* src_ptr,
                        int dst_width, int x, int dx) {
    for (int j = 0; j < dst_width - 1; j += 2) {
        dst_ptr[0] = src_ptr[x >> 16];
        x += dx;
        dst_ptr[1] = src_ptr[x >> 16];
        x += dx;
        dst_ptr += 2;
    }
    if (dst_width & 1) {
        dst_ptr[0] = src_ptr[x >> 16];
    }
}

// Scales a single row of pixels up by 2x using point sampling.
static void ScaleColsUp2_C(uint8* dst_ptr, const uint8* src_ptr,
                           int dst_width, int, int) {
    for (int j = 0; j < dst_width - 1; j += 2) {
        dst_ptr[1] = dst_ptr[0] = src_ptr[0];
        src_ptr += 1;
        dst_ptr += 2;
    }
    if (dst_width & 1) {
        dst_ptr[0] = src_ptr[0];
    }
}


// Blend 2 rows into 1 for conversions such as I422ToI420.
static void HalfRow_C(const uint8* src_uv, int src_uv_stride,
                      uint8* dst_uv, int pix) {
    for (int x = 0; x < pix; ++x) {
        dst_uv[x] = (src_uv[x] + src_uv[src_uv_stride + x] + 1) >> 1;
    }
}
// C version 2x2 -> 2x1.
static void InterpolateRow_C(uint8* dst_ptr, const uint8* src_ptr,
                             int src_stride,
                             int width, int source_y_fraction) {
    if (source_y_fraction == 0) {
        memcpy(dst_ptr, src_ptr, width);
        return;
    }
    if (source_y_fraction == 128) {
        HalfRow_C(src_ptr, static_cast<int>(src_stride), dst_ptr, width);
        return;
    }
    int y1_fraction = source_y_fraction;
    int y0_fraction = 256 - y1_fraction;
    const uint8* src_ptr1 = src_ptr + src_stride;
    
    for (int x = 0; x < width - 1; x += 2) {
        dst_ptr[0] = (src_ptr[0] * y0_fraction + src_ptr1[0] * y1_fraction) >> 8;
        dst_ptr[1] = (src_ptr[1] * y0_fraction + src_ptr1[1] * y1_fraction) >> 8;
        src_ptr += 2;
        src_ptr1 += 2;
        dst_ptr += 2;
    }
    if (width & 1) {
        dst_ptr[0] = (src_ptr[0] * y0_fraction + src_ptr1[0] * y1_fraction) >> 8;
    }
}
// Scale plane vertically with bilinear interpolation.
static void ScalePlaneVertical(int src_height,
                               int dst_width, int dst_height,
                               int src_stride, int dst_stride,
                               const uint8* src_argb, uint8* dst_argb,
                               int x, int y, int dy,
                               int bpp, FilterMode filtering) {
    // TODO(fbarchard): Allow higher bpp.
    assert(bpp >= 1 && bpp <= 4);
    assert(src_height != 0);
    assert(dst_width > 0);
    assert(dst_height > 0);
    int dst_width_bytes = dst_width * bpp;
    src_argb += (x >> 16) * bpp;
    void (*InterpolateRow)(uint8* dst_argb, const uint8* src_argb,
                           int src_stride, int dst_width, int source_y_fraction) =
    InterpolateRow_C;
    const int max_y = (src_height > 1) ? ((src_height - 1) << 16) - 1 : 0;
    for (int j = 0; j < dst_height; ++j) {
        if (y > max_y) {
            y = max_y;
        }
        int yi = y >> 16;
        int yf = filtering ? ((y >> 8) & 255) : 0;
        const uint8* src = src_argb + yi * src_stride;
        InterpolateRow(dst_argb, src, src_stride, dst_width_bytes, yf);
        dst_argb += dst_stride;
        y += dy;
    }
}

// Scale plane down, 3/4
static void ScalePlaneDown34(int /* src_width */, int /* src_height */,
                             int dst_width, int dst_height,
                             int src_stride, int dst_stride,
                             const uint8* src_ptr, uint8* dst_ptr,
                             FilterMode filtering) {
    assert(dst_width % 3 == 0);
    void (*ScaleRowDown34_0)(const uint8* src_ptr, int src_stride,
                             uint8* dst_ptr, int dst_width);
    void (*ScaleRowDown34_1)(const uint8* src_ptr, int src_stride,
                             uint8* dst_ptr, int dst_width);
    if (!filtering) {
        ScaleRowDown34_0 = ScaleRowDown34_C;
        ScaleRowDown34_1 = ScaleRowDown34_C;
    } else {
        ScaleRowDown34_0 = ScaleRowDown34_0_Box_C;
        ScaleRowDown34_1 = ScaleRowDown34_1_Box_C;
    }
    
    const int filter_stride = (filtering == kFilterLinear) ? 0 : src_stride;
    for (int y = 0; y < dst_height - 2; y += 3) {
        ScaleRowDown34_0(src_ptr, filter_stride, dst_ptr, dst_width);
        src_ptr += src_stride;
        dst_ptr += dst_stride;
        ScaleRowDown34_1(src_ptr, filter_stride, dst_ptr, dst_width);
        src_ptr += src_stride;
        dst_ptr += dst_stride;
        ScaleRowDown34_0(src_ptr + src_stride, -filter_stride,
                         dst_ptr, dst_width);
        src_ptr += src_stride * 2;
        dst_ptr += dst_stride;
    }
    
    // Remainder 1 or 2 rows with last row vertically unfiltered
    if ((dst_height % 3) == 2) {
        ScaleRowDown34_0(src_ptr, filter_stride, dst_ptr, dst_width);
        src_ptr += src_stride;
        dst_ptr += dst_stride;
        ScaleRowDown34_1(src_ptr, 0, dst_ptr, dst_width);
    } else if ((dst_height % 3) == 1) {
        ScaleRowDown34_0(src_ptr, 0, dst_ptr, dst_width);
    }
}

// Scale plane, 1/2
// This is an optimized version for scaling down a plane to 1/2 of
// its original size.

static void ScalePlaneDown2(int /* src_width */, int /* src_height */,
                            int dst_width, int dst_height,
                            int src_stride, int dst_stride,
                            const uint8* src_ptr, uint8* dst_ptr,
                            FilterMode filtering) {
    void (*ScaleRowDown2)(const uint8* src_ptr, int src_stride,
                          uint8* dst_ptr, int dst_width) =
    filtering == kFilterNone ? ScaleRowDown2_C :
    (filtering == kFilterLinear ? ScaleRowDown2Linear_C :
     ScaleRowDown2Box_C);
    int row_stride = src_stride << 1;
    if (!filtering) {
        src_ptr += src_stride;  // Point to odd rows.
        src_stride = 0;
    }
    
    if (filtering == kFilterLinear) {
        src_stride = 0;
    }
    // TODO(fbarchard): Loop through source height to allow odd height.
    for (int y = 0; y < dst_height; ++y) {
        ScaleRowDown2(src_ptr, src_stride, dst_ptr, dst_width);
        src_ptr += row_stride;
        dst_ptr += dst_stride;
    }
}


// Scale plane, 3/8
// This is an optimized version for scaling down a plane to 3/8
// of its original size.
//
// Uses box filter arranges like this
// aaabbbcc -> abc
// aaabbbcc    def
// aaabbbcc    ghi
// dddeeeff
// dddeeeff
// dddeeeff
// ggghhhii
// ggghhhii
// Boxes are 3x3, 2x3, 3x2 and 2x2

static void ScalePlaneDown38(int /* src_width */, int /* src_height */,
                             int dst_width, int dst_height,
                             int src_stride, int dst_stride,
                             const uint8* src_ptr, uint8* dst_ptr,
                             FilterMode filtering) {
    assert(dst_width % 3 == 0);
    void (*ScaleRowDown38_3)(const uint8* src_ptr, int src_stride,
                             uint8* dst_ptr, int dst_width);
    void (*ScaleRowDown38_2)(const uint8* src_ptr, int src_stride,
                             uint8* dst_ptr, int dst_width);
    if (!filtering) {
        ScaleRowDown38_3 = ScaleRowDown38_C;
        ScaleRowDown38_2 = ScaleRowDown38_C;
    } else {
        ScaleRowDown38_3 = ScaleRowDown38_3_Box_C;
        ScaleRowDown38_2 = ScaleRowDown38_2_Box_C;
    }
    
    const int filter_stride = (filtering == kFilterLinear) ? 0 : src_stride;
    for (int y = 0; y < dst_height - 2; y += 3) {
        ScaleRowDown38_3(src_ptr, filter_stride, dst_ptr, dst_width);
        src_ptr += src_stride * 3;
        dst_ptr += dst_stride;
        ScaleRowDown38_3(src_ptr, filter_stride, dst_ptr, dst_width);
        src_ptr += src_stride * 3;
        dst_ptr += dst_stride;
        ScaleRowDown38_2(src_ptr, filter_stride, dst_ptr, dst_width);
        src_ptr += src_stride * 2;
        dst_ptr += dst_stride;
    }
    
    // Remainder 1 or 2 rows with last row vertically unfiltered
    if ((dst_height % 3) == 2) {
        ScaleRowDown38_3(src_ptr, filter_stride, dst_ptr, dst_width);
        src_ptr += src_stride * 3;
        dst_ptr += dst_stride;
        ScaleRowDown38_3(src_ptr, 0, dst_ptr, dst_width);
    } else if ((dst_height % 3) == 1) {
        ScaleRowDown38_3(src_ptr, 0, dst_ptr, dst_width);
    }
}


// Scale plane, 1/4
// This is an optimized version for scaling down a plane to 1/4 of
// its original size.

static void ScalePlaneDown4(int /* src_width */, int /* src_height */,
                            int dst_width, int dst_height,
                            int src_stride, int dst_stride,
                            const uint8* src_ptr, uint8* dst_ptr,
                            FilterMode filtering) {
    void (*ScaleRowDown4)(const uint8* src_ptr, int src_stride,
                          uint8* dst_ptr, int dst_width) =
    filtering ? ScaleRowDown4Box_C : ScaleRowDown4_C;
    int row_stride = src_stride << 2;
    if (!filtering) {
        src_ptr += src_stride * 2;  // Point to row 2.
        src_stride = 0;
    }
    
    if (filtering == kFilterLinear) {
        src_stride = 0;
    }
    for (int y = 0; y < dst_height; ++y) {
        ScaleRowDown4(src_ptr, src_stride, dst_ptr, dst_width);
        src_ptr += row_stride;
        dst_ptr += dst_stride;
    }
}


// Scale plane down to any dimensions, with interpolation.
// (boxfilter).
//
// Same method as SimpleScale, which is fixed point, outputting
// one pixel of destination using fixed point (16.16) to step
// through source, sampling a box of pixel with simple
// averaging.
static void ScalePlaneBox(int src_width, int src_height,
                          int dst_width, int dst_height,
                          int src_stride, int dst_stride,
                          const uint8* src_ptr, uint8* dst_ptr) {
    assert(dst_width > 0);
    assert(dst_height > 0);
    
    // Initial source x/y coordinate and step values as 16.16 fixed point.
    int x = 0;
    int y = 0;
    int dx = 0;
    int dy = 0;
    ScaleSlope(src_width, src_height, dst_width, dst_height, kFilterBox,
               &x, &y, &dx, &dy);
    const int max_y = (src_height << 16);
    // TODO(fbarchard): Remove this and make AddRows handle boxheight 1.
    if (!IS_ALIGNED(src_width, 16) || dst_height * 2 > src_height) {
        uint8* dst = dst_ptr;
        for (int j = 0; j < dst_height; ++j) {
            int iy = y >> 16;
            const uint8* src = src_ptr + iy * src_stride;
            y += dy;
            if (y > max_y) {
                y = max_y;
            }
            int boxheight = (y >> 16) - iy;
            ScalePlaneBoxRow_C(dst_width, boxheight,
                               x, dx, src_stride,
                               src, dst);
            dst += dst_stride;
        }
        return;
    }
    // Allocate a row buffer of uint16.
    align_buffer_64(row16, src_width * 2);
    
    void (*ScaleAddCols)(int dst_width, int boxheight, int x, int dx,
                         const uint16* src_ptr, uint8* dst_ptr) =
    (dx & 0xffff) ? ScaleAddCols2_C: ScaleAddCols1_C;
    void (*ScaleAddRows)(const uint8* src_ptr, int src_stride,
                         uint16* dst_ptr, int src_width, int src_height) = ScaleAddRows_C;
    
    for (int j = 0; j < dst_height; ++j) {
        int iy = y >> 16;
        const uint8* src = src_ptr + iy * src_stride;
        y += dy;
        if (y > (src_height << 16)) {
            y = (src_height << 16);
        }
        int boxheight = (y >> 16) - iy;
        ScaleAddRows(src, src_stride, reinterpret_cast<uint16*>(row16),
                     src_width, boxheight);
        ScaleAddCols(dst_width, boxheight, x, dx, reinterpret_cast<uint16*>(row16),
                     dst_ptr);
        dst_ptr += dst_stride;
    }
    free_aligned_buffer_64(row16);
}


// Scale up down with bilinear interpolation.
static void ScalePlaneBilinearUp(int src_width, int src_height,
                                 int dst_width, int dst_height,
                                 int src_stride, int dst_stride,
                                 const uint8* src_ptr, uint8* dst_ptr,
                                 FilterMode filtering) {
    assert(src_width != 0);
    assert(src_height != 0);
    assert(dst_width > 0);
    assert(dst_height > 0);
    
    // Initial source x/y coordinate and step values as 16.16 fixed point.
    int x = 0;
    int y = 0;
    int dx = 0;
    int dy = 0;
    ScaleSlope(src_width, src_height, dst_width, dst_height, filtering,
               &x, &y, &dx, &dy);
    
    void (*InterpolateRow)(uint8* dst_ptr, const uint8* src_ptr,
                           int src_stride, int dst_width, int source_y_fraction) =
    InterpolateRow_C;
    
    void (*ScaleFilterCols)(uint8* dst_ptr, const uint8* src_ptr,
                            int dst_width, int x, int dx) =
    filtering ? ScaleFilterCols_C : ScaleCols_C;
    
    if (!filtering && src_width * 2 == dst_width && x < 0x8000) {
        ScaleFilterCols = ScaleColsUp2_C;
        
    }
    
    const int max_y = (src_height - 1) << 16;
    if (y > max_y) {
        y = max_y;
    }
    int yi = y >> 16;
    const uint8* src = src_ptr + yi * src_stride;
    
    // Allocate 2 row buffers.
    const int kRowSize = (dst_width + 15) & ~15;
    align_buffer_64(row, kRowSize * 2);
    
    uint8* rowptr = row;
    int rowstride = kRowSize;
    int lasty = yi;
    
    ScaleFilterCols(rowptr, src, dst_width, x, dx);
    if (src_height > 1) {
        src += src_stride;
    }
    ScaleFilterCols(rowptr + rowstride, src, dst_width, x, dx);
    src += src_stride;
    
    for (int j = 0; j < dst_height; ++j) {
        yi = y >> 16;
        if (yi != lasty) {
            if (y > max_y) {
                y = max_y;
                yi = y >> 16;
                src = src_ptr + yi * src_stride;
            }
            if (yi != lasty) {
                ScaleFilterCols(rowptr, src, dst_width, x, dx);
                rowptr += rowstride;
                rowstride = -rowstride;
                lasty = yi;
                src += src_stride;
            }
        }
        if (filtering == kFilterLinear) {
            InterpolateRow(dst_ptr, rowptr, 0, dst_width, 0);
        } else {
            int yf = (y >> 8) & 255;
            InterpolateRow(dst_ptr, rowptr, rowstride, dst_width, yf);
        }
        dst_ptr += dst_stride;
        y += dy;
    }
    free_aligned_buffer_64(row);
}


// Scale plane down with bilinear interpolation.
static void ScalePlaneBilinearDown(int src_width, int src_height,
                                   int dst_width, int dst_height,
                                   int src_stride, int dst_stride,
                                   const uint8* src_ptr, uint8* dst_ptr,
                                   FilterMode filtering) {
    assert(dst_width > 0);
    assert(dst_height > 0);
    
    // Initial source x/y coordinate and step values as 16.16 fixed point.
    int x = 0;
    int y = 0;
    int dx = 0;
    int dy = 0;
    ScaleSlope(src_width, src_height, dst_width, dst_height, filtering,
               &x, &y, &dx, &dy);
    
    void (*InterpolateRow)(uint8* dst_ptr, const uint8* src_ptr,
                           int src_stride, int dst_width, int source_y_fraction) =
    InterpolateRow_C;
    
    void (*ScaleFilterCols)(uint8* dst_ptr, const uint8* src_ptr,
                            int dst_width, int x, int dx) = ScaleFilterCols_C;
    
    // TODO(fbarchard): Consider not allocating row buffer for kFilterLinear.
    // Allocate a row buffer.
    align_buffer_64(row, src_width);
    
    const int max_y = (src_height - 1) << 16;
    for (int j = 0; j < dst_height; ++j) {
        if (y > max_y) {
            y = max_y;
        }
        int yi = y >> 16;
        const uint8* src = src_ptr + yi * src_stride;
        if (filtering == kFilterLinear) {
            ScaleFilterCols(dst_ptr, src, dst_width, x, dx);
        } else {
            int yf = (y >> 8) & 255;
            InterpolateRow(row, src, src_stride, src_width, yf);
            ScaleFilterCols(dst_ptr, row, dst_width, x, dx);
        }
        dst_ptr += dst_stride;
        y += dy;
    }
    free_aligned_buffer_64(row);
}


// Scale Plane to/from any dimensions, without interpolation.
// Fixed point math is used for performance: The upper 16 bits
// of x and dx is the integer part of the source position and
// the lower 16 bits are the fixed decimal part.

static void ScalePlaneSimple(int src_width, int src_height,
                             int dst_width, int dst_height,
                             int src_stride, int dst_stride,
                             const uint8* src_ptr, uint8* dst_ptr) {
    // Initial source x/y coordinate and step values as 16.16 fixed point.
    int x = 0;
    int y = 0;
    int dx = 0;
    int dy = 0;
    ScaleSlope(src_width, src_height, dst_width, dst_height, kFilterNone,
               &x, &y, &dx, &dy);
    
    void (*ScaleCols)(uint8* dst_ptr, const uint8* src_ptr,
                      int dst_width, int x, int dx) = ScaleCols_C;
    if (src_width * 2 == dst_width && x < 0x8000) {
        ScaleCols = ScaleColsUp2_C;
    }
    
    for (int i = 0; i < dst_height; ++i) {
        ScaleCols(dst_ptr, src_ptr + (y >> 16) * src_stride,
                  dst_width, x, dx);
        dst_ptr += dst_stride;
        y += dy;
    }
}


void ScalePlane(const uint8* src, int src_stride,
                int src_width, int src_height,
                uint8* dst, int dst_stride,
                int dst_width, int dst_height,
                FilterMode filtering) {
    // Simplify filtering when possible.
    filtering = ScaleFilterReduce(src_width, src_height,
                                  dst_width, dst_height,
                                  filtering);
    
    // Negative height means invert the image.
    if (src_height < 0) {
        src_height = -src_height;
        src = src + (src_height - 1) * src_stride;
        src_stride = -src_stride;
    }
    
    // Use specialized scales to improve performance for common resolutions.
    // For example, all the 1/2 scalings will use ScalePlaneDown2()
    if (dst_width == src_width && dst_height == src_height) {
        // Straight copy.
        CopyPlane(src, src_stride, dst, dst_stride, dst_width, dst_height);
        return;
    }
    if (dst_width == src_width) {
        int dy = FixedDiv(src_height, dst_height);
        // Arbitrary scale vertically, but unscaled vertically.
        ScalePlaneVertical(src_height,
                           dst_width, dst_height,
                           src_stride, dst_stride, src, dst,
                           0, 0, dy, 1, filtering);
        return;
    }
    if (dst_width <= Abs(src_width) && dst_height <= src_height) {
        // Scale down.
        if (4 * dst_width == 3 * src_width &&
            4 * dst_height == 3 * src_height) {
            // optimized, 3/4
            ScalePlaneDown34(src_width, src_height, dst_width, dst_height,
                             src_stride, dst_stride, src, dst, filtering);
            return;
        }
        if (2 * dst_width == src_width && 2 * dst_height == src_height) {
            // optimized, 1/2
            ScalePlaneDown2(src_width, src_height, dst_width, dst_height,
                            src_stride, dst_stride, src, dst, filtering);
            return;
        }
        // 3/8 rounded up for odd sized chroma height.
        if (8 * dst_width == 3 * src_width &&
            dst_height == ((src_height * 3 + 7) / 8)) {
            // optimized, 3/8
            ScalePlaneDown38(src_width, src_height, dst_width, dst_height,
                             src_stride, dst_stride, src, dst, filtering);
            return;
        }
        if (4 * dst_width == src_width && 4 * dst_height == src_height &&
            filtering != kFilterBilinear) {
            // optimized, 1/4
            ScalePlaneDown4(src_width, src_height, dst_width, dst_height,
                            src_stride, dst_stride, src, dst, filtering);
            return;
        }
    }
    if (filtering == kFilterBox && dst_height * 2 < src_height) {
        ScalePlaneBox(src_width, src_height, dst_width, dst_height,
                      src_stride, dst_stride, src, dst);
        return;
    }
    if (filtering && dst_height > src_height) {
        ScalePlaneBilinearUp(src_width, src_height, dst_width, dst_height,
                             src_stride, dst_stride, src, dst, filtering);
        return;
    }
    if (filtering) {
        ScalePlaneBilinearDown(src_width, src_height, dst_width, dst_height,
                               src_stride, dst_stride, src, dst, filtering);
        return;
    }
    ScalePlaneSimple(src_width, src_height, dst_width, dst_height,
                     src_stride, dst_stride, src, dst);
}

int I420Scale(const uint8* src_y, int src_stride_y,
              const uint8* src_u, int src_stride_u,
              const uint8* src_v, int src_stride_v,
              int src_width, int src_height,
              uint8* dst_y, int dst_stride_y,
              uint8* dst_u, int dst_stride_u,
              uint8* dst_v, int dst_stride_v,
              int dst_width, int dst_height,
              FilterMode filtering) {
    if (!src_y || !src_u || !src_v || src_width == 0 || src_height == 0 ||
        !dst_y || !dst_u || !dst_v || dst_width <= 0 || dst_height <= 0) {
        return -1;
    }
    int src_halfwidth = SUBSAMPLE(src_width, 1, 1);
    int src_halfheight = SUBSAMPLE(src_height, 1, 1);
    int dst_halfwidth = SUBSAMPLE(dst_width, 1, 1);
    int dst_halfheight = SUBSAMPLE(dst_height, 1, 1);
    
    ScalePlane(src_y, src_stride_y, src_width, src_height,
               dst_y, dst_stride_y, dst_width, dst_height,
               filtering);
    ScalePlane(src_u, src_stride_u, src_halfwidth, src_halfheight,
               dst_u, dst_stride_u, dst_halfwidth, dst_halfheight,
               filtering);
    ScalePlane(src_v, src_stride_v, src_halfwidth, src_halfheight,
               dst_v, dst_stride_v, dst_halfwidth, dst_halfheight,
               filtering);
    return 0;
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

static int I422ToYUY2(const uint8* src_y, int src_stride_y,
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
    
    for (int y = 0; y < height; ++y) {
        I422ToYUY2Row(src_y, src_u, src_v, dst_yuy2, width);
        src_y += src_stride_y;
        src_u += src_stride_u;
        src_v += src_stride_v;
        dst_yuy2 += dst_stride_yuy2;
    }
    return 0;
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
static int YUY2ToI422(const uint8* src_yuy2, int src_stride_yuy2,
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

int YUY2Scale(uint8* src_yuy2, int src_stride_yuy2,
              int src_width,int src_height,	
              uint8* dst_yuy2,int dst_stride_yuy2,
              int dst_width,int dst_height,FilterMode filterMode)
{
    uint8* src_y,*src_u,*src_v,*dst_y,*dst_u,*dst_v;
    src_y = (uint8*)malloc(src_width*src_height);
    src_u = (uint8*)malloc((src_width*src_height+1)>>1);
    src_v = (uint8*)malloc((src_width*src_height+1)>>1);
    
    
    if(src_y == NULL || src_u == NULL || src_v == NULL)
    {
        SAFE_FREE(src_y);
        SAFE_FREE(src_u);
        SAFE_FREE(src_v);
        return -1;
    }
    
    YUY2ToI422(src_yuy2,src_stride_yuy2,src_y,src_width,src_u,src_width/2,src_v,src_width/2,src_width,src_height);
    
    dst_y = (uint8*)malloc(((dst_width*dst_height+1)));
    ScalePlane(src_y,src_width,src_width,src_height,dst_y,dst_width,dst_width,dst_height,filterMode);
    
    free(src_y);
    dst_u = (uint8*)malloc(((dst_width*dst_height+1)>>1));
    
    if(dst_u == NULL)
    {
        free(src_u);
        free(src_v);
        return -1;
    }
    
    ScalePlane(src_u,src_width/2,src_width/2,src_height,dst_u,dst_width/2,dst_width/2,dst_height,filterMode);
    
    free(src_u);
    
    dst_v = (uint8*)malloc(((dst_width*dst_height+1)>>1));
    if(dst_v == NULL)
    {
        free(src_v);
        return -1;
    }
    
    ScalePlane(src_v,src_width/2,src_width/2,src_height,dst_v,dst_width/2,dst_width/2,dst_height,filterMode);
    
    free(src_v);
    
    I422ToYUY2(dst_y,dst_width,dst_u,dst_width/2,dst_v,dst_width/2,dst_yuy2,dst_stride_yuy2,dst_width,dst_height);
    
    free(dst_y);
    free(dst_u);
    free(dst_v);
    return 0;
}