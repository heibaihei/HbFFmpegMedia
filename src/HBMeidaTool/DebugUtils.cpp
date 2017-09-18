//
//  DebugUtils.cpp
//  JNIMedia
//
//  Created by Javan on 15/7/1.
//  Copyright (c) 2015年 Javan. All rights reserved.
//

#include "DebugUtils.h"

#include <stdlib.h>

#include "convert_yuv.h"
#include "LogHelper.h"

// argb
typedef struct yuv2rgb_rgb_t {
    int r, g, b;
} yuv2rgb_rgb_t;

static inline void rgb_calc(yuv2rgb_rgb_t* rgb, int Y, int Cr, int Cb) {
    //    rgb->r = Y + Cr + (Cr >> 2) + (Cr >> 3) + (Cr >> 5);
    rgb->r = Y + Cb + (Cb >> 1) + (Cb >> 2) + (Cb >> 6);
    if (rgb->r < 0)
        rgb->r = 0;
    else if (rgb->r > 255)
        rgb->r = 255;
    rgb->g = Y - (Cb >> 2) + (Cb >> 4) + (Cb >> 5) - (Cr >> 1) + (Cr >> 3) + (Cr >> 4) + (Cr >> 5);
    if (rgb->g < 0)
        rgb->g = 0;
    else if (rgb->g > 255)
        rgb->g = 255;
    //    rgb->b = Y + Cb + (Cb >> 1) + (Cb >> 2) + (Cb >> 6);
    rgb->b = Y + Cr + (Cr >> 2) + (Cr >> 3) + (Cr >> 5);
    if (rgb->b < 0)
        rgb->b = 0;
    else if (rgb->b > 255)
        rgb->b = 255;
}

#define YUV2RGB_SET_RGB(p, rgb) *p++ = (unsigned char)rgb.r; *p++ = (unsigned char)rgb.g; *p++ = (unsigned char)rgb.b; *p++ = 0xff

static void yuv420sp_to_rgba(unsigned char const* yuv420sp, int width, int height, unsigned char* rgba) {
    const int width4 = width * 4;
    unsigned char const* y0_ptr = yuv420sp;
    unsigned char const* y1_ptr = yuv420sp + width;
    unsigned char const* cb_ptr = yuv420sp + (width * height);
    unsigned char const* cr_ptr = cb_ptr + 1;
    unsigned char* rgba0 = rgba;
    unsigned char* rgba1 = rgba + width4;
    int Y00, Y01, Y10, Y11;
    int Cr = 0;
    int Cb = 0;
    int r, c;
    yuv2rgb_rgb_t rgb00, rgb01, rgb10, rgb11;
    for (r = 0; r < height / 2; ++r) {
        for (c = 0; c < width / 2; ++c, cr_ptr += 2, cb_ptr += 2) {
            Cr = *cr_ptr;
            Cb = *cb_ptr;
            if (Cb < 0)
                Cb += 127;
            else
                Cb -= 128;
            if (Cr < 0)
                Cr += 127;
            else
                Cr -= 128;
            Y00 = *y0_ptr++;
            Y01 = *y0_ptr++;
            Y10 = *y1_ptr++;
            Y11 = *y1_ptr++;
            rgb_calc(&rgb00, Y00, Cr, Cb);
            rgb_calc(&rgb01, Y01, Cr, Cb);
            rgb_calc(&rgb10, Y10, Cr, Cb);
            rgb_calc(&rgb11, Y11, Cr, Cb);
            YUV2RGB_SET_RGB(rgba0, rgb00);
            YUV2RGB_SET_RGB(rgba0, rgb01);
            YUV2RGB_SET_RGB(rgba1, rgb10);
            YUV2RGB_SET_RGB(rgba1, rgb11);
        }
        y0_ptr += width;
        y1_ptr += width;
        rgba0 += width4;
        rgba1 += width4;
    }
}

int writePPMHeader(FILE *f, char magic, int w, int h, int color) {
    if (f==NULL) {
        printf("FILE error\n");
        exit(0);
    }
    
    if (magic=='A') {// ASCII
        fprintf(f, "P3\n");
    } else if (magic=='B') {
        fprintf(f, "P6\n");
    } else {
        printf("Magic can only be A(ASCII) or B(binary)\n");
        exit(0);
    }
    
    fprintf(f, "%d %d\n", w, h);
    fprintf(f, "%d\n", color);
    return 0;
}

int writePPMdataP6(FILE* f, unsigned char* rgba, int w, int h) {
    int i,j;
    for(i=0;i<w;i++) {
        for(j=0;j<h;j++) {
            fwrite(rgba, sizeof(unsigned char), 3, f);
            rgba+=4;
        }
    }
    return 0;
}

void writePPMFile(const char *dst, int *rgba, int w, int h) {
    FILE *f6 = fopen(dst, "w");
    if (f6==NULL) {
        printf("FILE error\n");
        return;
    }
    
    writePPMHeader(f6, 'B', w, h, 255);
    writePPMdataP6(f6, (unsigned char*)rgba, w, h);
    
    fclose(f6);
}

void DebugUtils::writeYUVPPMFile(const char *filename, unsigned char const*yuv, int w, int h) {
    int *rgba = NULL;
    rgba = new int[w*h];
    
    LOGE("begin to decodeYUV420SP %p", yuv);
    yuv420sp_to_rgba((unsigned char*)yuv, w, h, (unsigned char*)rgba);
    LOGE("end of decodeYUV420SP");
    LOGE("begin to writePPMFile");
    writePPMFile(filename, rgba, w, h);
    LOGE("end to writePPMFile");
    
    delete [] rgba;
}

void DebugUtils::writeI420PPMFile(const char *filename, const unsigned char *yuv, int w, int h) {
    int *argb = NULL;
    argb = new int[w*h];
    int numOfPixel = w * h;
    int positionOfV = numOfPixel;
    int positionOfU = numOfPixel/4 + numOfPixel;
    
    LOGE("begin to I420ToARGB %p,  %d, %d,  argb %p", yuv, w, h, argb);
    int ret = I420ToARGB(yuv, w, yuv + positionOfV, w/2, yuv + positionOfU, w/2, (uint8*)argb, w*4, w, h);
    if (ret<0) {
        LOGE("I420TOARGBfial !");
    }
    LOGE("end of I420ToARGB");
    LOGE("begin to writePPMFile");
    writePPMFile(filename, argb, w, h);
    LOGE("end to writePPMFile");
    
    delete [] argb;
}
