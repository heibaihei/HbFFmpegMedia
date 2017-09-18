#include "FormatCvt_row_define.h"
#include "FormatCvt_row.h"
#include "FormatCvt.h"

#include <string>

//---------------------------------------------------------------------------------
int FormatCvt::J420ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height) {
  int y;
  void (*J422ToARGBRow)(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width) = J422ToARGBRow_C;
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

#if defined(HAS_J422TOARGBROW_NEON)&&defined(HAVE_NEON)
    J422ToARGBRow = J422ToARGBRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      J422ToARGBRow = J422ToARGBRow_NEON;
    }
#endif

  for (y = 0; y < height; ++y) {
    J422ToARGBRow(src_y, src_u, src_v, dst_argb, width);
    dst_argb += dst_stride_argb;
    src_y += src_stride_y;
    if (y & 1) {
      src_u += src_stride_u;
      src_v += src_stride_v;
    }
  }
  return 0;
}

int FormatCvt::J422ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height) {
  int y;
  void (*J422ToARGBRow)(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width) = J422ToARGBRow_C;
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
      src_stride_u * 2 == width &&
      src_stride_v * 2 == width &&
      dst_stride_argb == width * 4) {
    width *= height;
    height = 1;
    src_stride_y = src_stride_u = src_stride_v = dst_stride_argb = 0;
  }
 
#if defined(HAS_J422TOARGBROW_NEON)&&defined(HAVE_NEON)
    J422ToARGBRow = J422ToARGBRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      J422ToARGBRow = J422ToARGBRow_NEON;
    }
#endif
 

  for (y = 0; y < height; ++y) {
    J422ToARGBRow(src_y, src_u, src_v, dst_argb, width);
    dst_argb += dst_stride_argb;
    src_y += src_stride_y;
    src_u += src_stride_u;
    src_v += src_stride_v;
  }
  return 0;
}


int FormatCvt::ARGBToJ420(const uint8* src_argb, int src_stride_argb,
               uint8* dst_yj, int dst_stride_yj,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  int y;
  void (*ARGBToUVJRow)(const uint8* src_argb0, int src_stride_argb,
                       uint8* dst_u, uint8* dst_v, int width) = ARGBToUVJRow_C;
  void (*ARGBToYJRow)(const uint8* src_argb, uint8* dst_yj, int pix) =ARGBToYJRow_C;
  if (!src_argb ||
      !dst_yj || !dst_u || !dst_v ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_argb = src_argb + (height - 1) * src_stride_argb;
    src_stride_argb = -src_stride_argb;
  }

  #if defined(HAS_ARGBTOYJROW_NEON)&&defined(HAVE_NEON)
      ARGBToYJRow = ARGBToYJRow_Any_NEON;
      if (IS_ALIGNED(width, 8)) {
        ARGBToYJRow = ARGBToYJRow_NEON;
      }
  #endif
  #if defined(HAS_ARGBTOUVJROW_NEON)&&defined(HAVE_NEON)
      ARGBToUVJRow = ARGBToUVJRow_Any_NEON;
      if (IS_ALIGNED(width, 16)) {
        ARGBToUVJRow = ARGBToUVJRow_NEON;
      }
  #endif

  for (y = 0; y < height - 1; y += 2) {
    ARGBToUVJRow(src_argb, src_stride_argb, dst_u, dst_v, width);
    ARGBToYJRow(src_argb, dst_yj, width);
    ARGBToYJRow(src_argb + src_stride_argb, dst_yj + dst_stride_yj, width);
    src_argb += src_stride_argb * 2;
    dst_yj += dst_stride_yj * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    ARGBToUVJRow(src_argb, 0, dst_u, dst_v, width);
    ARGBToYJRow(src_argb, dst_yj, width);
  }
  return 0;
}



