LOCAL_PATH := $(call my-dir)
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
include $(CLEAR_VARS)
LOCAL_MODULE := vorbislib
LOCAL_SRC_FILES := ../../../../app/src/main/cpp/prebuilt/arm64-v8a/libvorbislib.so
include $(PREBUILT_SHARED_LIBRARY)
include $(CLEAR_VARS)
LOCAL_MODULE := mlbb_vorbis_jni
LOCAL_SRC_FILES := ../../../../app/src/main/cpp/mlbb_vorbis_jni.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../app/src/main/cpp/include
LOCAL_SHARED_LIBRARIES := vorbislib
LOCAL_CPPFLAGS := -std=c++17
include $(BUILD_SHARED_LIBRARY)
endif
