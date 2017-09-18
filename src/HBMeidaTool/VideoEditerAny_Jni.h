#if 0
#ifndef _VIDEO_EDITER_ANY_H_
#define _VIDEO_EDITER_ANY_H_
#include<jni.h>
//��������
#define JAVA_FUNC(name) Java_com_meitu_media_tools_editor_VideoEditerAny_##name

//����JAVA NATIVE �ӿ�
#ifdef __cplusplus
extern "C"{
#endif
	JNIEXPORT jboolean JNICALL
		JAVA_FUNC(nOpen)(JNIEnv* env,jobject obj,jstring videoFile);

	JNIEXPORT jdouble JNICALL
		JAVA_FUNC(nGetVideoDuration)(JNIEnv* env,jobject obj);

	JNIEXPORT jdouble JNICALL
		JAVA_FUNC(nGetProgressbarValue)(JNIEnv* env,jobject obj);

	JNIEXPORT jboolean JNICALL
		JAVA_FUNC(nClearProgressBarValue)(JNIEnv* env,jobject obj);

	JNIEXPORT jboolean JNICALL
		JAVA_FUNC(nSetImportMode)(JNIEnv* env,jobject obj,jint mode,jint size);

	JNIEXPORT jboolean JNICALL
		JAVA_FUNC(nSetWaterMark)(JNIEnv* env,jobject obj,jstring path,jint meipaiWatermarkType);

	JNIEXPORT jboolean JNICALL
		JAVA_FUNC(nSetEndingWaterMark)(JNIEnv* env,jobject obj,jstring jpath);

	JNIEXPORT jboolean JNICALL
		JAVA_FUNC(nInterrupt)(JNIEnv* env,jobject obj);

	JNIEXPORT jboolean JNICALL
		JAVA_FUNC(nCutVideoWithTime)(JNIEnv* env,jobject obj,jstring saveFile,jdouble startTime,double endTime);

	JNIEXPORT jboolean JNICALL
       	JAVA_FUNC(nCutVideoFillFrame)(JNIEnv* env,jobject obj,jstring saveFile,jdouble startTime,double endTime,jbyteArray frameColor,jint sizeMode);

	JNIEXPORT jboolean JNICALL
		JAVA_FUNC(nClose)(JNIEnv* env,jobject obj);

	JNIEXPORT jboolean JNICALL
		JAVA_FUNC(nSeekTo)(JNIEnv* env,jobject obj, jlong timeUS);

    JNIEXPORT jint JNICALL
        JAVA_FUNC(nGetVideoTrakIndex)(JNIEnv* env,jobject obj);

    JNIEXPORT jint JNICALL
        JAVA_FUNC(nGetAudioTrackIndex)(JNIEnv* env,jobject obj);

    JNIEXPORT jint JNICALL
        JAVA_FUNC(nGetSampleTrackIndex)(JNIEnv* env,jobject obj);

    JNIEXPORT jint JNICALL
        JAVA_FUNC(nReadSample)(JNIEnv* env,jobject obj,jobject byteBuffer, jint offset);

    JNIEXPORT jint JNICALL
        JAVA_FUNC(nDecodeVideo)(JNIEnv* env,jobject obj, jobject byteBuffer);

    JNIEXPORT jint JNICALL
        JAVA_FUNC(nGetOutputBufferSize)(JNIEnv* env,jobject obj);

    JNIEXPORT jlong JNICALL
        JAVA_FUNC(nGetSampleTime)(JNIEnv* env,jobject obj);

    JNIEXPORT jint JNICALL
        JAVA_FUNC(nGetSampleFlags)(JNIEnv* env,jobject obj);

    JNIEXPORT jboolean JNICALL
        JAVA_FUNC(nAdvance)(JNIEnv* env,jobject obj);

	JNIEXPORT jintArray JNICALL
        JAVA_FUNC(nGetRealOutputSize)(JNIEnv* env,jobject obj);

	JNIEXPORT void JNICALL
		JAVA_FUNC(nSetClipRegion)(JNIEnv* env, jobject obj, jint jX, jint jY, jint jWidth, jint jHeight);
#ifdef __cplusplus
}
#endif

#endif
#endif
