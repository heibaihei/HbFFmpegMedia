#if 0
#include <stdio.h>
#include <iostream>
#include <fstream>

#include "VideoEditerAny_Jni.h"
//#include "ImageLoadUtils.h"
#include "VideoEditerAny.h"
#include "LogHelper.h"
#include "MTMacros.h"
#include "JniHelper.h"

#define STB_IMAGE_IMPLEMENTATION
//HAVE_NEON is defined in Android.mk !
#ifdef HAVE_NEON
#define STBI_NEON
#endif // !HAVE_NEON
#include "stb_image.h"

//��Ƶ�༭����
CVideoEditerAny* gVideoEditerAny=NULL;

#define JAVA_CLASS_VIDEOEDITER "com/meitu/media/tools/editor/VideoEditerAny"

#define VIDEO_EDITER_PROGRESS_BEGAN 0x01
#define VIDEO_EDITER_PROGRESS_CHANGED 0x02
#define VIDEO_EDITER_PROGRESS_ENDED 0x03
#define VIDEO_EDITER_PROGRESS_CANCELED 0x04

class AndroidVideoEditerAnyProgressListener : public VideoEditerAnyProgressListener {
public:
    AndroidVideoEditerAnyProgressListener() {}
    virtual ~AndroidVideoEditerAnyProgressListener() {
    }

    virtual void videoEditerAnyProgressBegan(CVideoEditerAny* editer) {
        postInfo(VIDEO_EDITER_PROGRESS_BEGAN, 0.0, 0.0);
    }

    virtual void videoEditerAnyProgressChanged(CVideoEditerAny* editer, double progress, double total) {
        postInfo(VIDEO_EDITER_PROGRESS_CHANGED, progress, total);
    }

    virtual void videoEditerAnyProgressEnded(CVideoEditerAny* editer) {
        postInfo(VIDEO_EDITER_PROGRESS_ENDED, 0.0, 0.0);
    }
    
    virtual void videoEditerAnyProgressCanceled(CVideoEditerAny* editer) {
        postInfo(VIDEO_EDITER_PROGRESS_CANCELED, 0, 0);
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
		LOGE("videoEditerAny open file failed");
		return JNI_FALSE;
	}
	SAFE_DELETE(gVideoEditerAny);
	gVideoEditerAny=new CVideoEditerAny;
    gVideoEditerAny->setProgressListener(new AndroidVideoEditerAnyProgressListener());
	int ret=gVideoEditerAny->Open(file);
	if (ret<0)
	{
		LOGE("videoEditerAny error when open file:%s",file);
		env->ReleaseStringUTFChars(videoFile,file);
		return JNI_FALSE;
	}
	env->ReleaseStringUTFChars(videoFile,file);

	return JNI_TRUE;
}


JNIEXPORT jdouble JNICALL
	JAVA_FUNC(nGetVideoDuration)(JNIEnv* env,jobject obj)
{
	if (NULL==gVideoEditerAny)
	{
		LOGE("videoEditer the object video not opened");
		return 0;
	}

	return gVideoEditerAny->GetVideoDuration();
}

JNIEXPORT jdouble JNICALL
	JAVA_FUNC(nGetProgressbarValue)(JNIEnv* env,jobject obj)
{
	if (NULL==gVideoEditerAny)
	{
		LOGE("videoEditer the object video not opened");
		return 0;
	}

	return gVideoEditerAny->GetProgressbarValue();
}

JNIEXPORT jboolean JNICALL
	JAVA_FUNC(nClearProgressBarValue)(JNIEnv* env,jobject obj)
{
	if (NULL==gVideoEditerAny)
	{
		LOGE("videoEditer the object video not opened");
		return false;
	}
	gVideoEditerAny->ClearProgressBarValue();
	return true;
}

JNIEXPORT jboolean JNICALL
	JAVA_FUNC(nSetImportMode)(JNIEnv* env,jobject obj,jint mode,jint size)
{
	if(NULL==gVideoEditerAny)
	{
		LOGE("videoEditer the object video not opened");
		return 0;
	}

	return (jboolean) gVideoEditerAny->SetImportMode((MTVideoImportMode)mode, size);
}

