#if 0
#ifndef _VIDEO_EDITER_H_
#define _VIDEO_EDITER_H_
#include<jni.h>
//������
#define JAVA_FUNC(name) Java_com_meitu_media_tools_editor_VideoEditer_##name

//����JAVA NATIVE �ӿ�







#ifdef __cplusplus
extern "C"{
#endif
JNIEXPORT jboolean JNICALL
	JAVA_FUNC(nOpen)(JNIEnv* env,jobject obj,jstring videoFile);

JNIEXPORT jdouble JNICALL
	JAVA_FUNC(nGetVideoDuration)(JNIEnv* env,jobject obj);

JNIEXPORT jint JNICALL
	JAVA_FUNC(nGetVideoWidth)(JNIEnv* env,jobject obj);

JNIEXPORT jint JNICALL
	JAVA_FUNC(nGetVideoHeight)(JNIEnv* env,jobject obj);

JNIEXPORT jint JNICALL
	JAVA_FUNC(nGetShowWidth)(JNIEnv* env,jobject obj);

JNIEXPORT jint JNICALL
	JAVA_FUNC(nGetShowHeight)(JNIEnv* env,jobject obj);

JNIEXPORT jint JNICALL
	JAVA_FUNC(nGetAllKeyFrame)(JNIEnv* env,jobject obj,jstring tempFolder);

JNIEXPORT jboolean JNICALL
	JAVA_FUNC(nSetLeftTop)(JNIEnv* env,jobject obj,jint cropLeft,jint cropTop);

JNIEXPORT jboolean JNICALL
	JAVA_FUNC(nSetKeyFrameSize)(JNIEnv* env,jobject obj,jint width,jint height);

JNIEXPORT jboolean JNICALL
	JAVA_FUNC(nSetKeyFrameStep)(JNIEnv* env,jobject obj,jdouble frameStep);

JNIEXPORT jboolean JNICALL
	JAVA_FUNC(nCutVideoWithTime)(JNIEnv* env,jobject obj,jstring saveFile,jdouble startTime,double endTime);

JNIEXPORT jboolean JNICALL
	JAVA_FUNC(nCutVideoFillFrame)(JNIEnv* env,jobject obj,jstring saveFile,jdouble startTime,double endTime,jbyteArray frameColor);

JNIEXPORT jboolean JNICALL
	JAVA_FUNC(nClose)(JNIEnv* env,jobject obj);

JNIEXPORT jboolean JNICALL
		Java_com_meitu_media_tools_editor_VideoEditer__1reverseVideo(JNIEnv *env, jclass type, jstring saveFile_,
															   jdouble startTime, jdouble endTime);
JNIEXPORT jint JNICALL
		Java_com_meitu_media_tools_editor_VideoEditer__1getVideoRotation(JNIEnv *env, jobject instance);

JNIEXPORT void JNICALL
		Java_com_meitu_media_tools_editor_VideoEditer__1setSwsMode(JNIEnv *env, jclass type, jint mode);

JNIEXPORT jboolean JNICALL
		Java_com_meitu_media_tools_editor_VideoEditer__1isAborted(JNIEnv *env, jclass type);

JNIEXPORT void JNICALL
		Java_com_meitu_media_tools_editor_VideoEditer__1abort(JNIEnv *env, jclass type);
    
JNIEXPORT jint JNICALL
    JAVA_FUNC(nativeGetVideoBitrate)(JNIEnv* env,jobject obj);
#ifdef __cplusplus
}
#endif

#endif
#endif
