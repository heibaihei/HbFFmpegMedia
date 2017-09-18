//
// Created by Javan on 15/8/20.
//
#if 0
#include "JniHelper.h"

#include <android/log.h>

#define  LOG_TAG    "JniHelper"
#ifndef LOGD
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#endif
#ifndef LOGE
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#endif

static pthread_key_t g_key;

jclass _getClassID(const char *className) {
    if (nullptr == className) {
        return nullptr;
    }

    JNIEnv* env = JniHelper::getEnv();

    jstring _jstrClassName = env->NewStringUTF(className);

    jclass _clazz = (jclass) env->CallObjectMethod(JniHelper::classloader,
                                                   JniHelper::loadclassMethod_methodID,
                                                   _jstrClassName);

    if (nullptr == _clazz) {
        LOGE("Classloader failed to find class of %s", className);
        env->ExceptionClear();
    }

    env->DeleteLocalRef(_jstrClassName);

    return _clazz;
}

void _detachCurrentThread(void* a) {
    JniHelper::getJavaVM()->DetachCurrentThread();
}


JavaVM* JniHelper::_psJavaVM = nullptr;
jmethodID JniHelper::loadclassMethod_methodID = nullptr;
jobject JniHelper::classloader = nullptr;

JavaVM* JniHelper::getJavaVM() {
    pthread_t thisthread = pthread_self();
    LOGD("JniHelper::getJavaVM(), pthread_self() = %ld", thisthread);
    return _psJavaVM;
}

void JniHelper::setJavaVM(JavaVM *javaVM) {
    pthread_t thisthread = pthread_self();
    LOGD("JniHelper::setJavaVM(%p), pthread_self() = %ld", javaVM, thisthread);
    _psJavaVM = javaVM;

    pthread_key_create(&g_key, _detachCurrentThread);
}

JNIEnv* JniHelper::cacheEnv(JavaVM* jvm) {
    JNIEnv* _env = nullptr;
    // get jni environment
    jint ret = jvm->GetEnv((void**)&_env, JNI_VERSION_1_4);

    switch (ret) {
        case JNI_OK :
            // Success!
            pthread_setspecific(g_key, _env);
            return _env;

        case JNI_EDETACHED :
            // Thread not attached
            if (jvm->AttachCurrentThread(&_env, nullptr) < 0)
            {
                LOGE("Failed to get the environment using AttachCurrentThread()");

                return nullptr;
            } else {
                // Success : Attached and obtained JNIEnv!
                pthread_setspecific(g_key, _env);
                return _env;
            }

        case JNI_EVERSION :
            // Cannot recover from this error
            LOGE("JNI interface version 1.4 not supported");
        default :
            LOGE("Failed to get the environment using GetEnv()");
            return nullptr;
    }
}

JNIEnv* JniHelper::getEnv() {
    JNIEnv *_env = (JNIEnv *)pthread_getspecific(g_key);
    if (_env == nullptr)
        _env = JniHelper::cacheEnv(_psJavaVM);
    return _env;
}

bool JniHelper::setClassLoaderFrom(jobject activityinstance) {
    JniMethodInfo _getclassloaderMethod;
    // Delete by Javan 2015.11.16. Don't need now.
    /*
    if (!JniHelper::getMethodInfo_DefaultClassLoader(_getclassloaderMethod,
                                                     "android/content/Context",
                                                     "getClassLoader",
                                                     "()Ljava/lang/ClassLoader;")) {
        return false;
    }

    jobject _c = JniHelper::getEnv()->CallObjectMethod(activityinstance,
                                                                _getclassloaderMethod.methodID);

    if (nullptr == _c) {
        return false;
    }

    JniMethodInfo _m;
    if (!JniHelper::getMethodInfo_DefaultClassLoader(_m,
                                                     "java/lang/ClassLoader",
                                                     "loadClass",
                                                     "(Ljava/lang/String;)Ljava/lang/Class;")) {
        return false;
    }

    JniHelper::classloader = JniHelper::getEnv()->NewGlobalRef(_c);
    JniHelper::loadclassMethod_methodID = _m.methodID;
*/  // end of delete.

    return true;
}

bool JniHelper::getStaticMethodInfo(JniMethodInfo &methodinfo,
                                    const char *className,
                                    const char *methodName,
                                    const char *paramCode) {
    if ((nullptr == className) ||
        (nullptr == methodName) ||
        (nullptr == paramCode)) {
        return false;
    }

    JNIEnv *env = JniHelper::getEnv();
    if (!env) {
        LOGE("Failed to get JNIEnv");
        return false;
    }

    jclass classID = env->FindClass(className);
    if (! classID) {
        LOGE("Failed to find class %s", className);
        env->ExceptionClear();
        return false;
    }

    jmethodID methodID = env->GetStaticMethodID(classID, methodName, paramCode);
    if (! methodID) {
        LOGE("Failed to find static method id of %s", methodName);
        env->ExceptionClear();
        return false;
    }

    methodinfo.classID = classID;
    methodinfo.env = env;
    methodinfo.methodID = methodID;
    return true;
}

bool JniHelper::getMethodInfo_DefaultClassLoader(JniMethodInfo &methodinfo,
                                                 const char *className,
                                                 const char *methodName,
                                                 const char *paramCode) {
    if ((nullptr == className) ||
        (nullptr == methodName) ||
        (nullptr == paramCode)) {
        return false;
    }

    JNIEnv *env = JniHelper::getEnv();
    if (!env) {
        return false;
    }

    jclass classID = env->FindClass(className);
    if (! classID) {
        LOGE("Failed to find class %s", className);
        env->ExceptionClear();
        return false;
    }

    jmethodID methodID = env->GetMethodID(classID, methodName, paramCode);
    if (! methodID) {
        LOGE("Failed to find method id of %s", methodName);
        env->ExceptionClear();
        return false;
    }

    methodinfo.classID = classID;
    methodinfo.env = env;
    methodinfo.methodID = methodID;

    return true;
}

bool JniHelper::getMethodInfo(JniMethodInfo &methodinfo,
                              const char *className,
                              const char *methodName,
                              const char *paramCode) {
    if ((nullptr == className) ||
        (nullptr == methodName) ||
        (nullptr == paramCode)) {
        LOGE("parameter is err %s", className);
        return false;
    }

    JNIEnv *env = JniHelper::getEnv();
    if (!env) {
        LOGE("Get Env err %s", className);
        return false;
    }

    jclass classID = env->FindClass(className);
    if (! classID) {
        LOGE("Failed to find class %s", className);
        env->ExceptionClear();
        return false;
    }

    jmethodID methodID = env->GetMethodID(classID, methodName, paramCode);
    if (! methodID) {
        LOGE("Failed to find method id of %s", methodName);
        env->ExceptionClear();
        return false;
    }

    methodinfo.classID = classID;
    methodinfo.env = env;
    methodinfo.methodID = methodID;

    return true;
}

std::string JniHelper::jstring2string(jstring jstr) {
    if (jstr == nullptr) {
        return "";
    }

    JNIEnv *env = JniHelper::getEnv();
    if (!env) {
        return nullptr;
    }

    const char* chars = env->GetStringUTFChars(jstr, nullptr);
    std::string ret(chars);
    env->ReleaseStringUTFChars(jstr, chars);

    return ret;
}
#endif
