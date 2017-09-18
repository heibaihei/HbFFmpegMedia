#if 0
#include "VideoEditer_Jni.h"
#include "VideoEditer.h"
#include "LogHelper.h"
#include "MTMacros.h"
#include "JniHelper.h"

#define JAVA_CLASS_VIDEOEDITER "com/meitu/media/tools/editor/VideoEditer"

static CVideoEditer* gVideoEditer=NULL;

#define VIDEO_EDITER_PROGRESS_BEGAN 0x01
#define VIDEO_EDITER_PROGRESS_CHANGED 0x02
#define VIDEO_EDITER_PROGRESS_ENDED 0x03


class AndroidVideoEditerProgressListener : public VideoEditerProgressListener {
public:
    AndroidVideoEditerProgressListener() {}
	~AndroidVideoEditerProgressListener() {
    }

    virtual void videoEditerProgressBegan(CVideoEditer* editer) {
        postInfo(VIDEO_EDITER_PROGRESS_BEGAN, 0.0, 0.0);
    }

    virtual void videoEditerProgressChanged(CVideoEditer* editer, double progress, double total) {
        postInfo(VIDEO_EDITER_PROGRESS_CHANGED, progress, total);
    }

    virtual void videoEditerProgressEnded(CVideoEditer* editer) {
        postInfo(VIDEO_EDITER_PROGRESS_ENDED, 0.0, 0.0);
    }

private:
    void postInfo(int what, double arg1, double arg2) {
        JniMethodInfo methodInfo;
        if (!JniHelper::getStaticMethodInfo(methodInfo, JAVA_CLASS_VIDEOEDITER, "_postInfo",
                                            "(IDD)V"))
        {
            LOGE("%s %d: error to get methodInfo", __FILE__, __LINE__);
            return;
        }

        methodInfo.env->CallStaticVoidMethod(methodInfo.classID, methodInfo.methodID, what, arg1, arg2);
        methodInfo.env->DeleteLocalRef(methodInfo.classID);
    }

private:

};

JNIEXPORT jboolean JNICALL
	JAVA_FUNC(nOpen)(JNIEnv* env,jobject obj,jstring videoFile)
{
	const char* file=env->GetStringUTFChars(videoFile,JNI_FALSE);
	if (videoFile==0||file==NULL)
	{
		LOGE("videoEditer open file failed");
		return JNI_FALSE;
	}
	SAFE_DELETE(gVideoEditer);
	gVideoEditer=new CVideoEditer;
    AndroidVideoEditerProgressListener *progressListener = new AndroidVideoEditerProgressListener();
    gVideoEditer->setProgressListener(progressListener);

	int ret=gVideoEditer->Open(file);
	if (ret<0)
	{
		LOGE("videEditer error when open file:%s",file);
		env->ReleaseStringUTFChars(videoFile,file);
		return JNI_FALSE;
	}
	env->ReleaseStringUTFChars(videoFile,file);

	return JNI_TRUE;
}


JNIEXPORT jdouble JNICALL
	JAVA_FUNC(nGetVideoDuration)(JNIEnv* env,jobject obj)
{
	if (NULL==gVideoEditer)
	{
		LOGE("videoEditer the object video not opened");
		return 0;
	}

	return gVideoEditer->GetVideoDuration();
}

JNIEXPORT jint JNICALL
Java_com_meitu_media_tools_editor_VideoEditer__1getVideoRotation(JNIEnv *env, jobject instance) {
    if (NULL==gVideoEditer)
    {
        LOGE("videoEditer the object video not opened");
        return 0;
    }

    return gVideoEditer->getVideoRotation();
}

JNIEXPORT jint JNICALL
	JAVA_FUNC(nGetVideoWidth)(JNIEnv* env,jobject obj)
{
	if (NULL==gVideoEditer)
	{
		LOGE("videoEditer the object video not opened");
		return 0;
	}
	return gVideoEditer->GetVideoWidth();
}

JNIEXPORT jint JNICALL
	JAVA_FUNC(nGetVideoHeight)(JNIEnv* env,jobject obj)
{
	if (NULL==gVideoEditer)
	{
		LOGE("videoEditer the object video not opened");
		return 0;
	}

	return gVideoEditer->GetVideoHeight();
}

JNIEXPORT jint JNICALL
	JAVA_FUNC(nGetShowWidth)(JNIEnv* env,jobject obj)
{
	if (NULL==gVideoEditer)
	{
		LOGE("videoEditer the object video not opened");
		return 0;
	}

	return gVideoEditer->GetShowWidth();
}

JNIEXPORT jint JNICALL
	JAVA_FUNC(nGetShowHeight)(JNIEnv* env,jobject obj)
{
	if (NULL==gVideoEditer)
	{
		LOGE("videoEditer the object video not opened");
		return 0;
	}

	return gVideoEditer->GetShowHeight();
}

JNIEXPORT jint JNICALL
	JAVA_FUNC(nGetAllKeyFrame)(JNIEnv* env,jobject obj,jstring tempFolder)
{
	if (NULL==gVideoEditer)
	{
		LOGE("videoEditer the object video not opened");
		return 0;
	}
	const char* folder=env->GetStringUTFChars(tempFolder,JNI_FALSE);
	if (tempFolder==0||folder==NULL)
	{
		LOGE("videoEditer failed to getKeyFrame: tempFolder is null");
		return 0;
	}
	LOGD("hrx  getAllKeyFrame start");
	int frameCount=gVideoEditer->GetAllKeyFrame(folder);
	LOGD("hrx  getAllKeyFrame end");
	env->ReleaseStringUTFChars(tempFolder,folder);

	return frameCount;
}