JNIEXPORT jboolean JNICALL
	JAVA_FUNC(nSetWaterMark)(JNIEnv* env,jobject obj,jstring jpath,jint meipaiWatermarkType)
{
	if(NULL == gVideoEditerAny)
	{
		LOGE("videoEditer the object video not opened");
		return JNI_FALSE;
	}
	const char* path = NULL;
	if(jpath)
	{
		path = env->GetStringUTFChars(jpath, JNI_FALSE);
	}

    std::ifstream ifs(path, std::ifstream::binary);
    filebuf *pbuf = ifs.rdbuf();
    uint8_t *buffer = nullptr;

    long size = pbuf->pubseekoff (0, ifs.end, ifs.in);
    pbuf->pubseekpos(0, ifs.in);

    buffer = new uint8_t[size];

    pbuf->sgetn((char *) buffer, size);

    ifs.close();


    int width, height, n;
    unsigned char* pixels = stbi_load_from_memory(buffer, (int)size, &width, &height, &n, 4);

    if(!pixels) { LOGE("stbi_load_from_memory error"); }

	int pathLen = strlen(path);
	int index = pathLen - 1;
	for( ; index >= 0; index--)
	{
		char a = path[index];
		char b = '/';
		if(a == b)
		{
			break;
		}
	}
	char temFolder[512];
	memset(temFolder, 0, sizeof(temFolder));
	memcpy(temFolder, path, index);

	char encodeFile[512];
	memset(encodeFile, 0, sizeof(encodeFile));
	sprintf(encodeFile, "%s/mmtools.watermark", temFolder);
	//LOGE("javan file = %s", encodeFile);
	//const char* encodeFile = "/mnt/sdcard/meipai.watermark";
	ADD_WaterMark::EncodeWatermarkToFile(encodeFile, pixels, width << 2, width, height);

	gVideoEditerAny->SetWaterMark(encodeFile, (MeipaiWatermarkType)meipaiWatermarkType);
	if(jpath)
	{
		env->ReleaseStringUTFChars(jpath, path);	
	}

	if (pixels) { stbi_image_free(pixels); }
	if (buffer) { delete[] buffer; }

	return true;
}

JNIEXPORT jboolean JNICALL
	JAVA_FUNC(nSetEndingWaterMark)(JNIEnv* env,jobject obj,jstring jpath)
{
	if(NULL == gVideoEditerAny)
	{
		LOGE("videoEditer the object video not opened");
		return JNI_FALSE;
	}
	/*
	const char* path = NULL;
	if(jpath)
	{
		path = env->GetStringUTFChars(jpath, JNI_FALSE);
	}

	int width,height;
	unsigned char* data = ImageLoadUtils::LoadImageData_File(path, &width, &height);
	LOGE("wfc LoadImageData_File width = %d, height = %d", width, height);

	int pathLen = strlen(path);
	int index = pathLen - 1;
	for( ; index >= 0; index--)
	{
		char a = path[index];
		char b = '/';
		if(a == b)
		{
			break;
		}
	}
	char temFolder[200];
	memset(temFolder, 0, sizeof(temFolder));
	memcpy(temFolder, path, index);

	char encodeFile[300];
	memset(encodeFile, 0, sizeof(encodeFile));
	sprintf(encodeFile, "%s/meipaiending.watermark", temFolder);
	ADD_WaterMark::EncodeWatermarkToFile(encodeFile, data, width << 2, width, height);

	gVideoEditerAny->SetEndingWaterMark(encodeFile);
	if(jpath)
	{
		env->ReleaseStringUTFChars(jpath, path);	
	}*/
	return true;
}

JNIEXPORT jboolean JNICALL
	JAVA_FUNC(nInterrupt)(JNIEnv* env,jobject obj)
{
	if (NULL==gVideoEditerAny)
	{
		LOGE("videoEditer the object video not opened");
		return JNI_FALSE;
	}
	gVideoEditerAny->Interrupt();

	return JNI_TRUE;
}