int FormatCvt::ARGBToJ422(const uint8* src_argb, int src_stride_argb,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  int y;
  void (*ARGBToUVJ422Row)(const uint8* src_argb, uint8* dst_u, uint8* dst_v,int pix) = ARGBToUVJ422Row_C;
  void (*ARGBToYJRow)(const uint8* src_argb, uint8* dst_y, int pix) =
      ARGBToYJRow_C;
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
      dst_stride_u * 2 == width &&
      dst_stride_v * 2 == width) {
    width *= height;
    height = 1;
    src_stride_argb = dst_stride_y = dst_stride_u = dst_stride_v = 0;
  }
 
  #if defined(HAS_ARGBTOYJROW_NEON)&&defined(HAVE_NEON)
      ARGBToYJRow = ARGBToYJRow_Any_NEON;
      if (IS_ALIGNED(width, 8)) {
        ARGBToYJRow = ARGBToYJRow_NEON;
      }
  #endif

  #if defined(HAS_ARGBTOUVJ422ROW_NEON)&&defined(HAVE_NEON)
      ARGBToUVJ422Row = ARGBToUVJ422Row_Any_NEON;
      if (IS_ALIGNED(width, 16)) {
        ARGBToUVJ422Row = ARGBToUVJ422Row_NEON;
      }
  #endif

  for (y = 0; y < height; ++y) {
    ARGBToUVJ422Row(src_argb, dst_u, dst_v, width);
    ARGBToYJRow(src_argb, dst_y, width);
    src_argb += src_stride_argb;
    dst_y += dst_stride_y;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  return 0;

}


 

     


//-------------------------------------I------------------------------------------
int FormatCvt::I420ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height) {
  int y;
  void (*I422ToARGBRow)(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width) = I422ToARGBRow_C;
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
  #if defined(HAS_I422TOARGBROW_NEON)&&defined(HAVE_NEON)
      I422ToARGBRow = I422ToARGBRow_Any_NEON;
      if (IS_ALIGNED(width, 8)) {
        I422ToARGBRow = I422ToARGBRow_NEON;
      }
  #endif
 

  for (y = 0; y < height; ++y) {
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

int FormatCvt::I422ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height) {
  int y;
  void (*I422ToARGBRow)(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width) = I422ToARGBRow_C;
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
      src_stride_u * 2 == width &&
      src_stride_v * 2 == width &&
      dst_stride_argb == width * 4) {
    width *= height;
    height = 1;
    src_stride_y = src_stride_u = src_stride_v = dst_stride_argb = 0;
  }
  #if defined(HAS_I422TOARGBROW_NEON)&&defined(HAVE_NEON)
      I422ToARGBRow = I422ToARGBRow_Any_NEON;
      if (IS_ALIGNED(width, 8)) {
        I422ToARGBRow = I422ToARGBRow_NEON;
      }
  #endif
 
  for (y = 0; y < height; ++y) {
    I422ToARGBRow(src_y, src_u, src_v, dst_argb, width);
    dst_argb += dst_stride_argb;
    src_y += src_stride_y;
    src_u += src_stride_u;
    src_v += src_stride_v;
  }
  return 0;
}

int FormatCvt::ARGBToI422(const uint8* src_argb, int src_stride_argb,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  int y;
  void (*ARGBToUV422Row)(const uint8* src_argb, uint8* dst_u, uint8* dst_v,
      int pix) = ARGBToUV422Row_C;
  void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix) =
      ARGBToYRow_C;
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
      dst_stride_u * 2 == width &&
      dst_stride_v * 2 == width) {
    width *= height;
    height = 1;
    src_stride_argb = dst_stride_y = dst_stride_u = dst_stride_v = 0;
  }
#if defined(HAS_ARGBTOUV422ROW_NEON)&&defined(HAVE_NEON)
    ARGBToUV422Row = ARGBToUV422Row_Any_NEON;
    if (IS_ALIGNED(width, 16)) {
      ARGBToUV422Row = ARGBToUV422Row_NEON;
    }
#endif
#if defined(HAS_ARGBTOYROW_NEON)&&defined(HAVE_NEON)
    ARGBToYRow = ARGBToYRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      ARGBToYRow = ARGBToYRow_NEON;
    }
#endif

  for (y = 0; y < height; ++y) {
    ARGBToUV422Row(src_argb, dst_u, dst_v, width);
    ARGBToYRow(src_argb, dst_y, width);
    src_argb += src_stride_argb;
    dst_y += dst_stride_y;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  return 0;
}



 int FormatCvt::ARGBToI420(const uint8* src_argb, int src_stride_argb,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  int y;
  void (*ARGBToUVRow)(const uint8* src_argb0, int src_stride_argb,
      uint8* dst_u, uint8* dst_v, int width) = ARGBToUVRow_C;
  void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix) =
      ARGBToYRow_C;
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
#if defined(HAS_ARGBTOYROW_NEON)&&defined(HAVE_NEON)
    ARGBToYRow = ARGBToYRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      ARGBToYRow = ARGBToYRow_NEON;
    }
#endif
#if defined(HAS_ARGBTOUVROW_NEON)&&defined(HAVE_NEON)
    ARGBToUVRow = ARGBToUVRow_Any_NEON;
    if (IS_ALIGNED(width, 16)) {
      ARGBToUVRow = ARGBToUVRow_NEON;
    }