JNIEXPORT jboolean JNICALL
	JAVA_FUNC(nSetLeftTop)(JNIEnv* env,jobject obj,jint cropLeft,jint cropTop)
{
	if (NULL==gVideoEditer)
	{
		LOGE("videoEditer the object video not opened");
		return JNI_FALSE;
	}

	if (gVideoEditer->SetLeftTop(cropLeft,cropTop)==-1)
	{
		LOGE("videoEditer unknown error");
		return JNI_FALSE;
	}

	return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
	JAVA_FUNC(nSetKeyFrameSize)(JNIEnv* env,jobject obj,jint width,jint height)
{
	if (NULL==gVideoEditer)
	{
		LOGE("videoEditer the object video not opened");
		return JNI_FALSE;
	}
	gVideoEditer->SetKeyFrameSize(width,height);
	
	return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
	JAVA_FUNC(nSetKeyFrameStep)(JNIEnv* env,jobject obj,jdouble frameStep)
{
	if (NULL==gVideoEditer)
	{
		LOGE("videoEditer the object video not opened");
		return JNI_FALSE;
	}
	gVideoEditer->SetKeyFrameStep(frameStep);

	return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
	JAVA_FUNC(nCutVideoWithTime)(JNIEnv* env,jobject obj,jstring saveFile,jdouble startTime,double endTime)
{
	if (NULL==gVideoEditer)
	{
		LOGE("videoEditer the object video not opened");
		return JNI_FALSE;
	}
	const char* file=env->GetStringUTFChars(saveFile,JNI_FALSE);
	if (saveFile==0||file==NULL)
	{
		LOGE("videoEditer error cutVideo savepath is null");
		return JNI_FALSE;
	}
	int ret=gVideoEditer->CutVideo(file,startTime,endTime);
	env->ReleaseStringUTFChars(saveFile,file);
	if (ret==-1)
	{
		return JNI_FALSE;
	}

	return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
	JAVA_FUNC(nCutVideoFillFrame)(JNIEnv* env,jobject obj,jstring saveFile,jdouble startTime,double endTime,jbyteArray frameColor)
{
	if (NULL==gVideoEditer)
	{
		LOGE("videoEditer the object video not opened");
		return JNI_FALSE;
	}
	const char* file=env->GetStringUTFChars(saveFile,JNI_FALSE);
	if (saveFile==0||file==NULL)
	{
		LOGE("videoEditer error cutVideo savepath is null");
		return JNI_FALSE;
	}
	int ret =-1;
	if (frameColor&&env->GetArrayLength(frameColor)>=3)
	{
		jbyte values[3] = {0};
		env->GetByteArrayRegion(frameColor,0,3,values);
		ret = gVideoEditer->CutVideo(file,startTime,endTime,values[0],values[1],values[2]);
	}else{
		ret=gVideoEditer->CutVideo(file,startTime,endTime);
	}

	env->ReleaseStringUTFChars(saveFile,file);
	if (ret==-1)
	{
		return JNI_FALSE;
	}

	return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_meitu_media_tools_editor_VideoEditer__1reverseVideo(JNIEnv *env, jclass type, jstring saveFile_,
													   jdouble startTime, jdouble endTime) {
	if (NULL == gVideoEditer) {
		LOGE("videoEditer the object video not opened");
		return JNI_FALSE;
	}

	const char *saveFile = env->GetStringUTFChars(saveFile_, 0);

	if (saveFile == NULL) {
		LOGE("videoEditer error cutVideo savepath is null");
		return JNI_FALSE;
	}
	int ret = gVideoEditer->ReCutVideoWithTime(saveFile, startTime, endTime);
	env->ReleaseStringUTFChars(saveFile_, saveFile);
	if (ret == -1) {
		return JNI_FALSE;
	}

	return JNI_TRUE;

	env->ReleaseStringUTFChars(saveFile_, saveFile);
}

JNIEXPORT jboolean JNICALL
	JAVA_FUNC(nClose)(JNIEnv* env,jobject obj)
{
	if (NULL==gVideoEditer)
	{
		LOGE("videoEditer the object video not opened");
		return JNI_FALSE;
	}
	gVideoEditer->Close();

	return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_meitu_media_tools_editor_VideoEditer__1setSwsMode(JNIEnv *env, jclass type, jint mode) {
    CVideoEditer::SetVideoEditerSwsMode(mode);
}

JNIEXPORT void JNICALL
Java_com_meitu_media_tools_editor_VideoEditer__1abort(JNIEnv *env, jclass type) {

    if (NULL==gVideoEditer)
    {
        LOGE("videoEditer the object video not opened");
        return;
    }
    gVideoEditer->abort();
}

JNIEXPORT jboolean JNICALL
Java_com_meitu_media_tools_editor_VideoEditer__1isAborted(JNIEnv *env, jclass type) {

    if (NULL==gVideoEditer)
    {
        LOGE("videoEditer the object video not opened");
        return JNI_TRUE;
    }
    return (jboolean)gVideoEditer->isAborted();
}

JNIEXPORT jint JNICALL
	JAVA_FUNC(nativeGetVideoBitrate)(JNIEnv* env,jobject obj)
{
	if (NULL==gVideoEditer)
	{
		LOGE("videoEditer the object video not opened");
		return 0;
	}

	return gVideoEditer->getVideoBitrate();
}
#endif
