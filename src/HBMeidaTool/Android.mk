LOCAL_PATH := $(call my-dir)

###########################
#
# mmtools shared library
#
###########################

include $(CLEAR_VARS)

LOCAL_MODULE := mmtools

LOCAL_SRC_FILES := \
JniHelper.cpp \
MTVideoTools.cpp \
MediaEdit_wrap.cxx \
MediaFilter.cpp \
FramePicker.cpp \
mmtools_android_jni.cpp \


LOCAL_C_INCLUDES := $(LOCAL_PATH)

LOCAL_LDLIBS := -llog

LOCAL_CFLAGS   +=  -fexceptions
LOCAL_CPPFLAGS := -Wno-deprecated-declarations
LOCAL_EXPORT_CPPFLAGS := -Wno-deprecated-declarations

LOCAL_CFLAGS += -D__STDC_CONSTANT_MACROS  -D__STDC_FORMAT_MACROS

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
 LOCAL_CPPFLAGS += -DHAVE_NEON=1 -DHAVE_NEON32=1
 LOCAL_CFLAGS += -DHAVE_NEON=1 -DHAVE_NEON32=1 -Wl,--no-warn-mismatch
    LOCAL_ARM_NEON  := true
 CPPFLAGS += -mfpu=neon
 CFLAGS += -mfpu=neon
endif

LOCAL_SHARED_LIBRARIES := ffmpeg

include $(BUILD_SHARED_LIBRARY)

###########################
#
# mmtools static library
#
###########################


LOCAL_MODULE := mmtools_static
LOCAL_MODULE_FILENAME := libmmtools

# exclude ASIO source code from $(LOCAL_SRC_FILES)
EXCLUDE_SRC_FILES := \
MTVideoTools.cpp \
MediaEdit_wrap.cxx \
JniHelper.cpp \
mmtools_android_jni.cpp \


LOCAL_SRC_FILES := $(filter-out $(EXCLUDE_SRC_FILES),$(LOCAL_SRC_FILES))

LOCAL_LDLIBS :=
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
LOCAL_EXPORT_LDLIBS := -llog

include $(BUILD_STATIC_LIBRARY)