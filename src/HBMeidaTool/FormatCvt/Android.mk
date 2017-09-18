LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := FormatCvt

LOCAL_SRC_FILES := \
FormatCvt.cpp \
FormatCvt_row_any.cpp \
FormatCvt_row_c.cpp \
FormatCvt_row_neon.cpp \
FormatCvt_row_neon64.cpp 

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
 LOCAL_CPPFLAGS += -DHAVE_NEON=1 -DHAVE_NEON32=1
 LOCAL_CFLAGS += -DHAVE_NEON=1 -DHAVE_NEON32=1
    LOCAL_ARM_NEON  := true
 CPPFLAGS += -mfpu=neon
 CFLAGS += -mfpu=neon
endif

include $(BUILD_STATIC_LIBRARY)