JNIEXPORT jboolean JNICALL
	JAVA_FUNC(nCutVideoWithTime)(JNIEnv* env,jobject obj,jstring saveFile,jdouble startTime,double endTime)
{
	if (NULL==gVideoEditerAny)
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
	int ret=gVideoEditerAny->CutVideoWithTime(file,startTime,endTime);
	env->ReleaseStringUTFChars(saveFile,file);
	if (ret < 0)
	{
		return JNI_FALSE;
	}

	return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
	JAVA_FUNC(nCutVideoFillFrame)(JNIEnv* env,jobject obj,jstring saveFile,jdouble startTime,double endTime,jbyteArray frameColor,jint sizeMode)
{
	if (NULL==gVideoEditerAny)
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
		ret = gVideoEditerAny->CutVideoWithFrame(file,startTime,endTime,values[0],values[1],values[2],(MTTargetVideoSize)sizeMode);
	}else{
		ret=gVideoEditerAny->CutVideoWithTime(file,startTime,endTime);
	}

	env->ReleaseStringUTFChars(saveFile,file);
	if (ret==-1)
	{
		return JNI_FALSE;
	}

	return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
	JAVA_FUNC(nClose)(JNIEnv* env,jobject obj)
{
	if (NULL==gVideoEditerAny)
	{
		LOGE("videoEditer the object video not opened");
		return JNI_FALSE;
	}
	gVideoEditerAny->Close();

	return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
        JAVA_FUNC(nSeekTo)(JNIEnv* env,jobject obj, jlong timeUS) {
    if (NULL==gVideoEditerAny)
    {
        LOGE("videoEditer the object video not opened");
        return JNI_FALSE;
    }

    return (jboolean) (gVideoEditerAny->seekTo(timeUS) >= 0);
}

JNIEXPORT jint JNICALL
        JAVA_FUNC(nGetVideoTrakIndex)(JNIEnv* env,jobject obj) {
    if (NULL==gVideoEditerAny)
    {
        LOGE("videoEditer the object video not opened");
        return JNI_FALSE;
    }

    return gVideoEditerAny->getVideoTrakIndex();
}

JNIEXPORT jint JNICALL
        JAVA_FUNC(nGetAudioTrackIndex)(JNIEnv* env,jobject obj) {
    if (NULL==gVideoEditerAny)
    {
        LOGE("videoEditer the object video not opened");
        return JNI_FALSE;
    }

    return gVideoEditerAny->getAudioTrackIndex();
}

JNIEXPORT jint JNICALL
        JAVA_FUNC(nGetSampleTrackIndex)(JNIEnv* env,jobject obj) {
    if (NULL==gVideoEditerAny)
    {
        LOGE("videoEditer the object video not opened");
        return -1;
    }

    return gVideoEditerAny->getSampleTrackIndex();
}

/**
 * The end of stream has been reached.
 */
static int END_OF_STREAM = -1;
/**
 * Neither a sample nor a format was read in full. This may be because insufficient data is
 * buffered upstream. If multiple tracks are enabled, this return value may indicate that the
 * next piece of data to be returned from the {@link SampleSource} corresponds to a different
 * track than the one for which data was requested.
 */
static int NOTHING_READ = -2;
/**
 * A sample was read.
 */
static int SAMPLE_READ = -3;
/**
 * A format was read.
 */
static int FORMAT_READ = -4;

JNIEXPORT jint JNICALL
        JAVA_FUNC(nReadSample)(JNIEnv* env,jobject obj, jobject byteBuffer, jint offset) {
    if (NULL==gVideoEditerAny)
    {
        LOGE("videoEditer the object video not opened");
        return JNI_FALSE;
    }
    uint8_t  *buffer = nullptr;
    if (byteBuffer!= nullptr) {
        buffer = (uint8_t *) (env)->GetDirectBufferAddress(byteBuffer);
        buffer += offset;
    }

    return gVideoEditerAny->readSample(buffer);
}

JNIEXPORT jint JNICALL
        JAVA_FUNC(nDecodeVideo)(JNIEnv* env, jobject obj, jobject byteBuffer) {
    if (NULL==gVideoEditerAny)
    {
        LOGE("videoEditer the object video not opened");
        return JNI_FALSE;
    }

    uint8_t *buffer = (uint8_t*)(env)->GetDirectBufferAddress(byteBuffer);
    int32_t bufferSize = (int32_t)env->GetDirectBufferCapacity(byteBuffer);
    int32_t outSize = 0;
    int64_t timeSample = -1;
    auto ret = gVideoEditerAny->decodeVideo(buffer, bufferSize, timeSample, outSize);
    if (ret) {
    }
//    LOGE("ret %d timeSample %lld, outSize %d",ret, timeSample, outSize);
    return (jint) (ret > 0 ? timeSample :-1);
}

JNIEXPORT jint JNICALL
JAVA_FUNC(nGetOutputBufferSize)(JNIEnv* env,jobject obj) {
    if (NULL==gVideoEditerAny)
    {
        LOGE("videoEditer the object video not opened");
        return JNI_FALSE;
    }

    return gVideoEditerAny->getOutputBufferSize();
}

JNIEXPORT jlong JNICALL
        JAVA_FUNC(nGetSampleTime)(JNIEnv* env,jobject obj) {
    if (NULL==gVideoEditerAny)
    {
        LOGE("videoEditer the object video not opened");
        return JNI_FALSE;
    }

    return gVideoEditerAny->getSampleTime();
}

JNIEXPORT jint JNICALL
        JAVA_FUNC(nGetSampleFlags)(JNIEnv* env,jobject obj) {
    if (NULL==gVideoEditerAny)
    {
        LOGE("videoEditer the object video not opened");
        return JNI_FALSE;
    }

    return gVideoEditerAny->getSampleFlags();
}


JNIEXPORT jboolean JNICALL
        JAVA_FUNC(nAdvance)(JNIEnv* env,jobject obj) {
    if (NULL==gVideoEditerAny)
    {
        LOGE("videoEditer the object video not opened");
        return JNI_FALSE;
    }

    return (jboolean) gVideoEditerAny->advance();
}

JNIEXPORT jintArray JNICALL
        JAVA_FUNC(nGetRealOutputSize)(JNIEnv* env,jobject obj) {
    if (NULL==gVideoEditerAny)
    {
        LOGE("videoEditer the object video not opened");
        return NULL;
    }

    const pair<int, int> &pair = gVideoEditerAny->getRealOutputSize();
    int cArray[] = {pair.first, pair.second};
    jintArray intArray = env->NewIntArray(2);

    env->SetIntArrayRegion(intArray,0, sizeof(cArray)/ sizeof(cArray[0]), cArray);

    return intArray;
}

JNIEXPORT void JNICALL
JAVA_FUNC(nSetClipRegion)(JNIEnv* env, jobject obj, jint jX, jint jY, jint jWidth, jint jHeight) 
{
	if (NULL==gVideoEditerAny)
    {
        LOGE("videoEditer the object video not opened");
        return;
    }

	gVideoEditerAny->SetCropRegion(jX, jY, jWidth, jHeight);
}
#endif
