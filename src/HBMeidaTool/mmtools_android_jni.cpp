#if 0
#include <cassert>

#include "JniHelper.h"
#include "LogHelper.h"

extern "C" {
JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JniHelper::setJavaVM(vm);

    jclass clazz;
    JNIEnv *env = JniHelper::getEnv();
    assert(env != NULL);

    LOGI("%s", __func__);

    return JNI_VERSION_1_4;
}

JNIEXPORT void JNI_OnUnload(JavaVM *jvm, void *reserved)
{
    LOGI("%s", __func__);
}
}
#endif