#endif

  for (y = 0; y < height - 1; y += 2) {
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



  


//-------------------------------------------------------------------------

int FormatCvt::ARGBToNV12(const uint8* src_argb, int src_stride_argb,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_uv, int dst_stride_uv,
               int width, int height) {
  int y;
  int halfwidth = (width + 1) >> 1;
  void (*ARGBToUVRow)(const uint8* src_argb0, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int width) = ARGBToUVRow_C;
  void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix) =
      ARGBToYRow_C;
  void (*MergeUVRow_)(const uint8* src_u, const uint8* src_v, uint8* dst_uv,
                      int width) = MergeUVRow_C;
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

#if defined(HAS_ARGBTOYROW_NEON)&&defined(HAVE_NEON)
    ARGBToYRow = ARGBToYRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      ARGBToYRow = ARGBToYRow_NEON;
    }
#endif
#if defined(HAS_ARGBTOUVROW_NEON)&&defined(HAVE_NEON)
    ARGBToUVRow = ARGBToUVRow_Any_NEON;
    if (IS_ALIGNED(width, 16)) {
      ARGBToUVRow = ARGBToUVRow_NEON;
    }
#endif

#if defined(HAS_MERGEUVROW_NEON)&&defined(HAVE_NEON)
    MergeUVRow_ = MergeUVRow_Any_NEON;
    if (IS_ALIGNED(halfwidth, 16)) {
      MergeUVRow_ = MergeUVRow_NEON;
    }
#endif
  {
    // Allocate a rows of uv.
    align_buffer_64(row_u, ((halfwidth + 31) & ~31) * 2);
    uint8* row_v = row_u + ((halfwidth + 31) & ~31);

    for (y = 0; y < height - 1; y += 2) {
      ARGBToUVRow(src_argb, src_stride_argb, row_u, row_v, width);
      MergeUVRow_(row_u, row_v, dst_uv, halfwidth);
      ARGBToYRow(src_argb, dst_y, width);
      ARGBToYRow(src_argb + src_stride_argb, dst_y + dst_stride_y, width);
      src_argb += src_stride_argb * 2;
      dst_y += dst_stride_y * 2;
      dst_uv += dst_stride_uv;
    }
    if (height & 1) {
      ARGBToUVRow(src_argb, 0, row_u, row_v, width);
      MergeUVRow_(row_u, row_v, dst_uv, halfwidth);
      ARGBToYRow(src_argb, dst_y, width);
    }
    free_aligned_buffer_64(row_u);
  }
  return 0;
}



 

int FormatCvt::NV12ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_uv, int src_stride_uv,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height) {
  int y;
  void (*NV12ToARGBRow)(const uint8* y_buf,
                        const uint8* uv_buf,
                        uint8* rgb_buf,
                        int width) = NV12ToARGBRow_C;
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

#if defined(HAS_NV12TOARGBROW_NEON)&&defined(HAVE_NEON)
    NV12ToARGBRow = NV12ToARGBRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      NV12ToARGBRow = NV12ToARGBRow_NEON;
    }
#endif

  for (y = 0; y < height; ++y) {
    NV12ToARGBRow(src_y, src_uv, dst_argb, width);
    dst_argb += dst_stride_argb;
    src_y += src_stride_y;
    if (y & 1) {
      src_uv += src_stride_uv;
    }
  }
  return 0;
}



int FormatCvt::NV21ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_uv, int src_stride_uv,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height) {
  int y;
  void (*NV21ToARGBRow)(const uint8* y_buf,
                        const uint8* uv_buf,
                        uint8* rgb_buf,
                        int width) = NV21ToARGBRow_C;
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
#if defined(HAS_NV21TOARGBROW_NEON)&&defined(HAVE_NEON)
    NV21ToARGBRow = NV21ToARGBRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      NV21ToARGBRow = NV21ToARGBRow_NEON;
    }
  
#endif

  for (y = 0; y < height; ++y) {
    NV21ToARGBRow(src_y, src_uv, dst_argb, width);
    dst_argb += dst_stride_argb;
    src_y += src_stride_y;
    if (y & 1) {
      src_uv += src_stride_uv;
    }
  }
  return 0;
}

