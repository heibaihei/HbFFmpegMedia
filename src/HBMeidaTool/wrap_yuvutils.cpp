#if 0
#include <jni.h>
#include <cstdlib>
#include "FormatCvt/FormatCvt.h"
#include "LogHelper.h"

extern "C" {


JNIEXPORT jboolean JNICALL
Java_com_meitu_utils_colors_YUVUtils_ARGB2NV21(JNIEnv *env, jclass type, jobject argb, jobject dst,
                                               jint width, jint height) {
    if (argb == NULL || dst == NULL) {
        LOGE("Input buffer or out put buffer must not be NULL!");
        return JNI_FALSE;
    }

    if (width == 0 || height == 0) {
        LOGE("Input argb data width %d, height %d", width, height);
        return JNI_FALSE;
    }

    jlong argb_size = env->GetDirectBufferCapacity(argb);
    uint8_t* argb_data = (uint8_t*)env->GetDirectBufferAddress(argb);

    if (argb_size != width * height * 4) {
        LOGE("ARGB buffer size was not enough!");
        return JNI_FALSE;
    }

    jlong yuv_size = env->GetDirectBufferCapacity(dst);
    uint8_t *yuv_data = (uint8_t*)env->GetDirectBufferAddress(dst);

    if (yuv_size < width*height*3/2) {
        LOGE("Convert YUV dst buffer size was not enough!");
        return JNI_FALSE;
    }

    unsigned char* p420SP = yuv_data;
    unsigned char* pY = p420SP;
    unsigned char* pUV = p420SP + width*height;

    int ret = FormatCvt::ARGBToNV21(argb_data, width*4, pY, width, pUV, width, width, height);

    if (ret) {
        LOGE("ARGBToNV12 fail ret %d", ret);
        return JNI_FALSE;
    } else {
        return JNI_TRUE;
    }
}

JNIEXPORT jboolean JNICALL
Java_com_meitu_media_tools_utils_colors_YUVUtils_ARGB2NV12(JNIEnv *env, jclass type, jobject argb, jobject dst,
                                               jint width, jint height) {
    if (argb == NULL || dst == NULL) {
        LOGE("Input buffer or out put buffer must not be NULL!");
        return JNI_FALSE;
    }

    if (width == 0 || height == 0) {
        LOGE("Input argb data width %d, height %d", width, height);
        return JNI_FALSE;
    }

    jlong argb_size = env->GetDirectBufferCapacity(argb);
    uint8_t* argb_data = (uint8_t*)env->GetDirectBufferAddress(argb);

    if (argb_size != width * height * 4) {
        LOGE("ARGB buffer size was not enough!");
        return JNI_FALSE;
    }

    jlong yuv_size = env->GetDirectBufferCapacity(dst);
    uint8_t *yuv_data = (uint8_t*)env->GetDirectBufferAddress(dst);

    if (yuv_size < width*height*3/2) {
        LOGE("Convert YUV dst buffer size was not enough!");
        return JNI_FALSE;
    }

    unsigned char* p420SP = yuv_data;
    unsigned char* pY = p420SP;
    unsigned char* pUV = p420SP + width*height;

    int ret = FormatCvt::ARGBToNV12(argb_data, width*4, pY, width, pUV, width, width, height);

    if (ret) {
        LOGE("ARGBToNV12 fail ret %d", ret);
        return JNI_FALSE;
    } else {
        return JNI_TRUE;
    }
}

JNIEXPORT jboolean JNICALL
Java_com_meitu_media_tools_utils_colors_YUVUtils_NV12ToARGB(JNIEnv *env, jclass type, jobject nv12,
                                                jobject argb, jint width, jint height) {

    if (argb == NULL || nv12 == NULL) {
        LOGE("Input buffer or out put buffer must not be NULL!");
        return JNI_FALSE;
    }

    if (width == 0 || height == 0) {
        LOGE("Input argb data width %d, height %d", width, height);
        return JNI_FALSE;
    }

    jlong argb_size = env->GetDirectBufferCapacity(argb);
    uint8_t* argb_data = (uint8_t*)env->GetDirectBufferAddress(argb);

    if (argb_size != width * height * 4) {
        LOGE("ARGB buffer size was not enough!");
        return JNI_FALSE;
    }

    jlong yuv_size = env->GetDirectBufferCapacity(nv12);
    uint8_t *yuv_data = (uint8_t*)env->GetDirectBufferAddress(nv12);

    if (yuv_size < width*height*3/2) {
        LOGE("Convert YUV dst buffer size was not enough!");
        return JNI_FALSE;
    }

    unsigned char* p420SP = yuv_data;
    unsigned char* pY = p420SP;
    unsigned char* pUV = p420SP + width*height;

    int ret = FormatCvt::NV12ToARGB(pY, width, pUV, width, argb_data, width*4, width, height);

    if (ret) {
        LOGE("NV12ToARGB fail ret %d", ret);
        return JNI_FALSE;
    } else {
        return JNI_TRUE;
    }
}

JNIEXPORT jboolean JNICALL
Java_com_meitu_media_tools_utils_colors_YUVUtils_NV21ToARGB(JNIEnv *env, jclass type, jobject nv12,
                                                jobject argb, jint width, jint height) {

    if (argb == NULL || nv12 == NULL) {
        LOGE("Input buffer or out put buffer must not be NULL!");
        return JNI_FALSE;
    }

    if (width == 0 || height == 0) {
        LOGE("Input argb data width %d, height %d", width, height);
        return JNI_FALSE;
    }

    jlong argb_size = env->GetDirectBufferCapacity(argb);
    uint8_t* argb_data = (uint8_t*)env->GetDirectBufferAddress(argb);

    if (argb_size != width * height * 4) {
        LOGE("ARGB buffer size was not enough!");
        return JNI_FALSE;
    }

    jlong yuv_size = env->GetDirectBufferCapacity(nv12);
    uint8_t *yuv_data = (uint8_t*)env->GetDirectBufferAddress(nv12);

    if (yuv_size < width*height*3/2) {
        LOGE("Convert YUV dst buffer size was not enough!");
        return JNI_FALSE;
    }

    unsigned char* p420SP = yuv_data;
    unsigned char* pY = p420SP;
    unsigned char* pUV = p420SP + width*height;

    int ret = FormatCvt::NV21ToARGB(pY, width, pUV, width, argb_data, width*4, width, height);

    if (ret) {
        LOGE("NV21ToARGB fail ret %d", ret);
        return JNI_FALSE;
    } else {
        return JNI_TRUE;
    }
}

JNIEXPORT jboolean JNICALL
Java_com_meitu_media_tools_utils_colors_YUVUtils_ARGB2I420(JNIEnv *env, jclass type, jobject argb, jobject dst,
                                               jint width, jint height) {
    if (argb == NULL || dst == NULL) {
        LOGE("Input buffer or out put buffer must not be NULL!");
        return JNI_FALSE;
    }

    if (width == 0 || height == 0) {
        LOGE("Input argb data width %d, height %d", width, height);
        return JNI_FALSE;
    }

    jlong argb_size = env->GetDirectBufferCapacity(argb);
    uint8_t* argb_data = (uint8_t*)env->GetDirectBufferAddress(argb);

    if (argb_size != width * height * 4) {
        LOGE("ARGB buffer size was not enough!");
        return JNI_FALSE;
    }

    jlong yuv_size = env->GetDirectBufferCapacity(dst);
    uint8_t *yuv_data = (uint8_t*)env->GetDirectBufferAddress(dst);

    if (yuv_size < width*height*3/2) {
        LOGE("Convert YUV dst buffer size was not enough!");
        return JNI_FALSE;
    }

    unsigned char* pI420 = yuv_data;
    unsigned char* pY = pI420;
    unsigned char* pU = pI420 + width*height;
    unsigned char* pV = pU + width*height/4;

    int ret = FormatCvt::ARGBToI420(argb_data, width*4,
                                    pY, width,
                                    pU, width/2,
                                    pV, width/2,
                                    width, height);

    if (ret) {
        LOGE("ARGBToI420 fail ret %d", ret);
        return JNI_FALSE;
    } else {
        return JNI_TRUE;
    }
}

JNIEXPORT jboolean JNICALL
Java_com_meitu_media_tools_utils_colors_YUVUtils_I4202NV12(JNIEnv *env, jclass type, jobject i420, jobject nv12,
                                               jint width, jint height) {

    if (i420 == NULL || nv12 == NULL) {
        LOGE("Input buffer or out put buffer must not be NULL!");
        return JNI_FALSE;
    }

    if (width == 0 || height == 0) {
        LOGE("Input argb data width %d, height %d", width, height);
        return JNI_FALSE;
    }


    jlong i420_size = env->GetDirectBufferCapacity(i420);
    uint8_t*i420_data = (uint8_t*)env->GetDirectBufferAddress(i420);

    if (i420_size < width * height * 3/2) {
        LOGE("i420 buffer size was not enough!");
        return JNI_FALSE;
    }

    jlong nv12_size = env->GetDirectBufferCapacity(nv12);
    uint8_t *nv12_data = (uint8_t*)env->GetDirectBufferAddress(nv12);

    if (nv12_size < width*height*3/2) {
        LOGE("Convert YUV dst buffer size was not enough!");
        return JNI_FALSE;
    }

    unsigned char* pI420 = i420_data;
    unsigned char* pY = pI420;
    unsigned char* pU = pI420 + width*height;
    unsigned char* pV = pU + width*height/4;

    unsigned char* p420SP = nv12_data;
    unsigned char* pNV12Y = p420SP;
    unsigned char* pNV12UV = p420SP + width*height;

    int ret = FormatCvt::I420ToNV12(
            pY, width,
            pU, width/2,
            pV, width/2,
            pNV12Y, width,
            pNV12UV, width,
            width, height);

    if (ret) {
        LOGE("I420ToNV12 fail ret %d", ret);
        return JNI_FALSE;
    } else {
        return JNI_TRUE;
    }
}

JNIEXPORT jboolean JNICALL
Java_com_meitu_media_tools_utils_colors_YUVUtils_I4202NV21(JNIEnv *env, jclass type, jobject i420, jobject nv21,
                                               jint width, jint height) {

    if (i420 == NULL || nv21 == NULL) {
        LOGE("Input buffer or out put buffer must not be NULL!");
        return JNI_FALSE;
    }

    if (width == 0 || height == 0) {
        LOGE("Input argb data width %d, height %d", width, height);
        return JNI_FALSE;
    }


    jlong i420_size = env->GetDirectBufferCapacity(i420);
    uint8_t*i420_data = (uint8_t*)env->GetDirectBufferAddress(i420);

    if (i420_size < width * height * 3/2) {
        LOGE("i420 buffer size was not enough!");
        return JNI_FALSE;
    }

    jlong nv21_size = env->GetDirectBufferCapacity(nv21);
    uint8_t *nv21_data = (uint8_t*)env->GetDirectBufferAddress(nv21);

    if (nv21_size < width*height*3/2) {
        LOGE("Convert YUV dst buffer size was not enough!");
        return JNI_FALSE;
    }

    unsigned char* pI420 = i420_data;
    unsigned char* pY = pI420;
    unsigned char* pU = pI420 + width*height;
    unsigned char* pV = pU + width*height/4;

    unsigned char* p420SP = nv21_data;
    unsigned char* pNV12Y = p420SP;
    unsigned char* pNV12UV = p420SP + width*height;

    int ret = FormatCvt::I420ToNV21(
            pY, width,
            pU, width/2,
            pV, width/2,
            pNV12Y, width,
            pNV12UV, width,
            width, height);

    if (ret) {
        LOGE("I420ToNV12 fail ret %d", ret);
        return JNI_FALSE;
    } else {
        return JNI_TRUE;
    }
}


JNIEXPORT jboolean JNICALL
Java_com_meitu_media_tools_utils_colors_YUVUtils_I420ToARGB(JNIEnv *env, jclass type, jobject i420,
                                                jobject argb, jint width, jint height) {

    if (argb == NULL || i420 == NULL) {
        LOGE("Input buffer or out put buffer must not be NULL!");
        return JNI_FALSE;
    }

    if (width == 0 || height == 0) {
        LOGE("Input argb data width %d, height %d", width, height);
        return JNI_FALSE;
    }

    jlong argb_size = env->GetDirectBufferCapacity(argb);
    uint8_t* argb_data = (uint8_t*)env->GetDirectBufferAddress(argb);

    if (argb_size != width * height * 4) {
        LOGE("ARGB buffer size was not enough!");
        return JNI_FALSE;
    }

    jlong yuv_size = env->GetDirectBufferCapacity(i420);
    uint8_t *yuv_data = (uint8_t*)env->GetDirectBufferAddress(i420);

    if (yuv_size < width*height*3/2) {
        LOGE("Convert YUV dst buffer size was not enough!");
        return JNI_FALSE;
    }

    unsigned char* pI420 = yuv_data;
    unsigned char* pY = pI420;
    unsigned char* pU = pI420 + width*height;
    unsigned char* pV = pU + width*height/4;

    int ret = FormatCvt::I420ToARGB(
            pY, width,
            pU, width/2,
            pV, width/2,
            argb_data, width*4,
            width, height);

    if (ret) {
        LOGE("I420ToARGB fail ret %d", ret);
        return JNI_FALSE;
    } else {
        return JNI_TRUE;
    }
}

}

#endif