int FormatCvt::ARGBToNV21(const uint8* src_argb, int src_stride_argb,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_uv, int dst_stride_uv,
               int width, int height) {
  int y;
  int halfwidth = (width + 1) >> 1;
  void (*ARGBToUVRow)(const uint8* src_argb0, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int width) = ARGBToUVRow_C;
  void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix) =
      ARGBToYRow_C;
  void (*MergeUVRow_)(const uint8* src_u, const uint8* src_v, uint8* dst_uv,
                      int width) = MergeUVRow_C;
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
#if defined(HAS_ARGBTOYROW_NEON)&&defined(HAVE_NEON)

    ARGBToYRow = ARGBToYRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      ARGBToYRow = ARGBToYRow_NEON;
    }
  
#endif
#if defined(HAS_ARGBTOUVROW_NEON)&&defined(HAVE_NEON)
    ARGBToUVRow = ARGBToUVRow_Any_NEON;
    if (IS_ALIGNED(width, 16)) {
      ARGBToUVRow = ARGBToUVRow_NEON;
    }
#endif
#if defined(HAS_MERGEUVROW_NEON)&&defined(HAVE_NEON)
    MergeUVRow_ = MergeUVRow_Any_NEON;
    if (IS_ALIGNED(halfwidth, 16)) {
      MergeUVRow_ = MergeUVRow_NEON;
    }
#endif
  {
    // Allocate a rows of uv.
    align_buffer_64(row_u, ((halfwidth + 31) & ~31) * 2);
    uint8* row_v = row_u + ((halfwidth + 31) & ~31);

    for (y = 0; y < height - 1; y += 2) {
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
  }
  return 0;
}

//-------------------------------------------------------------------------------
int FormatCvt::ARGBToYUY2J(const uint8* src_argb, int src_stride_argb,
               uint8* dst_yuy2, int dst_stride_yuy2,
               int width, int height) {
  int y;
  void (*ARGBToUVJ422Row)(const uint8* src_argb, uint8* dst_u, uint8* dst_v,
      int pix) = ARGBToUVJ422Row_C;
  void (*ARGBToYJRow)(const uint8* src_argb, uint8* dst_y, int pix) =
      ARGBToYJRow_C;
  void (*I422ToYUY2JRow)(const uint8* src_y, const uint8* src_u,
      const uint8* src_v, uint8* dst_yuy2, int width) = I422ToYUY2JRow_C;

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
#if defined(HAS_ARGBTOUV422ROW_NEON)&&defined(HAVE_NEON)
    ARGBToUVJ422Row = ARGBToUVJ422Row_Any_NEON;
    if (IS_ALIGNED(width, 16)) {
      ARGBToUVJ422Row = ARGBToUVJ422Row_NEON;
    }
#endif
#if defined(HAS_ARGBTOYROW_NEON)&&defined(HAVE_NEON)
    ARGBToYJRow = ARGBToYJRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      ARGBToYJRow = ARGBToYJRow_NEON;
    }
#endif

#if defined(HAS_I422TOYUY2JROW_NEON)&&defined(HAVE_NEON)
    I422ToYUY2JRow = I422ToYUY2JRow_Any_NEON;
    if (IS_ALIGNED(width, 16)) {
      I422ToYUY2JRow = I422ToYUY2JRow_NEON;
    }
#endif

  {
    // Allocate a rows of yuv.
    align_buffer_64(row_y, ((width + 63) & ~63) * 2);
    uint8* row_u = row_y + ((width + 63) & ~63);
    uint8* row_v = row_u + ((width + 63) & ~63) / 2;

    for (y = 0; y < height; ++y) {
      ARGBToUVJ422Row(src_argb, row_u, row_v, width);
      ARGBToYJRow(src_argb, row_y, width);
      I422ToYUY2JRow(row_y, row_u, row_v, dst_yuy2, width);
      src_argb += src_stride_argb;
      dst_yuy2 += dst_stride_yuy2;
    }

    free_aligned_buffer_64(row_y);
  }
  return 0;
}

int FormatCvt::YUY2JToARGB(const uint8* src_yuy2, int src_stride_yuy2,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height) {
  int y;
  void (*YUY2ToARGBRow)(const uint8* src_yuy2, uint8* dst_argb, int pix) =
      YUY2JToARGBRow_C;
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
 
#if defined(HAS_YUY2JTOARGBROW_NEON)&&defined(HAVE_NEON)
 
    YUY2ToARGBRow = YUY2JToARGBRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      YUY2ToARGBRow = YUY2JToARGBRow_NEON;
    }
 
#endif
  for (y = 0; y < height; ++y) {
    YUY2ToARGBRow(src_yuy2, dst_argb, width);
    src_yuy2 += src_stride_yuy2;
    dst_argb += dst_stride_argb;
  }
  return 0;
}

int FormatCvt::YUVFullToYUY2( const uint8* src_yuv444, int src_stride_yuv444, uint8* dst_yuy2, int dst_stride_yuy2, int width, int height )
{
	int y;
	void (*YUV444ToYUY2Row)(const uint8* src_yuv444,uint8* dst_yuy2, int pix) = YUV444ToYUY2Row_C;

	// Negative height means invert the image.
	if (height < 0) {
		height = -height;
		dst_yuy2 = dst_yuy2 + (height - 1) * dst_stride_yuy2;
		dst_stride_yuy2 = -dst_stride_yuy2;
	}

#if defined(HAVE_NEON) 
	YUV444ToYUY2Row = YUV444ToYUY2Row_Any_NEON;
	if (IS_ALIGNED(width, 16)) {
		YUV444ToYUY2Row = YUV444ToYUY2Row_NEON;
	}
#endif


	for (y = 0; y < height ; y ++) {
		YUV444ToYUY2Row(src_yuv444,dst_yuy2,width);
		dst_yuy2 += dst_stride_yuy2;
		src_yuv444+=src_stride_yuv444;
	}
	return 0;
}

int FormatCvt::YUY2ToYUVFull( const uint8* src_yuy2, int src_stride_yuy2, uint8* dst_yuv444, int dst_stride_yuv444, int width, int height )
{
	int y;
	void (*YUY2ToYUV444Row)(const uint8* src_yuy2,uint8* dst_yuv444, int pix) = YUY2ToYUV444Row_C;

	// Negative height means invert the image.
	if (height < 0) {
		height = -height;
		src_yuy2 = src_yuy2 + (height - 1) * src_stride_yuy2;
		src_stride_yuy2 = -src_stride_yuy2;
	}

#if defined(HAVE_NEON)
	YUY2ToYUV444Row = YUY2ToYUV444Row_Any_NEON;
	if (IS_ALIGNED(width, 16)) {
		YUY2ToYUV444Row = YUY2ToYUV444Row_NEON;
	}
#endif


	for (y = 0; y < height ; y ++) {
		YUY2ToYUV444Row(src_yuy2, dst_yuv444,width);
		src_yuy2 += src_stride_yuy2;
		dst_yuv444+=dst_stride_yuv444;
	}
	return 0;
}

int FormatCvt::I420ToNV12( const uint8* src_y,int src_stride_y,const uint8* src_u,int src_stride_u, const uint8* src_v,int src_stride_v,uint8* dst_y,int dst_stride_y, uint8* dst_uv,int dst_stride_uv,int width,int height )
{
	if(src_y == NULL || src_u == NULL || src_v == NULL || dst_uv == NULL || dst_y == NULL)
	{
		return -1;
	}

	if((width&1) || (height&1))
	{
		return -1;
	}
	int halfWidth = width >> 1;
	for(int i = 0 ; i < height ; i ++)
	{
		memcpy(dst_y,src_y,sizeof(uint8)*width);
		//uv
		if(i&1)
		{
			for(int j = 0 ; j < halfWidth ; j ++)
			{
				dst_uv[(j<<1)] = src_u[j];
				dst_uv[(j<<1)|1] = src_v[j];
			}
			dst_uv += dst_stride_uv;
			src_u += src_stride_u;
			src_v += src_stride_v;
		}
		
		src_y += src_stride_y;
		dst_y += dst_stride_y;
	}

	return 0;
}

int FormatCvt::I420ToNV21(const uint8* src_y,int src_stride_y,const uint8* src_u,int src_stride_u,
	const uint8* src_v,int src_stride_v,uint8* dst_y,int dst_stride_y,
	uint8* dst_uv,int dst_stride_uv,int width,int height)
{
	if(src_y == NULL || src_u == NULL || src_v == NULL || dst_uv == NULL || dst_y == NULL)
	{
		return -1;
	}

	if((width&1) || (height&1))
	{
		return -1;
	}
	int halfWidth = width >> 1;
	for(int i = 0 ; i < height ; i ++)
	{
		memcpy(dst_y,src_y,sizeof(uint8)*width);
		//uv
		if(i&1)
		{
			for(int j = 0 ; j < halfWidth ; j ++)
			{
				dst_uv[(j<<1)] = src_v[j];
				dst_uv[(j<<1)|1] = src_u[j];
			}
			dst_uv += dst_stride_uv;
			src_u += src_stride_u;
			src_v += src_stride_v;
		}

		src_y += src_stride_y;
		dst_y += dst_stride_y;
	}

	return 0;
}
